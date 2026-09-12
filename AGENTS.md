# AGENTS.md

## Project overview

This repository is the GitHub checkout of **Komandare** — a UserLand Linux distribution for Windows built on the Cygwin kernel. One folder, full Unix userland and dev toolchains, zero traces on the host.

- **Vendor**: xRetia Labs
- **Homepage**: <https://komandare.github.io/>
- **Repo**: <https://github.com/xRetia/Komandare>

The repo holds the docs/site source plus the dev build toolchain. The full distro tree (launchers, `Module/`, `Config/`, `Unixlike/`, `Binary/`) lives in `C:\Komandare` on the maintainer's machine and is never committed here — it is distributed through the NSIS installer + `.kmp` module packages.

## What lives in this repo

| Path | Role |
| --- | --- |
| `index.html` | Bilingual (EN/ZH) landing page for the GitHub Pages site |
| `README.md` | The main English readme |
| `README.CN.md` | The Chinese readme |
| `Source/logo.png` | Project logo used by the site, readmes, and installer |
| `Source/kmd.c` | C source of the `kmd.exe` launcher |
| `Source/kRun.c` | C source of the `kRun.exe` launcher + Windows Run-style dialog |
| `Source/kRun.rc` | Resource source for `kRun.exe` (icon + dialog template) |
| `Source/build_krun.ps1` | Build script for `kRun.exe` (windres + zero-CRT gcc) |
| `Source/build-release.ps1` | Release packer (NSIS installer + `.kmp` module packs + `registry.kp`) |
| `Source/NSIS/` | Dev-only NSIS toolchain (`makensis`) used by `build-release.ps1` — never shipped |
| `Source/build_kmd.py` | Dev tool that dumps `kRun.exe` PE resources (legacy bat2exe format only) |
| `AGENTS.md` | This file |

## Keeping docs accurate — ground truth

Documentation repeatedly hard-codes facts that drift. When editing docs, verify against the real tree at `C:\Komandare`:

- **Version** — `Config\Kernel.ini` → `Version=`. This appears in the neofetch mock inside `index.html` and both readmes (`System Version`, `Komandare Version`, and the download card `stable · v...`).
- **`/kbin` command list** — `C:\Komandare\Unixlike\kbin\`. Every command appears in three places: the neofetch `Unique Libs` line + the kbin table in `index.html`, and the kbin section in both readmes. The three must stay in sync.
- **Module catalog** — `C:\Komandare\Module\` + registered loaders in `C:\Komandare\Config\Init.d\`. The set is mirrored in the Module tables of `index.html`, `README.md`, `README.CN.md`, and the prose "Bundled modules today" line.
- **Binary tools** — `C:\Komandare\Binary\`. Ports/wrappers that live there must not appear in the kbin tables and vice versa (e.g. `service` moved from `Binary/service.bat` to a bash script `Unixlike/kbin/service` — both readmes, the Binary table, the kbin table, and the Services section must agree).
- **kmod-setup version/features** — `Module\kmod-setup\kmod-setup` + its loader meta `version=` in `Config\Init.d\01-kmod-setup.cmd`. The kmod-setup section of each readme references major version + feature highlights.

## Editable sites/html conventions

`index.html` is a single-file bilingual page. English text is the default `innerHTML`; Chinese translations live in `data-zh` attributes (the page's JS swaps them). When editing:

- Add/change English text in the element body **and** mirror it in `data-zh`.
- HTML inside attributes must be entity-escaped (e.g. `&lt;code&gt;`).
- Keep EN/ZH content equivalent in meaning, not necessarily word-for-word.

The neofetch terminal mock in `index.html` must match the real output (see `Unixlike/kbin/version` / `neofetch` in the live tree).

## README EN ↔ CN

`README.md` and `README.CN.md` are mirrors. For every structural change (feature blurbs, tables, sections, FAQ entries), apply the same edit to both. Line-by-line parity is the norm; keep them identically ordered and sectioned.

## Releasing

Do not run the full release from this repo — `Source/build-release.ps1` walks up to the real tree root (it needs `Config\Kernel.ini`, `Module\7zip\7z.exe`, `Unixlike\bin\bash.exe`, `.github\Source\NSIS\makensis.exe`). It produces:

```
.release\Komandare-Setup-<ver>.exe   NSIS installer (full base runtime embedded)
.release\kmp\<module>.kmp            one .kmp per registered module
.release\registry.kp                 module index (version/size/sha256/file)
```

Examples:

```bash
powershell -ExecutionPolicy Bypass -File Source\build-release.ps1            # full release
powershell -ExecutionPolicy Bypass -File Source\build-release.ps1 -SkipNSIS  # module packs only
powershell -ExecutionPolicy Bypass -File Source\build-release.ps1 -SkipKmp   # base + installer only
```

Important release facts:

- `Config\AIShell.ini` (contains a live API key) is **stripped from the shipped payload** — never commit keys; the repo's own copies of such per-user settings should likewise never hold secrets.
- Modules are **not embedded** in the installer; users fetch them post-install from the GitHub release mirror via `kmod-setup install -r <name>`.
- `FirstRun=yes` is forced in the shipped `Kernel.ini` so fresh installs run the `kmd-welcome` wizard.

## Editing rules

- Do not add comments to code/files unless asked.
- Do not commit secrets or rotate existing keys in committed files.
- Commit only when explicitly requested; the repo is hosted at `github.com/xRetia/Komandare`.