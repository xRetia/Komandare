# build-release.ps1 - Komandare release packer (NSIS installer)
# Produces .release\ output:
#   Komandare-Setup-<ver>.exe   NSIS installer (base runtime embedded)
#   kmp\<module>.kmp            one per Module/* (via kmod-setup export)
#   registry.kp                 module index: version/size/sha256/file/description
#
# Installer embeds the full base file tree (Unixlike + Config + Binary + bats).
# Modules are NOT embedded - they are fetched post-install from the release
# mirror via kmod-setup (mirror default = GitHub releases/latest/download).
#
# Usage: powershell -ExecutionPolicy Bypass -File Source\build-release.ps1 [options]
# cmd-style switches (/switch=value, /flag, /?, /help; "-" may replace "/"):
#   /skipnsis        skip the NSIS installer step (module packs + registry only)
#   /skipkmp         skip module packing + registry.kp (base + NSIS only)
#   /modules=a,b     pack only these module names (default: ALL registered)
#   /desc=path       module annotation file (default: Source\registry-desc.ini)
#   /out=dir         output directory (default: <root>\.release)
#   /?, /h, /help    show this help and exit
#
# module annotations: registry.kp sections get a description= line from the
# annotation file (one "base=text" per line); it is the sole authoritative
# source for sync - kmod-setup reads registry.kp registry-first.

$ErrorActionPreference = "Stop"

# ---- cmd-style argument parsing: /switch=value, /flag, /?, /help ----
$usage = @'
build-release.ps1 - Komandare release packer (NSIS installer)

Usage: powershell -ExecutionPolicy Bypass -File Source\build-release.ps1 [options]

Options (cmd-style switches; "-" may replace "/"):
  /?, /h, /help     show this help and exit
  /skipnsis         skip the NSIS installer step (module packs + registry only)
  /skipkmp          skip module packing + registry.kp (base + NSIS only)
  /modules=a,b      pack only these modules (comma separated; default: all)
  /desc=path        module annotation/description file (default: Source\registry-desc.ini)
  /out=dir          output directory (default: <root>\.release)

Examples:
  build-release.ps1
  build-release.ps1 /skipnsis
  build-release.ps1 /skipnsis /modules=7zip,php
  build-release.ps1 /skipkmp /out=C:\rel\test
'@

$opt = @{
    SkipNSIS = $false
    SkipKmp  = $false
    Modules  = @()
    Desc     = $null
    OutDir   = $null
    Help     = $false
}
$pending = $null   # switch name waiting for its value from the next token
foreach ($raw in $args) {
    $a = "$raw"
    if ($pending -ne $null) {
        if ($pending -eq 'Modules') { $opt.Modules = @(($a -split '[,;]') | ForEach-Object { $_.Trim() } | Where-Object { $_ }) }
        elseif ($pending -eq 'Desc')  { $opt.Desc  = $a }
        elseif ($pending -eq 'OutDir'){ $opt.OutDir = $a }
        $pending = $null
        continue
    }
    $sw = ""; $val = $null
    if ($a -match '^[/-]([^=]+?)(?:=(.*))?$') {
        $sw  = $Matches[1].Trim().ToLowerInvariant()
        if ($Matches.Count -gt 2 -and $null -ne $Matches[2] -and $Matches[2].Length -gt 0) { $val = $Matches[2] }
    } else {
        throw "unrecognized argument: $a"
    }
    switch ($sw) {
        '?'       { $opt.Help = $true }
        'h'       { $opt.Help = $true }
        'help'    { $opt.Help = $true }
        'skipnsis'{ $opt.SkipNSIS = $true }
        'skipkmp' { $opt.SkipKmp = $true }
        'modules' { if ($null -eq $val) { $pending = 'Modules' } else { $opt.Modules = @(($val -split '[,;]') | ForEach-Object { $_.Trim() } | Where-Object { $_ }) } }
        'desc'    { if ($null -eq $val) { $pending = 'Desc' }    else { $opt.Desc = $val } }
        'out'     { if ($null -eq $val) { $pending = 'OutDir' }  else { $opt.OutDir = $val } }
        default   { Write-Host $usage; throw "unrecognized switch: /$sw" }
    }
}
if ($null -ne $pending) { throw "missing value for switch /$pending" }
if ($opt.Help) { Write-Host $usage; exit 0 }
if ($opt.SkipNSIS -and $opt.SkipKmp) {
    Write-Host "warning: /skipnsis + /skipkmp = base payload only" -ForegroundColor Yellow
}
$SkipNSIS = $opt.SkipNSIS
$SkipKmp  = $opt.SkipKmp
$Modules  = $opt.Modules

