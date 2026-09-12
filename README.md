<h1 align="center">Komandare</h1>

<p align="center">
  <img src="./Source/logo.png" alt="Komandare logo" width="120" />
</p>

<p align="center">
  A UserLand Linux distribution for Windows, built on the Cygwin kernel — one folder, full Unix userland, complete dev toolchain, zero traces left on the host.
</p>

<p align="center">
  简体中文说明请阅读 <a href="./README.CN.md">README.CN.md</a>
</p>

<p align="center">
  <a href="https://github.com/xRetia/Komandare/releases">
    <img src="https://badgen.net/github/tag/xRetia/Komandare?label=Latest%20Version&color=blueviolet" alt="Latest Version" />
  </a>
  <a href="https://github.com/xRetia/Komandare/releases">
    <img src="https://badgen.net/github/release/xRetia/Komandare?label=Download&color=blue" alt="Download" />
  </a>
  <a href="https://komandare.github.io/">
    <img src="https://img.shields.io/badge/homepage-komandare.github.io-01E3F8.svg" alt="Homepage" />
  </a>
</p>

---

- **Vendor**: xRetia Labs
- **Homepage**: <https://komandare.github.io/>
- **Repository**: <https://github.com/xRetia/Komandare>

---

## The Shell

Every Komandare session starts with the kmd banner (editable at `Config/Banner.ini`, printed on every `bash` / `cmd` / `powershell` entry):

```
Welcome to Komandare Shell!
Copyright(c) xRetia Labs.
```

Followed by `ver` — neofetch plus the distro version:

```
    __ __                                __
   / //_/___  ____ ___  ____ _____  ____/ /___ _________
  / ,< / __ \/ __ `__ \/ __ `/ __ \/ __  / __ `/ ___/ _ \
 / /| / /_/ / / / / / / /_/ / / / / /_/ / /_/ / /  /  __/
/_/ |_\____/_/ /_/ /_/\__,_/_/ /_/\__,_/\__,_/_/   \___/

------------------------------------------------------------

User             : demo@MYPC
OS               : Komandare
System Version   : 4.2.2026.0911
Kernel           : CYGWIN 3.6.6
Shell            : Komandare Shell
WCmdBox          : 20.08.1
Unique Libs      : /kbin [aishell, adb, apt-cyg, attrib, binwalk, dir, fastboot, jefferson, killall, kmd-welcome, kmod-setup, mklink, mkrootfs, neofetch, phptest, poweroff, ps, reboot, reset, runas_root, runas_user, service, su, sudo, top, unroot, ver, version, vihost, wcmdbox]
Project          : https://github.com/xRetia/Komandare

