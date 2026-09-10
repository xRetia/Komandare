#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Dump krun.exe PE resources and try zlib-inflate each one.
bat2exe stores the wrapped script as (usually compressed) resources.
"""
import struct, zlib, sys

def rva_to_off(pe, rva):
    data, sections = pe['data'], pe['sections']
    for name, va, vsz, raw_off, raw_sz in sections:
        if va <= rva < va + max(vsz, raw_sz):
            return raw_off + (rva - va)
    return None

def parse_pe(path):
    data = open(path, 'rb').read()
    if data[:2] != b'MZ':
        raise SystemExit('not a PE')
    pe_off = struct.unpack_from('<I', data, 0x3C)[0]
    if data[pe_off:pe_off+4] != b'PE\x00\x00':
        raise SystemExit('bad PE sig')
    machine = struct.unpack_from('<H', data, pe_off+4)[0]
    nsec = struct.unpack_from('<H', data, pe_off+6)[0]
    opt_off = pe_off + 24
    magic = struct.unpack_from('<H', data, opt_off)[0]
    opt_size = struct.unpack_from('<H', data, opt_off+20)[0]
    res_rva, res_sz = struct.unpack_from('<II', data, opt_off+112)
    sec_off = opt_off + opt_size
    sections = []
    for i in range(nsec):
        so = sec_off + i*40
        name = data[so:so+8].rstrip(b'\x00').decode('latin1')
        vsz, va, raw_sz, raw_off = struct.unpack_from('<IIII', data, so+8)
        sections.append((name, va, vsz, raw_off, raw_sz))
    return dict(machine=machine, sections=sections,
                res_rva=res_rva, res_sz=res_sz, data=data)

def walk(pe, rva, depth, prefix, out):
    """rva is always relative to resource directory root (res_rva)."""
    data = pe['data']
    abs_rva = pe['res_rva'] + rva
    off = rva_to_off(pe, abs_rva)
    if off is None:
        return out
    nNamed, nID = struct.unpack_from('<HH', data, off+12)
    base = off + 16
    for i in range(nNamed + nID):
        eo = base + i*8
        name_id, off_to = struct.unpack_from('<II', data, eo)
        is_dir = off_to >> 31
        off_to &= 0x7FFFFFFF
        if name_id & 0x80000000:
            name_off = rva_to_off(pe, pe['res_rva'] + (name_id & 0x7FFFFFFF))
            length = struct.unpack_from('<H', data, name_off)[0]
            name = data[name_off+2:name_off+2+length*2].decode('utf-16le', 'replace')
        else:
            name = str(name_id)
        if is_dir:
            walk(pe, off_to, depth+1, prefix + '/' + name, out)
        else:
            eoff = rva_to_off(pe, pe['res_rva'] + off_to)
            data_rva, size, _ = struct.unpack_from('<III', data, eoff)
            d_off = rva_to_off(pe, data_rva)
            blob = data[d_off:d_off+size]
            out.append({'type': prefix, 'name': name, 'size': size, 'blob': blob})
    return out

def try_inflate(blob):
    attempts = []
    for wbits in (15, -15, 47):
        try:
            d = zlib.decompressobj(wbits)
            out = d.decompress(blob) + d.flush()
            attempts.append((wbits, out))
        except Exception:
            pass
    return attempts

if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else r'C:\Komandare\krun.exe'
    pe = parse_pe(path)
    res = walk(pe, 0, 0, '', [])
    print(f'== {len(res)} resource leaves ==')
    for r in res:
        print(f"  type={r['type']} name={r['name']} size={r['size']}")
    for r in res:
        blob = r['blob']
        for wbits, out in try_inflate(blob):
            text = out.decode('utf-8', 'replace')
            print(f"\n=== type={r['type']} name={r['name']} orig={r['size']} "
                  f"-> zlib wbits={wbits} inflated {len(out)} bytes ===")
            print(text[:4000])
            break
        # raw text hint
        if r['size'] and r['size'] < 300:
            try:
                t = blob.decode('utf-8', 'replace')
                if t.isprintable() or 'bat' in t.lower():
                    print(f"  (raw?) {t!r}")
            except Exception:
                pass