# the script lives in <root>\Source (or <root>\.github\Source for the cloned
# dev repo); walk up until we hit the real Komandare tree (has Config\Kernel.ini)
$root  = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
while (-not (Test-Path (Join-Path $root "Config\Kernel.ini")) -and (Split-Path -Leaf $root) -ne (Split-Path -Leaf (Split-Path $root -Parent))) {
    $root = Split-Path -Parent $root
}
if (-not (Test-Path (Join-Path $root "Config\Kernel.ini"))) { throw "cannot locate Komandare root (no Config\Kernel.ini under $root)" }
$rel   = if ($opt.OutDir) { $opt.OutDir } else { Join-Path $root ".release" }
$stage = Join-Path $rel "stage"
$7z    = Join-Path $root "Module\7zip\7z.exe"
$bash  = Join-Path $root "Unixlike\bin\bash.exe"
$nsis  = Join-Path $root ".github\Source\NSIS\makensis.exe"

if (-not (Test-Path $7z))   { throw "7z.exe not found: $7z" }
if (-not (Test-Path $bash)) { throw "bash.exe not found: $bash" }
if (-not $SkipNSIS -and -not (Test-Path $nsis)) { throw "makensis.exe not found: $nsis (NSIS toolchain lives in .github\Source\NSIS)" }

# ---------------------------------------------------------------- clean stage
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
$kmpDir = Join-Path $rel "kmp"
if (Test-Path $kmpDir) { Remove-Item $kmpDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $kmpDir | Out-Null
New-Item -ItemType Directory -Force -Path "$stage\base" | Out-Null

Write-Host "== Komandare release build (NSIS) ==" -ForegroundColor Cyan
Write-Host "root: $root"

# ---------------------------------------------------------------- 1. base payload
Write-Host "`n[1/5] building base payload (embedded in installer) ..."
$payload = Join-Path $stage "base"

# pack full Unixlike with cygwin tar (NOT 7z): 7z follows junctions and
# derefs them into the archive, and treats cygwin reparse-point symlinks as
# broken, dropping them. tar keeps symlinks as links. kmod/kconf/tmp are now
# Cygwin TEXT symlinks ("!<symlink>" plain files, System attr) - tar packs
# them byte-exact and NSIS copies them as ordinary files, so they ship in
# the installer and kInitrd's klinks.sh self-heals them at boot.
# Sockets under .apt-cyg are ignored by tar with a warning (harmless).
$tar  = Join-Path $root "Unixlike\bin\tar.exe"
$posixRoot = $root.ToLower() -replace '\\','/' -replace '^([a-z]):', '/mnt/$1'
$posixTar  = ((Join-Path $payload "_u.tar").ToLower() -replace '\\','/' -replace '^([a-z]):', '/mnt/$1')
$posixOut  = ($payload.ToLower() -replace '\\','/' -replace '^([a-z]):', '/mnt/$1')
# CYGWIN=winsymlinks:sys on BOTH pack and extract: without it Cygwin recreates
# symlinks as reparse points on extract (needs no admin here, but produces
# non-plain files NSIS then mishandles). With :sys they stay text files.
& $bash -lc "export PATH=/bin:/usr/bin; cd '$posixRoot' && CYGWIN=winsymlinks:sys tar -cf '$posixTar' -C Unixlike . 2>/dev/null" | Out-Null
if (-not (Test-Path (Join-Path $payload "_u.tar"))) { throw "tar pack failed: no _u.tar" }
# extract: tar restores the text symlinks as-is (plain files, System attr).
New-Item -ItemType Directory -Force -Path (Join-Path $payload "Unixlike") | Out-Null
& $bash -lc "export PATH=/bin:/usr/bin; cd '$posixOut/Unixlike' && CYGWIN=winsymlinks:sys tar -xf '$posixTar'" 2>&1 | Out-Null
Remove-Item (Join-Path $payload "_u.tar") -Force

# add Config / Binary / root bats / exe
#   - Binary: no NSIS under it anymore - the installer builder is dev-only and
#     lives in .github\Source\NSIS (never copied into the payload)
#   - Config: exclude anything under .temp / .release / stage (never ship
#     build scratch dirs)
Copy-Item (Join-Path $root "Config")  (Join-Path $payload "Config")  -Recurse
Copy-Item (Join-Path $root "Binary")  (Join-Path $payload "Binary")  -Recurse
Remove-Item (Join-Path $payload "Config\.temp") -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $payload "Config\.release") -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $payload "Config\stage") -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $payload "Config\KmpCache") -Recurse -Force -ErrorAction SilentlyContinue
# AIShell* files carry per-user secrets, settings and command history
# (AIShell.ini / AIShell.history / ...) - strip them all from the release
# payload only; the live environment keeps its own copies untouched.
Get-ChildItem (Join-Path $payload "Config") -Filter "AIShell*" -File -ErrorAction SilentlyContinue |
    Remove-Item -Force -ErrorAction SilentlyContinue
