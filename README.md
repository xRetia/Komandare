<h1 align="center">Komandare</h1>

<p align="center"><img src="./Source/logo.png" alt="Komandare logo" width="120" /></p>

<p align="center"><strong>A portable Windows development sandbox.</strong><br>Bring a complete Cygwin + Bash toolchain to your IDE with <code>kRun</code> — without installing every tool into Windows.</p>

<p align="center">简体中文说明：<a href="./README.CN.md">README.CN.md</a></p>

<p align="center"><a href="https://github.com/xRetia/Komandare/releases"><img src="https://badgen.net/github/tag/xRetia/Komandare?label=Latest%20Version&color=blueviolet" alt="Latest Version" /></a> <a href="https://github.com/xRetia/Komandare/releases"><img src="https://badgen.net/github/release/xRetia/Komandare?label=Download&color=blue" alt="Download" /></a></p>

---

## What is Komandare?

Komandare is a **portable development environment for Windows**. It packages a Cygwin-based Unix userland, Bash, common command-line tools, and optional language toolchains into one relocatable directory. It is best understood as a **sandbox for your development environment**, not as another terminal emulator or a replacement for Windows itself.

The key idea is simple: **the environment is injected into the program you want to use**. Start an IDE, debugger, build tool, or script through `kRun`, and Komandare boots its modules first, then gives the target process the resulting `PATH`, toolchain variables, cache locations, and junctions.

```bat
kRun code .
kRun "C:\Program Files\JetBrains\Toolbox\bin\jetbrains-toolbox.exe"
kRun powershell -NoExit
```

Your IDE does not need a separate copy of every compiler, interpreter, package manager, or Unix utility. It sees the same environment as a Komandare shell.

## Why use it?

| Problem | Komandare's answer |
| --- | --- |
| Every project needs a different toolchain | Install and enable toolchains as portable `Module/` packages. |
| IDEs cannot see tools configured in another shell | Launch the IDE with `kRun`; the shell environment is injected before startup. |
| Reinstalling Windows means rebuilding development setup | Keep or copy the Komandare directory and restore the environment from that directory. |
| Package caches and global prefixes spread through the user profile | npm, pnpm, pip, Playwright, Go, Gradle, and Ruby state is redirected into `Module/`. |
| You need Unix commands on Windows | Use Cygwin + Bash, `/kbin`, `apt-cyg`, and the bundled Windows bridges. |
| You need help remembering shell commands | Use the optional `aishell` module to turn natural language into a reviewed command. |

> **One folder is the unit of portability.** The launcher derives `KMD_ROOT` from its own location, so the tree can be moved to another directory, drive, machine, or backup without rebuilding paths by hand.

## The three entry points

### 1. `kRun`: inject the environment into an IDE or tool

This is the most important Komandare workflow. `kRun.exe` starts the normal `kInitrd.cmd` boot chain and passes the command to it. The boot chain adds Cygwin and `Binary/` to `PATH`, loads registered modules in `Config/Init.d/`, sets cache and toolchain variables, repairs portable junctions, and then starts the target program.

```bat
kRun code .                 :: VS Code sees Node, Python, Git, Bash, etc.
kRun devenv project.sln     :: start another Windows development tool
kRun my-build.cmd           :: run a project script in the same environment
```

Run `kRun.exe` without arguments to open a small native Windows Run-style dialog. Use `--console` when a target should always receive a visible console window. `kRun` is implemented in `Source/kRun.c` as a zero-CRT native PE.

### 2. `kmd`: start the complete shell

Double-click `kmd.exe` to open a ready-to-use Komandare terminal. It prefers Windows Terminal, falls back to the bundled console, and can inject a Windows Terminal profile fragment for path translation. `kmd.exe` launches and exits; it does not remain as a background daemon.

```bat
kmd.exe
```

On first boot, `kmd-welcome` can install the modules you select. You can skip the wizard and manage modules later.

### 3. `kBash` / `kCmd`: enter from a terminal you already use

```bat
kBash                         :: Bash with the Komandare environment
kCmd                          :: cmd with the Komandare environment
kInitrd.cmd bash -c "git status"
```

All launchers converge on `kInitrd.cmd`, so a shell and an IDE launched with `kRun` receive the same environment.

## Install and first run

