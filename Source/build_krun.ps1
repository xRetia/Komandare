# build_krun.ps1 - rebuild kRun.exe from kRun.c/kRun.rc (zero-CRT native PE)
# Output: <Komandare root>\kRun.exe (replaces the bat2exe-wrapped binary)
#
# Usage: powershell -ExecutionPolicy Bypass -File Source\build_krun.ps1

$ErrorActionPreference = "Stop"

$srcDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root   = Split-Path -Parent (Split-Path -Parent $srcDir)

$gcc  = Join-Path $root "Unixlike\bin\x86_64-pc-cygwin-gcc.exe"
$wres = Join-Path $root "Unixlike\bin\windres.exe"
$out  = Join-Path $root "kRun.exe"

if (-not (Test-Path $gcc))  { throw "gcc not found: $gcc" }
if (-not (Test-Path $wres)) { throw "windres not found: $wres" }

Push-Location $srcDir
try {
    & $wres kRun.rc -O coff -o kRun.res
    if (-not $?) { throw "windres failed" }
    & $gcc -nostdlib -mwindows "-Wl,-e,main" "-Wl,--subsystem,windows" -o $out kRun.c kRun.res -lkernel32 -luser32 -lshell32
    if (-not $?) { throw "gcc failed" }
} finally {
    Pop-Location
}

Write-Host "built: $out"