# ensure FirstRun=yes in the shipped Kernel.ini so a fresh install triggers
# the welcome guide (kmd-welcome) on first launch; kInitrd rewrites it to
# FirstRun=no after the guide completes.
$payloadKernel = Join-Path $payload "Config\Kernel.ini"
if (Test-Path $payloadKernel) {
    $kcontent = Get-Content $payloadKernel
    $kcontent = $kcontent | ForEach-Object {
        if ($_ -match "^FirstRun=") { "FirstRun=yes" } else { $_ }
    }
    if ($kcontent -notcontains "FirstRun=yes") { $kcontent += "FirstRun=yes" }
    Set-Content $payloadKernel $kcontent -Encoding ASCII
}
# built-in Init.d keeps the loaders of CORE modules - those whose Module dir
# carries a .KmdCore marker - plus the cache-clean loader; every other module
# loader is created by kmod-setup when its kmp gets installed (user-selectable)
$coreModuleDirs = @(Get-ChildItem (Join-Path $root "Module") -Directory -Force -ErrorAction SilentlyContinue |
    Where-Object { Test-Path (Join-Path $_.FullName ".KmdCore") } |
    ForEach-Object { $_.Name })
$coreLoaders = @{}
foreach ($dir in $coreModuleDirs) {
    Get-ChildItem (Join-Path $root "Config\Init.d") -Filter *.cmd -ErrorAction SilentlyContinue |
        Where-Object { (Get-Content $_.FullName -TotalCount 1) -match "directory=$([regex]::Escape($dir))([\s]|$)" } |
        ForEach-Object { $coreLoaders[$_.Name] = $true }
}
$coreLoaders["90-z-cache-clean.cmd"] = $true   # cache-clean dispatcher is always shipped
$coreLoaders["99-cache-clean.cmd"]   = $true   # (both possible file names)
Get-ChildItem (Join-Path $payload "Config\Init.d") -Filter *.cmd |
    Where-Object { -not $coreLoaders.ContainsKey($_.Name) } |
    Remove-Item -Force
Write-Host "  core module loaders kept: $($coreLoaders.Keys -join ', ')" -ForegroundColor DarkGray