1. Download **[Komandare-Setup-5.2.2026.0912.exe](https://github.com/xRetia/Komandare/releases/tag/5.2.2026.0912)** from the 5.2 release.
2. Install it to a directory you control. The base runtime is included; optional modules are fetched after installation.
3. Start `kmd.exe` once and choose modules in `kmd-welcome`, or install them explicitly:

```bash
kmod-setup sync
kmod-setup install -r git
kmod-setup install -r python3x
kmod-setup install -r nodejs
```

4. Open your project from that environment or launch its IDE through `kRun`:

```bat
cd C:\work\my-project
kRun code .
```

The installer ships the base runtime. Module packs (`.kmp`) and `registry.kp` are published separately through Releases and are downloaded by `kmod-setup`; modules are not embedded in the installer.

## What gets contained

Komandare does not promise that Windows APIs or third-party applications can never write outside the tree. Its promise is narrower and practical: **the managed development toolchains keep their state in the sandbox**.

| Toolchain | Contained state |
| --- | --- |
| Node.js 22 | npm prefix/cache, pnpm store/cache, corepack |
| Python 3.12 | pip cache, Playwright browsers, user site |
| Go | `GOPATH`, `GOMODCACHE`, `GOCACHE`, `GOENV` |
| Ruby 4 | gems and SSL certificates |
| Gradle | `GRADLE_USER_HOME` |
| PHP 8.3 | runtime and Composer tooling |

`z-cache-clean` can maintain cache sizes using `Config/CacheClean.ini`. Copying the directory carries the configured toolchains and their caches with it. Deleting the directory removes the Komandare-managed state; Windows itself and programs you launch remain subject to their own behavior.

## Shell and built-in tools

Komandare includes Cygwin userland, Bash, Vim, Git support, `curl`, `gcc`-class development tools, `apt-cyg`, and a `/kbin` command layer. Useful built-ins include:

| Command | Purpose |
| --- | --- |
| `kmod-setup` | install, update, export, remove, and mirror module packages |
| `ver` / `version` | show Komandare and system information |
| `apt-cyg` / `apt` | manage Cygwin packages |
| `sudo` / `su` | Windows UAC-backed elevated workflows |
| `service` | dispatch services defined under `Config/Service/` |
| `webd` | start a quick `python3 -m http.server` |
| `wcmdbox` / `mobabox` | bridge Windows commands and hardware/network information |
| `binwalk` / `jefferson` | firmware and filesystem analysis |

The `Binary/` directory adds portable standalone utilities such as Caddy, mitmproxy, SSH clients, `tcping`, `RawCap`, `cygwin-setup`, Composer, and PHPUnit.

## Modules and `kmod-setup`

Modules are self-contained directories registered by numbered loaders under `Config/Init.d/`. The 5.2 module system supports remote registries, multiple mirrors, SHA-256 verification, dependencies, install hooks, offline-tolerant registry caching, root-layout packages, and protection for core modules.

```bash
kmod-setup list
kmod-setup list -r
kmod-setup install -r python3x
kmod-setup update [name]
kmod-setup export <name> [out.kmp]
kmod-setup remove <name>
kmod-setup mirror [url]
```

To create a private module, place its files under `Module/<name>/`, add a loader under `Config/Init.d/`, or use `kmod-setup install <name>`. A `.kmp` package is a zip containing `kmp.ini`, `loader.cmd`, and `module/`.

## Optional AI Shell

The `aishell` module converts a natural-language request into a suggested shell command and requires an explicit `RUN` confirmation before execution. Configure the endpoint and model in `Config/AIShell.ini`. **Do not commit API keys**; the release build strips this per-user configuration from the shipped payload.

```bash
aishell "find log files modified in the last 7 days and sort by size"
```

## Directory layout

| Path | Role |
| --- | --- |
| `kRun.exe` | launch any program after booting and injecting the Komandare environment |
| `kmd.exe` | open the complete Komandare shell |
| `kBash.bat` / `kCmd.bat` | enter from an existing terminal |
| `kInitrd.cmd` | common boot chain and environment setup |
| `Config/` | loaders, aliases, cache policy, services, and settings |
| `Module/` | portable toolchain state and packages |
| `Binary/` | standalone Windows tools and wrappers |
| `Unixlike/` | Cygwin userland, Bash, `/kbin`, `etc/`, and `home/` |

## Important boundaries

Komandare is not a VM, WSL distribution, or security boundary. It uses Cygwin userland and Windows processes; programs launched through it still have the Windows permissions of their user or an elevated UAC session. The “sandbox” means a portable, contained development workspace and boot environment, not process isolation.

## Source and release

- **Latest release:** [Komandare 5.2](https://github.com/xRetia/Komandare/releases/tag/5.2.2026.0912)
- **Homepage:** <https://komandare.github.io/>
- **Repository:** <https://github.com/xRetia/Komandare>
- **Vendor:** xRetia Labs

The repository contains the homepage, documentation, launcher sources, and release scripts. The full runtime tree is distributed through the installer and module packages rather than committed here.

*Copyright (c) 2020–2026 xRetia Labs. All rights reserved.*