------------------------------------------------------------
Welcome to Komandare!
Komandare Version: 4.2.2026.0911
Copyright (c)2026 xRetia Labs
```

---

## What makes Komandare different

**A real distro, not a wrapper.** Komandare is a UserLand Linux distribution whose kernel is Cygwin and whose userland is curated for developers: full Unix commands (`bash`, `vim`, `git`, `curl`, `gcc`-class toolchains, package management via `apt-cyg`), organized as portable *modules* instead of a single monolithic install.

**Zero footprint on the host.** This is the core promise. `python3`, `nodejs`, `golang`, `ruby`, `gradle`, `php`, `npm`/`pnpm`/`pip`/`playwright` — **nothing** is ever created under `%APPDATA%`, `%LOCALAPPDATA%` or any user-profile directory. Global prefixes, package caches, stores, Go module cache, Gradle user home, Ruby gems — all of them live *inside* the Komandare tree (`Module/...`). Delete the folder and the host is exactly as it was.

**kRun — run any IDE inside the kmd environment.** `kRun.exe` performs the same environment injection as a normal shell boot (PATH, module vars, caches) and then launches any third-party program in it. Open VSCode / any IDE / any dev tool with the full Komandare toolchain visible:

```bat
kRun code .
```

Run `kRun` with no arguments and it pops a Windows-style Run dialog (bottom-left, with the Komandare icon) — type any command there and it launches with the same full environment. The binary is built from `Source/kRun.c` (C, zero-CRT native PE) via `Source/build_krun.ps1`.

**kCmd / kBash — kmd environment in any other terminal.** Already sitting in PowerShell, Windows Terminal, ConEmu or CMD? `kCmd` and `kBash` hand you a fully booted Komandare shell right there, no new window:

```bat
kCmd            :: enter cmd with the kmd environment
kBash           :: enter bash with the kmd environment
```

**kmd — the recommended launcher.** Double-click `kmd.exe` and get a proper Komandare terminal: it prefers Windows Terminal (≥ 1.22), falls back to the bundled ConEmu build, and — as a nice touch — injects a WT *profile fragment* so that dragging files into the terminal converts paths to `/mnt/c/...` automatically, without touching your WT settings.json.

**AI Shell — natural-language to shell commands.** `aishell` translates plain-English (or Chinese) descriptions into shell commands via an OpenAI-compatible API. Describe what you want and get a suggested command with an optional `RUN` confirmation before execution. Configure the API endpoint and model in `Config/AIShell.ini`:

```bash
aishell "find all log files modified in the last 7 days and sort by size"
```

---

## Quick Start

### 1. Launch a full Komandare terminal

Double-click `kmd.exe` (or pin it to the taskbar / Start Menu).

1. It detects Windows Terminal (version ≥ `MinVersion` in `Config/Terminal.ini`, default 1.60);
2. if present → injects the `pathTranslationStyle` JSON Fragment and runs `wt -d <cwd> cmd /k kInitrd.cmd bash`;
3. otherwise → falls back to `Console/kmd64.exe` (ConEmu), then `Console/kmd32.exe`.

The new terminal opens in the directory you invoked `kmd.exe` from (override with `StartDirectory` in `Terminal.ini`).

> `kmd.exe` is a zero-dependency native PE (no CRT). It launches and exits, leaving no background process.

### 2. First run: the module wizard

On the very first boot, kInitrd launches **kmd-welcome**, a full-screen TUI setup wizard (dialog-based, Windows XP text-setup style) that walks you through selecting and installing modules — or you can skip it and manage modules later with `kmod-setup` (see below).

### 3. Enter the environment from any other terminal

```bat
kBash          :: Komandare bash in the current window
kCmd           :: Komandare cmd in the current window
```

Both are thin wrappers around the core entry point:

```bat
kInitrd.cmd bash            :: same as kbash
kInitrd.cmd cmd             :: same as kcmd
kInitrd.cmd bash -c "ls"    :: boot the environment and run any command
```

### 4. Run a third-party program with the kmd environment

```bat
kRun code .               :: launch VSCode with the full Komandare toolchain
kRun <anything.exe> <args> :: same for any editor / IDE / debugger / tool
```

`kRun` performs the identical environment injection as a shell boot (modules, PATH, cache vars, junctions) before starting the target program — the program sees a complete Unix-like dev environment without ever being installed on the host.

---

## Boot sequence — kInitrd

`kInitrd.cmd` is the kernel entry point of the distro: every launch path (kmd / Console/kmd64 / Console/kmd32 / kCmd / kBash) converges here. It boots the system with a single-line progress display (`Loading xxx ...`):

1. **binaries** — put `Binary/` on PATH;
2. **Cygwin** — put `Unixlike/kbin`, `bin`, `sbin`, `usr/bin`, `usr/sbin` on PATH;
3. **filesystem** — rebuild `Unixlike/tmp` as a text symlink to the real Windows `%TEMP%`, and the `/kmod` + `/kconf` kernel paths (absolute, mount-derived — the tree survives being moved to another drive);
4. **modules** — run every `Config/Init.d/NN-*.cmd` in numeric order (00-99), each registering one toolchain module;
5. **desktop** — probe the current user's Desktop from the registry into `%DESKTOP%`;
6. **aliases** — load doskey macros from `SystemAliases.ini` + `UserAliases.ini`;
7. **locale** — `LANG=zh_CN.UTF-8`, `chcp 65001`;
8. **identity** — remap `/etc/passwd` based on elevated status (`runas_root` / `runas_user`);
9. **first run** — launch `kmd-welcome` wizard unless disabled in `Kernel.ini`;
10. **banner** — print `Config/Banner.ini`, then exec the requested shell.

After boot, the environment variables `KMD_ROOT` (distro root), `MOD_ROOT`, `BIN_ROOT`, `UNIX_ROOT`, `DESKTOP` are all set, and every script addresses the tree relative to `KMD_ROOT` — so the entire folder can be relocated freely.

---

## Layout

| Path | Role |
| --- | --- |
| `kmd.exe` | Recommended launcher (Windows Terminal first, ConEmu fallback) |
| `kRun.exe` | Run any third-party program inside the kmd environment |
| `kBash.bat` / `kCmd.bat` | Enter the kmd environment from any other terminal |
| `kInitrd.cmd` | Kernel entry point — the boot loader every launcher converges to |
| `Config/` | Launcher config, module registry (`Init.d/`), aliases, cache policy, services |
| `Module/` | Toolchain packages, one folder per app, registered by `Config/Init.d/*.cmd` |
| `Binary/` | Portable standalone tools & wrappers (caddy, mitmproxy, cygwin-setup, ...) |
| `Unixlike/` | The Cygwin userland: `bin/`, `usr/`, `kbin/` (Komandare built-in commands), `etc/`, `home/` |

---

## Binary — portable standalone tools

Everything in `Binary/` is green and put on PATH automatically at boot. Besides the official `cygwin-setup`, it bundles the everyday Windows networking / dev helpers:

| Tool | What it does |
| --- | --- |
| `caddy.exe` | lightweight HTTP / reverse-proxy server |
| `mitmproxy` / `mitmdump` / `mitmweb` | interactive HTTPS interception & analysis |
| `plink.exe` / `scp.exe` / `sftp.exe` | PuTTY SSH CLI clients (sftp/scp) |
| `tcping.exe` | TCP ping — check if a port accepts connections |
| `RawCap.exe` | capture raw traffic on a local interface to a pcap file |
| `streams.exe` / `streams64.exe` | strip NTFS Alternate Data Streams (Mark-of-the-Web) |
| `cygwin-setup.exe` | the official Cygwin `setup-x86_64.exe` (full package manager) |
| `composer.bat` (+ `.phar`) | PHP dependency manager (`composer` on PATH) |
| `phpunit.bat` (+ `.phar`) | PHPUnit test runner |
| `binwalk.bat` / `jefferson.bat` | firmware extraction & file-system analysis (front-ends for the Python tools in `Module/`) |
| `desktopini.bat` | interactively set a folder's display name / icon via `desktop.ini` |
| `pptp-dial.bat` (+ `pptp.pbk`) | dial / disconnect a PPTP VPN (`dial <addr> <user> <pass>` / `disconnect`) |
| `MoveLater.exe` | defer a file deletion to the next reboot |
| `kmd_reset.cmd` | reset the ConEmu tab title to "Komandare" |

---

## Toolchains — no host pollution

Every module pins its state **inside `Module/<name>/`**. Nothing is written under `%APPDATA%` / `%LOCALAPPDATA%` / `%USERPROFILE%`:

| Toolchain | What is contained |
| --- | --- |
| **Node.js 22** | `npm` cache & prefix, `pnpm` store & cache, `corepack` — plus a self-healing `%APPDATA%\npm` junction into `Module/nodejs/packages` |
| **Python 3.12** | `pip` cache, Playwright browsers, user-site — `PIP_CACHE_DIR`, `PLAYWRIGHT_BROWSERS_PATH`, `PYTHONUSERBASE` all inside `Module/python3x` |
| **Go** | `GOPATH`, `GOMODCACHE`, `GOCACHE`, `GOENV` inside `Module/golang` |
| **Ruby 4** | `GEM_HOME` / `GEM_PATH`, SSL certs inside `Module/ruby4` |
| **Gradle** | `GRADLE_USER_HOME` inside `Module/gradle` |
| **PHP 8.3** | runtime inside `Module/php` |

Cache self-maintenance is handled at every boot by the `z-cache-clean` module: directories are created on demand, sizes are computed against `Config/CacheClean.ini` (defaults: npm 512 MB, pnpm-store 1 GB, pnpm-cache 256 MB, pip 1 GB, playwright 2 GB), and you're asked before anything is cleaned. Move the whole tree to another machine — caches move with it.

### Module catalog

Each `Module/<name>/` is one self-contained toolchain package, registered by `Config/Init.d/*.cmd`:

| Module | What it provides |
| --- | --- |
| `7zip` | 7-Zip archiving (`7z`, `7za`) |
| `aishell` | AI-powered natural-language → shell command translator |
| `adb` / `fastboot` | Android device bridge & bootloader tools |
| `dig` | DNS lookup utility |
| `git` | Git (prefers Git for Windows, falls back to bundled Cygwin git) |
| `golang` | Go toolchain (`go`, `gofmt`) |
| `gradle` | Gradle build system |
| `gsudo` | UAC-elevated `sudo` for Windows |
| `iperf3` | network throughput benchmark |
| `lessc` | LESS CSS precompiler |
| `nodejs` | Node.js 22 + `npm` / `pnpm` / `corepack` |
| `ntr` | network traffic relay tool |
| `php` | PHP 8.3 runtime |
| `pstop` | process viewer / manager |
| `python3x` | Python 3.12 + `pip` / Playwright |
| `qemu` | QEMU machine emulator |
| `ruby4` | Ruby 4 runtime (`GEM_HOME` inside) |
| `ruby-2.1.7` | legacy Ruby 2.1.7 runtime |
| `selfsign-ssl` | local self-signed SSL certificate generator |
| `socat` | bidirectional data relay / socket tools |
| `squashfs-tools` | `mksquashfs` / `unsquashfs` filesystem tools |
| `unzip` | ZIP extraction |
| `vbox-helper` | VirtualBox headless helper (Docker VM support) |
| `vbox` | VirtualBox runtime |
| `windows-driver-sign` | sign Windows drivers (WHQL / self-signed) |
| `z-cache-clean` | cache self-maintenance at boot (see above) |

---

## kmod-setup — module package manager

Modules are managed by `kmod-setup` (a built-in `/kbin` command, **v4.3.0**):

```bash
kmod-setup list                  # list registered modules
kmod-setup list -r               # compare with remote registry versions
kmod-setup install <dir> [priority 00-99] [name] [version] [publisher]
kmod-setup install <pkg.kmp>     # install from a local package
kmod-setup install -r <name>     # download & install from the remote registry
kmod-setup sync                  # refresh the remote registry cache
kmod-setup update [name]         # check / install remote updates
kmod-setup mirror [url]          # show / set download mirror (default GitHub releases)
kmod-setup export <name> [out.kmp]
kmod-setup remove <name>         # unregister a module (keeps Module/<dir>)
```

- `install <dir>` registers an existing `Module/<dir>` by generating its `Config/Init.d/NN-name.cmd` loader;
- `.kmp` packages are plain zips (`kmp.ini` + `loader.cmd` + `module/`) — hand-craftable, sha256-verified when pulled remotely;
- `mirror` switches the download base (default: `github.com/xRetia/komandare-mod-pkgs` latest release) — handy for intranet/offline environments.

**v4.3.0 compatibility highlights:**
- **registry.kp v2 schema** — `[global] kmpbase=` / `scriptbase=` base-URL overrides; relative paths resolve against the mirror, absolute URLs pass through.
- **Multi-mirror support** — every `mirror=` line in `Config/Registry.ini` is tried in order.
- **Root-layout packages** — `root=yes` in `kmp.ini` installs directly into `KMD_ROOT` with zero Cygwin forks (avoids `dll data read copy failed` under memory pressure).
- **`-latest` conflict detection** — before installing a `-latest` package, prompts to remove older same-tool versions.
- **Install hooks** — `.sh` / `.ps1` / `.cmd` hooks dispatched by extension; failed hooks wipe the payload and abort.
- **Dependency support** — `depends=` in kmp.ini / registry, cycle detection, isolated subshell installs.
- **Offline-tolerant registry refresh** — TTL-based cache (default 600 s), falls back to stale cache when offline.
- **GitHub release-asset flattening** — retries bare basename if `file=kmp/x.kmp` returns 404.
- **Busybox unzip fallback** — core install can still unpack `.kmp` files without full toolchain.
- **`.KmdCore` protection** — `remove` refuses to delete system modules.

Bundled modules today: 7-Zip, aishell, adb, dig, git (prefers Git for Windows, falls back to the bundled Cygwin git), golang, gradle, gsudo, iperf3, lessc, nodejs, ntr, php, pstop, python3x, qemu, ruby4, selfsign-ssl, socat, squashfs-tools, unzip, vbox, vbox-helper, windows-driver-sign, z-cache-clean.

---

## kbin — built-in commands

`/kbin` is the command layer Komandare layers on top of Cygwin (takes precedence over the regular `bin`):

| Command | What it does |
| --- | --- |
| `su` / `nosu` | Trigger a Windows UAC elevation that opens a new root `kmd` window (root without password); `nosu` / `unroot` restore the normal-user mapping |
| `sudo` | run a command as root via `gsudo` UAC elevation, auto-restoring the passwd mapping afterwards |
| `ver` / `version` | neofetch + distro version (reads `Config/Kernel.ini`); `-s` prints just the version |
| `kmod-setup` | module system package manager |
| `kmd-welcome` | first-run setup wizard (full-screen TUI, installs modules) |
| `mkrootfs` | rebuild the `kmod` / `kconf` / `tmp` text-symlinks (mount-table derived, idempotent) |
| `neofetch` | system info display |
| `ps` / `top` / `killall` | process observation (Windows-aware) |
| `ifconfig` / `hwinfo` | network & hardware info via wcmdbox/mobabox |
| `adb` / `fastboot` | Android device operations |
| `apt-cyg` | Cygwin package manager (aliases: `apt`, `apt-get`, `yum`, `apk`) |
| `binwalk` / `jefferson` | firmware analysis |
| `phptest` | quick PHP scratchpad: opens a temp `.php` in `nano`, then runs it |
| `wcmdbox` / `mobabox` | Komandare tool boxes (run Windows tools from Unix paths) |
| `service` | service manager dispatcher — resolves `Config/Service/<name>.bat` and executes actions |
| `vihost` | opens `/etc/hosts` in `nano` with root privileges (requires `sudo`) |
| `poweroff` / `reboot` / `reset` | quick system actions |
| `attrib` / `dir` / `mklink` | Windows filesystem commands bridged through wcmdbox |

For full Cygwin package management run `cygwin-setup` (the official `setup-x86_64.exe`, bundled in `Binary/`); for day-to-day installs just use `apt`.

---

## Aliases

doskey macros from `SystemAliases.ini` (system) + `UserAliases.ini` (yours). Highlights:

| Alias | Expands to | Purpose |
| --- | --- | --- |
| `apt` / `apt-get` / `yum` / `apk` | `bash apt-cyg $*` | install Cygwin packages with a package-manager habit |
| `su` / `nosu` | UAC elevation / revert | root without password |
| `sudo` | `gsudo $*` | run a command with Windows UAC elevation |
| `ver` / `version` | `sh /kbin/version` | neofetch + version |
| `d` / `desktop` | `cd $DESKTOP` | jump to desktop |
| `webd` | `python3 -m http.server` | serve current dir |
| `e.` | `explorer .` | open folder in Explorer |
| `np` | `notepad $*` | open files in Notepad |
| `vi` | `vim $*` | open files in Vim |
| `gl` / `gm` | git log graph / add-commit-push | git one-liners |
| `ifconfig` / `hwinfo` | mobabox bridge | network / hardware info |
| `mtr` | `ntr $*` | route tracing |
| `clear` / `cls` | clear the screen | reset the terminal |
| `ls` / `ll` / `la` | colorized, auto-hides NTUSER leftovers | directory listing |

---

## Services

`service` (a built-in `/kbin` command) is a tiny service manager that resolves `Config/Service/<name>.bat` and executes actions:

```bat
service docker start       :: start the Docker VM (VirtualBox headless)
service docker stop        :: suspend
service docker force-stop  :: power off
service docker reset       :: restore to Factory snapshot
service docker install     :: redeploy the VM from vm.7z
service docker opendir     :: open the VM's Samba share
```

Drop a `Config/Service/<name>.bat` to add your own service.

---

## Portability

- **Path anchoring** — everything addresses itself relative to `KMD_ROOT` (derived from `kInitrd.cmd`'s real location); the tree can be moved anywhere, even to another drive;
- **Self-healing junctions** — `%APPDATA%\npm` is re-pointed into `Module/nodejs/packages` at every boot;
- **Cache containment** — npm / pnpm / pip / playwright / Go caches stay inside `Module/`, never in user directories;
- **Text symlinks** — `Unixlike/kmod`, `/kconf` and `tmp` are plain text symlinks rebuilt from the mount table at boot, so 7z/NSIS/tar copies survive byte-exact with no admin rights;
- **WT profile injection** — Windows Terminal is customized through a standard Fragment only; set `UseWindowsTerminal=no` or delete the fragment to detach at any time.

### Packaging & permissions

ZIP/tar/NSIS round-trips can strip POSIX group/other execute bits (symptom: `rc=127` in Cygwin). Two chokepoints re-apply them: `Source/build-release.ps1` (before building the installer) and `kmod-setup install` (after unpacking a `.kmp`). If you hand-copy the tree and hit `rc=127`, `chmod +x` the file in bash.

---

## FAQ

**Q: kmd says "no Windows Terminal available and the fallback also failed"?**
`Console/kmd64.exe` / `kmd32.exe` were moved or renamed — check `Fallback64` / `Fallback32` in `Terminal.ini`.

**Q: How do I force ConEmu instead of WT?**
Set `UseWindowsTerminal=no` in `Terminal.ini`, or launch `Console/kmd64.exe` directly.

**Q: Chinese text looks garbled in my terminal?**
Komandare switches to UTF-8 (`chcp 65001`) automatically at boot. If you entered via `kBash`/`kCmd` from an external host, make sure the host terminal's font and encoding are UTF-8; for PowerShell consider Windows Terminal or ConEmu.

**Q: How do I add my own tool?**
Portable green programs: drop into `Binary/` (auto-on-PATH). Anything needing env setup / caches: drop into `Module/` and `kmod-setup install` it (or hand-write `Config/Init.d/NN-name.cmd`). Pure aliases: `Config/UserAliases.ini`.

---

*Copyright (c) 2020 - 2026 xRetia Labs. All rights reserved.*