# ship the Module directories of core modules too, so the kept loaders can
# actually resolve %MOD_ROOT%\<dir> on a fresh install
if ($coreModuleDirs.Count -gt 0) {
    $payloadMod = Join-Path $payload "Module"
    New-Item -ItemType Directory -Force -Path $payloadMod | Out-Null
    foreach ($dir in $coreModuleDirs) {
        $srcMod = Join-Path $root "Module\$dir"
        if (Test-Path $srcMod) {
            Write-Host "  core module dir: $dir" -ForegroundColor DarkGray
            Copy-Item $srcMod (Join-Path $payloadMod $dir) -Recurse -Force
        }
    }
}
foreach ($f in @("kBash.bat","kCmd.bat","kInitrd.cmd","kmd.exe","kRun.exe","icon.ico","README.md","README.CN.md")) {
    $src = Join-Path $root $f
    if (Test-Path $src) { Copy-Item $src (Join-Path $payload $f) -Force }
}
# bundle Source/logo.png (README header icon)
$srcLogo = Join-Path $root "Source\logo.png"
if (Test-Path $srcLogo) {
    New-Item -ItemType Directory -Force -Path (Join-Path $payload "Source") | Out-Null
    Copy-Item $srcLogo (Join-Path $payload "Source\logo.png") -Force
}
# bare core install can unpack .kmp packages with correct ACLs - the busybox
# unzip fallback in kmod-setup corrupts ACLs (NULL SID deny entries) and should
# only ever be a last resort.
Copy-Item (Join-Path $root "Module\unzip\unzip.exe") (Join-Path $payload "Unixlike\bin\unzip.exe") -Force
# ---- purge all history & cache from the payload ----
$unixPayload = Join-Path $payload "Unixlike"
# history dotfiles under home/ and root/
$historyPatterns = @(
    ".bash_history", ".python_history", ".node_repl_history",
    ".mysql_history", ".psql_history", ".redis-cli-history",
    ".ruby_history", ".irb_history", ".lesshst", ".viminfo",
    ".wget-hsts", ".gdb_history", ".sqlite_history", ".luahistory",
    ".php_history", ".lesshst", ".node_repl_history"
)
foreach ($homeDir in @("home", "root")) {
    $hd = Join-Path $unixPayload $homeDir
    if (Test-Path $hd) {
        # clean each user subdirectory under home/
        Get-ChildItem $hd -Directory -Force -ErrorAction SilentlyContinue | ForEach-Object {
            foreach ($pat in $historyPatterns) {
                Remove-Item (Join-Path $_.FullName $pat) -Force -ErrorAction SilentlyContinue
            }
        }
        # clean root home directly (root/ has no subdirectory)
        foreach ($pat in $historyPatterns) {
            Remove-Item (Join-Path $hd $pat) -Force -ErrorAction SilentlyContinue
        }
    }
}
# cache directories anywhere in Unixlike tree
$cacheDirNames = @(
    ".apt-cyg", ".cache", "__pycache__", ".npm", ".pnpm-store",
    ".pnpm-cache", ".pip-cache", ".playwright", ".corepack", ".gocache"
)
Get-ChildItem $unixPayload -Recurse -Directory -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -in $cacheDirNames } |
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
# log files under var/log/
$varLog = Join-Path $unixPayload "var\log"
if (Test-Path $varLog) {
    Get-ChildItem $varLog -File -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
}
Write-Host "  history & cache purged" -ForegroundColor DarkGray

# archive repack strips group/other execute bits from ACLs (observed: mode 770,
# other::---). Cygwin then refuses to exec such files (rc=127) because the
# exec'ing process is not the file owner. Restore 755 semantics on everything
# (dirs included) so the installed base is executable for all users.
$payloadUnix = $payload.ToLower() -replace '\\','/' -replace '^([a-z]):', '/mnt/$1'
& $bash -c "export PATH=/bin:/usr/bin; chmod -R u+rwx,go+rx '$payloadUnix'" 2>&1 | Out-Null
$payloadSize = (Get-ChildItem $payload -Recurse -File | Measure-Object Length -Sum).Sum
Write-Host ("  base payload: {0:N1} MB ({1} files)" -f ($payloadSize/1MB), (Get-ChildItem $payload -Recurse -File).Count)

$baseVer = (Select-String -Path (Join-Path $root "Config\Kernel.ini") -Pattern "^Version=(.+)$").Matches[0].Groups[1].Value.Trim()

# ---------------------------------------------------------------- 2. module kmps
Write-Host "`n[2/5] packing module packages ..."
$loaders = Get-ChildItem (Join-Path $root "Config\Init.d") -Filter *.cmd |
    Where-Object { (Get-Content $_.FullName -First 1) -match "name=([^\s]+)" } |
    ForEach-Object {
        $name = $Matches[1]
        $meta = Get-Content $_.FullName -First 1
        $ver  = if ($meta -match "version=([^\s]+)") { $Matches[1] } else { "1.0" }
        $dir  = if ($meta -match "directory=([^\s]+)") { $Matches[1] } else { "" }
        [pscustomobject]@{ Name=$name; Ver=$ver; Dir=$dir; Loader=$_.FullName }
    } | Where-Object { $_.Dir -and (Test-Path (Join-Path $root "Module\$($_.Dir)")) }

if ($SkipKmp) {
    Write-Host "  (skipped by -SkipKmp)"
} else {
if ($Modules.Count -gt 0) { $loaders = $loaders | Where-Object { $Modules -contains $_.Name } }

foreach ($m in $loaders) {
    Write-Host ("  export {0} v{1} ..." -f $m.Name, $m.Ver) -NoNewline
    $out = Join-Path (Join-Path $rel "kmp") "$($m.Name).kmp"
    $posixOut = $out.ToLower()  -replace '\\','/' -replace '^([a-z]):', '/mnt/$1'
    $rootU    = $root.ToLower() -replace '\\','/' -replace '^([a-z]):', '/mnt/$1'
    & $bash -c "export KMD_ROOT='$rootU' MOD_ROOT='$rootU/Module' PATH=/bin:/usr/bin:/kbin; kmod-setup export $($m.Name) $posixOut" 2>&1 | ForEach-Object { Write-Host "    $_" }
    if (-not (Test-Path $out)) { Write-Host " FAILED" -ForegroundColor Red }
    else { Write-Host (" done ({0:N1} MB)" -f ((Get-Item $out).Length/1MB)) }
}
}

# ---------------------------------------------------------------- 3. registry.kp
Write-Host "`n[3/5] generating registry.kp ..."
if ($SkipKmp) {
    Write-Host "  (skipped by /skipkmp)"
} else {
# module annotation file: look for an explicit /desc= first, then the
# repository copy (.github\Source\registry-desc.ini), then Source\.
function Resolve-ModuleDesc {
    param([hashtable]$map, [string]$section)
    if ($map.ContainsKey($section)) { return $map[$section] }
    $base = $section
    # strip the version suffix: trailing "-latest" or "-<number[.more]>" so a
    # versioned/rolling section (php-8.5.1, golang-latest) resolves to the
    # annotation key of its base module.
    if ($base -match '^(.*)-(latest|[0-9][0-9.]*)$') { $base = $Matches[1] }
    if ($map.ContainsKey($base)) { return $map[$base] }
    return $null
}
$descMap = @{}
$descSrc = $null
foreach ($cand in @(
        $(if ($opt.Desc) { $opt.Desc } else { $null }),
        $(if ($opt.Desc) { Join-Path $root $opt.Desc } else { $null }),
        (Join-Path $root ".github\Source\registry-desc.ini"),
        (Join-Path $root "Source\registry-desc.ini"))) {
    if ($cand -and (Test-Path $cand)) { $descSrc = $cand; break }
}
if ($descSrc) {
    foreach ($line in (Get-Content $descSrc -ErrorAction SilentlyContinue)) {
        $line = $line.Trim()
        if (-not $line -or $line.StartsWith('#')) { continue }
        $i = $line.IndexOf('=')
        if ($i -lt 0) { continue }
        $k = $line.Substring(0, $i).Trim()
        $v = $line.Substring($i + 1).Trim()
        if ($k) { $descMap[$k] = $v }
    }
    Write-Host "  annotations: $descSrc ($($descMap.Count) entries)" -ForegroundColor DarkGray
} else {
    Write-Host "  annotations: none (no registry-desc.ini found)" -ForegroundColor DarkGray
}
$sb = New-Object System.Text.StringBuilder
foreach ($f in (Get-ChildItem (Join-Path $rel "kmp") -Filter *.kmp)) {
    $h = (Get-FileHash $f.FullName -Algorithm SHA256).Hash.ToLower()
    $name = $f.BaseName
    $ver = ""
    & $7z e -so -bso0 $f.FullName kmp.ini 2>$null | ForEach-Object {
        if ($_ -match "^version=(.+)$") { $ver = $Matches[1].Trim() }
    }
    [void]$sb.AppendLine("[$name]")
    [void]$sb.AppendLine("name=$name")
    [void]$sb.AppendLine("version=$ver")
    [void]$sb.AppendLine("size=$($f.Length)")
    [void]$sb.AppendLine("sha256=$h")
    [void]$sb.AppendLine("directory=$name")
    # GitHub flattens release assets to the archive root (no sub-dirs), so the
    # registry must reference the bare asset name - "kmp/x.kmp" would 404.
    [void]$sb.AppendLine("file=$($f.Name)")
    $desc = Resolve-ModuleDesc $descMap $name
    if ($desc) { [void]$sb.AppendLine("description=$desc") }
    [void]$sb.AppendLine("")
}
[void]$sb.AppendLine("[global]")
[void]$sb.AppendLine("generator=build-release.ps1")
[void]$sb.AppendLine("generated=$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
$regPath = Join-Path $rel "registry.kp"
[System.IO.File]::WriteAllText($regPath, $sb.ToString())
Write-Host ("  registry.kp written ({0} packages, {1} annotations)" -f (Get-ChildItem (Join-Path $rel 'kmp') -Filter *.kmp).Count, $descMap.Count)
}

# ---------------------------------------------------------------- 4. NSIS installer
if ($SkipNSIS) { Write-Host "`n[4/5] NSIS step skipped (-SkipNSIS)"; exit 0 }
Write-Host "`n[4/5] building NSIS installer ..."
$nsi = Join-Path $stage "installer.nsi"
$outExe = Join-Path $rel "Komandare-Setup-$baseVer.exe"
if (Test-Path $outExe) { Remove-Item $outExe -Force }

# collect file list relative to $payload for the NSIS File commands
$files = Get-ChildItem $payload -Recurse -File | ForEach-Object {
    $_.FullName.Substring($payload.Length + 1)
}
# normalize to forward slashes for NSIS
$files = $files | ForEach-Object { $_ -replace '\\','/' }

# group File commands by directory: NSIS File's target dir = last SetOutPath,
# and /oname alone does NOT create subdirs, so emit SetOutPath per directory.
$inc = New-Object System.Text.StringBuilder
$files | Group-Object { if ($_.Contains('/')) { Split-Path $_ -Parent } else { '.' } } |
    Sort-Object Name |
    ForEach-Object {
        $dir = $_.Name
        if ($dir -eq '.') {
            [void]$inc.AppendLine("    SetOutPath `"`$INSTDIR`"")
        } else {
            [void]$inc.AppendLine("    SetOutPath `"`$INSTDIR\$($dir -replace '/','\')`"")
        }
        foreach ($f in ($_.Group | Sort-Object)) {
            $oname = Split-Path $f -Leaf
            $src   = Join-Path $payload $f
            [void]$inc.AppendLine("    File `"/oname=$oname`" `"$src`"")
        }
    }
$incContent = $inc.ToString()

# detect Cygwin text symlinks: these are plain files carrying the System (+S)
# attribute. NSIS File copies them but strips +S, so Cygwin no longer
# recognizes them as symlinks. We emit SetFileAttributes SYSTEM for each
# so the installed tree matches the payload.
$sysFiles = @(Get-ChildItem $payload -Recurse -File -Force |
    Where-Object { $_.Attributes.HasFlag([System.IO.FileAttributes]::System) } |
    ForEach-Object { $_.FullName.Substring($payload.Length + 1) -replace '\\','/' } |
    Sort-Object)
$sysAttr = New-Object System.Text.StringBuilder
if ($sysFiles.Count -gt 0) {
    foreach ($f in $sysFiles) {
        $winPath = $f -replace '/','\'
        [void]$sysAttr.AppendLine('    SetFileAttributes "$INSTDIR\' + $winPath + '" SYSTEM')
    }
    Write-Host "  $($sysFiles.Count) text symlinks: +S attribute restoration queued" -ForegroundColor DarkGray
}
$sysAttrContent = $sysAttr.ToString()

# Modules are NOT embedded and NOT shown as installer components. After a core
# install the user picks modules via the first-run guide (wguide) or
# `kmod-setup install <name> -r`. This keeps the components page to Core + PATH.
$outExeU  = $outExe  -replace '\\','/'
$iconU    = (Join-Path $root 'icon.ico') -replace '\\','/'

$nsiContent = @"
; Komandare NSIS installer script
; Generated by build-release.ps1 - do not edit
Unicode true
!include "MUI2.nsh"
!include "StrFunc.nsh"
`${StrStr}
`${UnStrStr}

!ifndef HWND_BROADCAST
!define HWND_BROADCAST 0xFFFF
!endif
!ifndef WM_WININICHANGE
!define WM_WININICHANGE 0x001A
!endif

Name "Komandare"
OutFile "$outExeU"
InstallDir "C:\Komandare"
InstallDirRegKey HKCU "Software\Komandare" ""
RequestExecutionLevel user
SetCompressor /SOLID lzma

!define MUI_ABORTWARNING
!define MUI_ICON "$iconU"
!define MUI_UNICON "$iconU"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"

; ---- Core (required, always installed, cannot be deselected) ----
Section "-Core" SEC01
    SetOutPath "`$INSTDIR"
    $incContent

    ; restore System (+S) attribute on Cygwin text symlinks (NSIS File
    ; strips +S; without it Cygwin won't recognize text symlinks)
$sysAttrContent

    ; start menu + desktop shortcuts (target: kmd.exe recommended launcher).
    ; NSIS CreateShortcut has no working-dir parameter; use /NoWorkingDir since
    ; kmd.exe launches its own shell (cwd is decided by bash, not the shortcut).
    CreateShortcut /NoWorkingDir "`$SMPROGRAMS\Komandare\Komandare.lnk" "`$INSTDIR\kmd.exe" "" "`$INSTDIR\icon.ico"
    CreateShortcut /NoWorkingDir "`$DESKTOP\Komandare.lnk" "`$INSTDIR\kmd.exe" "" "`$INSTDIR\icon.ico"
    WriteRegStr HKCU "Software\Komandare" "" "`$INSTDIR"
    WriteUninstaller "`$INSTDIR\uninstall.exe"
SectionEnd

; ---- Add to PATH (optional, selected by default) ----
Section "Add to PATH" SEC02
    ReadRegStr `$0 HKCU "Environment" "Path"
    `${StrStr} `$1 `$0 "`$INSTDIR"
    StrCmp `$1 "" 0 path_done
    StrCmp `$0 "" path_empty path_nonempty
    path_empty:
    StrCpy `$0 "`$INSTDIR"
    Goto path_write
    path_nonempty:
    StrCpy `$0 "`$0;`$INSTDIR"
    path_write:
    WriteRegExpandStr HKCU "Environment" "Path" "`$0"
    SendMessage `${HWND_BROADCAST} `${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=50
    path_done:
SectionEnd

Section "Uninstall"
    ; remove the PATH entry we added (only if present and we own the tail)
    ReadRegStr `$0 HKCU "Environment" "Path"
    StrCmp `$0 "`$INSTDIR" 0 up_not_solo
    DeleteRegValue HKCU "Environment" "Path"
    Goto up_path_done
    up_not_solo:
    StrCmp `$0 "" up_path_done
    StrLen `$2 `$0
    StrLen `$3 ";`$INSTDIR"
    `${UnStrStr} `$1 `$0 ";`$INSTDIR"
    StrCmp `$1 "" up_path_done
    IntOp `$5 `$2 - `$3
    StrCmp `$1 "`$5" 0 up_path_done
    StrCpy `$0 `$0 `$5
    WriteRegExpandStr HKCU "Environment" "Path" "`$0"
    up_path_done:
    RMDir /r "`$SMPROGRAMS\Komandare"
    Delete "`$DESKTOP\Komandare.lnk"
    Delete "`$INSTDIR\uninstall.exe"
    DeleteRegKey HKCU "Software\Komandare"
    RMDir /r "`$INSTDIR"
SectionEnd
"@
[System.IO.File]::WriteAllText($nsi, $nsiContent)
Write-Host "  NSIS script written ($($files.Count) files)"

& $nsis $nsi
if ($LASTEXITCODE -ne 0) { throw "makensis failed (rc=$LASTEXITCODE)" }
Write-Host ("  installer: {0:N1} MB" -f ((Get-Item $outExe).Length/1MB))

# ---------------------------------------------------------------- 5. summary
Write-Host "`n[5/5] release summary ==" -ForegroundColor Cyan
Get-ChildItem $rel -File | ForEach-Object {
    Write-Host ("  {0,-28} {1,10:N1} MB" -f $_.Name, ($_.Length/1MB))
}
Get-ChildItem (Join-Path $rel "kmp") -Filter *.kmp | ForEach-Object {
    Write-Host ("  kmp\{0,-23} {1,10:N1} MB" -f $_.Name, ($_.Length/1MB))
}
Write-Host "`ndone."
