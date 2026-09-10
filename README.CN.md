---
AIGC:
  ContentProducer: '001191110102MAD55U9H0F10002'
  ContentPropagator: '001191110102MAD55U9H0F10002'
  Label: '1'
  ProduceID: '946595f0-f8e3-4886-ac18-4798917ecbfe'
  PropagateID: '946595f0-f8e3-4886-ac18-4798917ecbfe'
  ReservedCode1: '67872598-49d1-43e5-b650-27a3f8dea51b'
  ReservedCode2: '67872598-49d1-43e5-b650-27a3f8dea51b'
---

<h1 align="center">Komandare</h1>

<p align="center">
  <img src="./Source/logo.png" alt="Komandare logo" width="120" />
</p>

<p align="center">
  基于 Cygwin 内核的 UserLand Linux 发行版，面向开发者 —— 一个文件夹，完整 Unix 用户态与开发工具链，主机零痕迹。
</p>

<p align="center">
  For the English documentation, see <a href="./README.md">README.md</a>
</p>

<p align="center">
  <a href="https://github.com/xRetia/Komandare/releases">
    <img src="https://badgen.net/github/tag/xRetia/Komandare?label=最新版本&color=blueviolet" alt="Latest Version" />
  </a>
  <a href="https://github.com/xRetia/Komandare/releases">
    <img src="https://badgen.net/github/release/xRetia/Komandare?label=下载&color=blue" alt="Download" />
  </a>
  <a href="https://komandare.github.io/">
    <img src="https://img.shields.io/badge/主页-komandare.github.io-01E3F8.svg" alt="Homepage" />
  </a>
</p>

---

- **厂商**：xRetia Labs
- **主页**：<https://komandare.github.io/>
- **仓库**：<https://github.com/xRetia/Komandare>

---

## Shell 与 Banner

每次进入 Komandare 会话，都会先看到 kmd 欢迎横幅（位于 `Config/Banner.ini`，可自行改写，每次进入 `bash` / `cmd` / `powershell` 时输出）：

```
Welcome to Komandare Shell!
Copyright(c) xRetia Labs.
```

接着是 `ver` —— neofetch 加发行版版本信息：

```
==================== Komandare ====================
------------------------------------------------------------
User           : xRetia@MYPC
OS             : Komandare
System Version : 4.0.2026.0910
Kernel         : CYGWIN 3.5
Shell          : Komandare Shell
WCmdBox        : 20.08.1
Unique Libs    : /kbin [kmod-setup, neofetch, wcmdbox, ...]
Project        : https://github.com/xRetia/Komandare
------------------------------------------------------------
Welcome to Komandare!
```

---

## 与普通终端环境有什么不同

**真正的发行版，不是套壳。** Komandare 是一个 UserLand Linux 发行版：内核是 Cygwin，用户态为开发者精心整理 —— 完整的 Unix 命令（bash、vim、git、curl、gcc 等）、`apt-cyg` 包管理，工具链以可移植**模块**的形式组织，而非一坨单块安装。

**主机零污染。** 这是核心承诺。`python3`、`nodejs`、`golang`、`ruby`、`gradle`、`php`、`npm`/`pnpm`/`pip`/`playwright` —— **不会**在 `%APPDATA%`、`%LOCALAPPDATA%` 或任何用户目录下创建任何东西。全局前缀、包缓存、Store、Go 模块缓存、Gradle 用户目录、Ruby gems，全部收口在 Komandare 目录树内部（`Module/...`）。删掉整个文件夹，主机跟没装过一样。

**kRun —— 带 kmd 环境运行任何 IDE。** `kRun.exe` 会先完成与正常启动 shell 相同的环境注入（PATH、模块变量、缓存、junction），然后再启动任何第三方程序。用 VSCode / 任何 IDE / 任何开发工具，都能"看到"完整的 Komandare 工具链：

```bat
kRun code .
```

**kCmd / kBash —— 在其他终端里启用 kmd 环境。** 人已经坐在 PowerShell、Windows Terminal、ConEmu 或 CMD 里了？`kCmd` 和 `kBash` 直接在当前窗口给你一个完整引导的 Komandare shell，不用新开窗口：

```bat
kCmd            :: 进入带 kmd 环境的 cmd
kBash           :: 进入带 kmd 环境的 bash
```

**kmd —— 推荐启动器。** 双击 `kmd.exe` 得到一个完整的 Komandare 终端：优先用 Windows Terminal（≥ 1.60）承载，缺失时回退到内置的 ConEmu 构建；并通过 WT *profile Fragment* 注入路径转换 —— 往终端里拖文件自动变成 `/mnt/c/...`，且**不碰**你的 WT settings.json。

---

## 快速开始

### 1. 启动完整 Komandare 终端

双击 `kmd.exe`（或固定到任务栏 / 开始菜单）。

1. 检测 Windows Terminal 是否安装且版本 ≥ `Config/Terminal.ini` 的 `MinVersion`（默认 1.60）；
2. 满足 → 注入 `pathTranslationStyle` JSON Fragment 并运行 `wt -d <当前目录> cmd /k kInitrd.cmd bash`；
3. 不满足 → 回退 `Console/kmd64.exe`（ConEmu），再 `Console/kmd32.exe`。

启动目录跟随你调用 kmd 时所在的目录（可用 `StartDirectory` 覆盖）。

> `kmd.exe` 是零依赖原生 PE（无 CRT），启动即退出，不留驻后台进程。

### 2. 首次运行：模块安装向导

首次引导时 kInitrd 会自动启动 **kmd-welcome**，一个全屏 TUI 安装向导（dialog 实现，Windows XP 文本安装界面风格），带你勾选安装模块；也可以跳过，之后随时用 `kmod-setup` 管理（见下文）。

### 3. 在其他终端进入 Komandare 环境

```bat
kBash          :: 在当前窗口进入带 kmd 环境的 bash
kCmd           :: 在当前窗口进入带 kmd 环境的 cmd
```

两者都是核心入口的薄封装：

```bat
kInitrd.cmd bash            :: 同 kbash
kInitrd.cmd cmd             :: 同 kcmd
kInitrd.cmd bash -c "ls"    :: 引导环境后执行任意命令
```

### 4. 用 Komandare 环境运行第三方程序

```bat
kRun code .                :: 以完整 Komandare 工具链启动 VSCode
kRun <任意程序.exe> <参数>   :: 任意编辑器 / IDE / 调试器 / 工具同理
```

`kRun` 在启动目标程序前完成与 shell 引导完全相同的环境注入（模块、PATH、缓存变量、junction），让目标程序看到完整 Unix 开发环境，而程序本身不需要安装到主机上。

---

## 引导流程 —— kInitrd

`kInitrd.cmd` 是整个发行版的内核入口：所有启动路径（kmd / Console/kmd64 / Console/kmd32 / kCmd / kBash）最终都汇聚到这里，以单行进度显示（`Loading xxx ...`）完成引导：

1. **binaries** — 将 `Binary/` 加入 PATH；
2. **Cygwin** — 将 `Unixlike/kbin`、`bin`、`sbin`、`usr/bin`、`usr/sbin` 加入 PATH；
3. **filesystem** — 重建 `Unixlike/tmp` 为指向系统 `%TEMP%` 的文本符号链接，同时重建 `/kmod`、`/kconf` 内核路径（绝对路径、由挂载表派生 —— 目录树搬到别的盘也有效）；
4. **modules** — 按数字顺序（00-99）逐个执行 `Config/Init.d/NN-*.cmd`，每个注册一个工具链模块；
5. **desktop** — 从注册表探测当前用户桌面路径到 `%DESKTOP%`；
6. **aliases** — 加载 `SystemAliases.ini` + `UserAliases.ini` 的 doskey 宏；
7. **locale** — `LANG=zh_CN.UTF-8`、`chcp 65001`；
8. **identity** — 依据管理员身份调用 `runas_root` / `runas_user` 改写 `/etc/passwd`；
9. **first run** — 首次启动运行 `kmd-welcome` 向导（`Kernel.ini` 可关闭）；
10. **banner** — 输出 `Config/Banner.ini`，然后执行传入的 shell。

引导完成后 `KMD_ROOT`（根目录）、`MOD_ROOT`、`BIN_ROOT`、`UNIX_ROOT`、`DESKTOP` 等环境变量全部就绪，所有脚本都以 `KMD_ROOT` 为锚点寻址 —— 整个目录可随意移动。

---

## 目录结构

| 路径 | 角色 |
| --- | --- |
| `kmd.exe` | 推荐启动器（优先 Windows Terminal，回退 ConEmu） |
| `kRun.exe` | 以 kmd 环境运行任意第三方程序 |
| `kBash.bat` / `kCmd.bat` | 从任意其他终端进入 kmd 环境 |
| `kInitrd.cmd` | 内核入口 —— 所有启动器汇聚的引导程序 |
| `Config/` | 启动器配置、模块清单（`Init.d/`）、别名、缓存策略、服务 |
| `Module/` | 工具链包，一个目录一个应用，由 `Config/Init.d/*.cmd` 注册 |
| `Binary/` | 便携独立工具与包装脚本（caddy、mitmproxy、cygwin-setup 等） |
| `Unixlike/` | Cygwin 用户态：`bin/`、`usr/`、`kbin/`（内置命令）、`etc/`、`home/` |

---

## Binary —— 便携独立工具

`Binary/` 下的东西全部是绿色便携的，启动时自动进 PATH。除官方 `cygwin-setup` 外，还收罗了常用的 Windows 网络 / 开发辅助工具：

| 工具 | 作用 |
| --- | --- |
| `caddy.exe` | 轻量 HTTP / 反向代理服务器 |
| `mitmproxy` / `mitmdump` / `mitmweb` | 交互式 HTTPS 抓包与流量分析 |
| `plink.exe` / `scp.exe` / `sftp.exe` | PuTTY 的 SSH 命令行客户端（sftp/scp） |
| `tcping.exe` | TCP ping —— 探测端口是否可连 |
| `RawCap.exe` | 抓取本机网卡原始流量存为 pcap |
| `streams.exe` / `streams64.exe` | 剥离 NTFS 备用数据流（Mark-of-the-Web） |
| `cygwin-setup.exe` | 官方 Cygwin `setup-x86_64.exe`（完整包管理） |
| `NSIS/` | Nullsoft 安装程序制作器 —— `makensis` |
| `composer.bat`（+ `.phar`） | PHP 依赖管理器（PATH 里的 `composer`） |
| `phpunit.bat`（+ `.phar`） | PHPUnit 测试运行器 |
| `binwalk.bat` / `jefferson.bat` | 固件提取与文件系统分析（`Module/` 内 Python 工具的前端） |
| `service.bat` | 服务管理器分发器（见「服务管理」） |
| `desktopini.bat` | 交互式给文件夹设置显示名 / 图标（写 `desktop.ini`） |
| `pptp-dial.bat`（+ `pptp.pbk`） | 拨号 / 断开 PPTP VPN（`dial <地址> <用户> <密码>` / `disconnect`） |
| `MoveLater.exe` | 把文件删除推迟到下次重启执行 |
| `kmd_reset.cmd` | 把 ConEmu 标签页标题重置为 "Komandare" |

---

## 工具链 —— 主机零污染

每个模块把状态**全部钉死在 `Module/<名称>/` 内部**，不写 `%APPDATA%` / `%LOCALAPPDATA%` / `%USERPROFILE%`：

| 工具链 | 收口内容 |
| --- | --- |
| **Node.js 22** | npm 缓存与前缀、pnpm store 与缓存、corepack —— 另有自愈的 `%APPDATA%\npm` junction 指向 `Module/nodejs/packages` |
| **Python 3.12** | pip 缓存、Playwright 浏览器、user-site —— `PIP_CACHE_DIR`、`PLAYWRIGHT_BROWSERS_PATH`、`PYTHONUSERBASE` 全部在 `Module/python3x` 内 |
| **Go** | `GOPATH`、`GOMODCACHE`、`GOCACHE`、`GOENV` 全部在 `Module/golang` 内 |
| **Ruby 4** | `GEM_HOME` / `GEM_PATH`、SSL 证书在 `Module/ruby4` 内 |
| **Gradle** | `GRADLE_USER_HOME` 在 `Module/gradle` 内 |
| **PHP 8.3** | 运行体在 `Module/php` 内 |

缓存自维护由每次启动的 `z-cache-clean` 模块完成：目录自动创建，体积按 `Config/CacheClean.ini` 检查（默认：npm 512 MB、pnpm-store 1 GB、pnpm-cache 256 MB、pip 1 GB、playwright 2 GB），超过上限会先询问再清理。整个目录拷到另一台机器 —— 缓存跟着走。

### 模块清单

每个 `Module/<名称>/` 都是一个自包含的工具链包，由 `Config/Init.d/*.cmd` 注册：

| 模块 | 提供什么 |
| --- | --- |
| `7zip` | 7-Zip 压缩（`7z`、`7za`） |
| `adb` / `fastboot` | Android 设备桥接与 bootloader 工具 |
| `dig` | DNS 查询工具 |
| `git` | Git（优先复用 Git for Windows，缺失回退内置 Cygwin git） |
| `golang` | Go 工具链（`go`、`gofmt`） |
| `gradle` | Gradle 构建系统 |
| `gsudo` | Windows 下 UAC 提权的 `sudo` |
| `iperf3` | 网络吞吐测试 |
| `lessc` | LESS CSS 预编译器 |
| `nodejs` | Node.js 22 + `npm` / `pnpm` / `corepack` |
| `ntr` | 网络流量转发工具 |
| `php` | PHP 8.3 运行体 |
| `pstop` | 进程查看 / 管理器 |
| `python3x` | Python 3.12 + `pip` / Playwright |
| `qemu` | QEMU 机器模拟器 |
| `ruby4` | Ruby 4 运行体（`GEM_HOME` 在内部） |
| `ruby-2.1.7` | 老版 Ruby 2.1.7 运行体 |
| `selfsign-ssl` | 本地自签名 SSL 证书生成器 |
| `socat` | 双向数据中继 / socket 工具 |
| `squashfs-tools` | `mksquashfs` / `unsquashfs` 文件系统工具 |
| `unzip` | ZIP 解压 |
| `vbox-helper` | VirtualBox headless 辅助（Docker 虚拟机支持） |
| `windows-driver-sign` | 签名 Windows 驱动（WHQL / 自签名） |
| `z-cache-clean` | 启动时缓存自维护（见上文） |

---

## kmod-setup —— 模块包管理器

模块由内置 `/kbin` 命令 `kmod-setup` 管理：

```bash
kmod-setup list                  # 列出已注册模块
kmod-setup list -r               # 对比远端 registry 版本
kmod-setup install <目录> [优先级 00-99] [包名] [版本] [发行者]
kmod-setup install <包.kmp>      # 从本地包安装
kmod-setup install <name> -r     # 从远端 registry 下载安装
kmod-setup sync                  # 刷新远端 registry 缓存
kmod-setup update [name]         # 检查/安装远端更新
kmod-setup mirror [url]          # 查看/设置下载镜像（默认 GitHub releases）
kmod-setup export <name> [out.kmp]
kmod-setup remove <name>         # 注销模块（保留 Module/<目录>）
```

- `install <目录>` 通过生成 `Config/Init.d/NN-名字.cmd` 注册已有的 `Module/<目录>`；
- `.kmp` 包是普通 zip（`kmp.ini` + `loader.cmd` + `module/`）—— 可手工构造，远端拉取会校验 sha256；
- `mirror` 切换下载基址（默认 `github.com/xRetia/komandare-mod-pkgs` 的 latest release），内网/离线环境很实用。

当前内置模块：7zip、adb、dig、git（优先复用 Git for Windows，缺失回退内置 Cygwin git）、golang、gradle、gsudo、iperf3、lessc、nodejs、ntr、php、pstop、python3x、qemu、ruby4、selfsign-ssl、socat、squashfs-tools、unzip、vbox-helper、windows-driver-sign、z-cache-clean。

---

## kbin —— 内置命令

`/kbin` 是 Komandare 叠加在 Cygwin 之上的自定义命令层（优先级高于普通 bin）：

| 命令 | 作用 |
| --- | --- |
| `su` / `nosu` | 触发 Windows UAC 提权，新开一个 root 权限的 kmd 窗口（root 免密）；`nosu` / `unroot` 还原普通用户映射 |
| `sudo` | 经 `gsudo` 弹 UAC 以 root 执行命令，结束后自动恢复 passwd 映射 |
| `ver` / `version` | neofetch + 发行版版本（读 `Config/Kernel.ini`）；`-s` 只输出版本号 |
| `kmod-setup` | 模块系统包管理器 |
| `kmd-welcome` | 首启安装向导（全屏 TUI，安装模块） |
| `mkrootfs` | 重建 `kmod` / `kconf` / `tmp` 文本符号链接（由挂载表派生，幂等） |
| `neofetch` | 系统信息展示 |
| `ps` / `top` / `killall` | 进程观测（Windows 感知） |
| `ifconfig` / `hwinfo` | 经 wcmdbox/mobabox 查网络/硬件信息 |
| `adb` / `fastboot` | Android 设备操作 |
| `apt-cyg` | Cygwin 包管理器（别名 `apt`、`apt-get`、`yum`、`apk`） |
| `binwalk` / `jefferson` | 固件分析 |
| `phptest` | PHP 快速草稿：`nano` 打开临时 `.php`，保存后直接运行 |
| `wcmdbox` / `mobabox` | Komandare 工具盒（从 Unix 路径跑 Windows 工具） |
| `poweroff` / `reboot` / `reset` | 快捷系统操作 |
| `attrib` / `dir` / `mklink` | 经 wcmdbox 桥接的 Windows 文件系统命令 |

完整 Cygwin 包管理用 `cygwin-setup`（官方 setup-x86_64.exe，已收入 `Binary/`）；日常装包用 `apt` 别名即可。

---

## 别名

doskey 宏来自 `SystemAliases.ini`（系统）+ `UserAliases.ini`（你的）。常用：

| 别名 | 展开为 | 用途 |
| --- | --- | --- |
| `apt` / `apt-get` / `yum` / `apk` | `bash apt-cyg $*` | 用包管理器习惯装 Cygwin 包 |
| `su` / `nosu` | UAC 提权 / 还原 | root 免密 |
| `sudo` | `gsudo $*` | 以 Windows UAC 提权运行命令 |
| `ver` / `version` | `sh /kbin/version` | neofetch + 版本 |
| `d` / `desktop` | `cd $DESKTOP` | 跳桌面 |
| `webd` | `python3 -m http.server` | 当前目录起 HTTP 服务 |
| `e.` | `explorer .` | 资源管理器打开当前目录 |
| `np` | `notepad $*` | 用记事本打开文件 |
| `vi` | `vim $*` | 用 Vim 打开文件 |
| `gl` / `gm` | git log 图形 / add-commit-push | git 一键流 |
| `ifconfig` / `hwinfo` | mobabox 桥接 | 网络/硬件信息 |
| `mtr` | `ntr $*` | 路由追踪 |
| `clear` / `cls` | 清屏 | 重置终端 |
| `ls` / `ll` / `la` | 彩色显示、自动隐藏 NTUSER 残留 | 目录列表 |

---

## 服务管理

`service`（在 `Binary/`）是个极简服务管理器：

```bat
service docker start       :: 启动 Docker 虚拟机（VirtualBox headless）
service docker stop        :: 挂起
service docker force-stop  :: 强制关机
service docker reset       :: 还原到 Factory 快照
service docker install     :: 从 vm.7z 重新部署虚拟机
service docker opendir     :: 打开虚拟机 Samba 共享
```

在 `Config/Service/` 放一个 `<名字>.bat` 即可新增服务。

---

## 便携性设计

- **路径锚定** — 一切内部脚本以 `%KMD_ROOT%`（由 kInitrd 按 `kInitrd.cmd` 实际位置推导）寻址，整棵目录树可搬到任意位置、任意盘符；
- **junction 自愈** — `%APPDATA%\npm` 每次启动自动重指向 `Module/nodejs/packages`；
- **缓存收口** — npm / pnpm / pip / playwright / Go 缓存全部留在 `Module/` 内部，绝不散落到用户目录，拷走即走；
- **文本符号链接** — `Unixlike/kmod`、`/kconf`、`tmp` 都是普通文本符号链接，启动时由挂载表重建，7z/NSIS/tar 复制字节级无损、无需管理员权限；
- **WT 配置无侵入** — 对 Windows Terminal 的定制只走标准 Fragment 机制，`UseWindowsTerminal=no` 或删除 Fragment 即可随时脱离。

### 打包与权限

ZIP/tar/NSIS 往返会剥掉 POSIX 的 group/other 执行位（症状：Cygwin 下 `rc=127`）。两个收口点会自动补齐：`Source/build-release.ps1`（打安装器前对整树执行 `chmod -R u+rwx,go+rx`）和 `kmod-setup install`（解包 `.kmp` 后）。手工拷贝目录遇到 `rc=127`，在 bash 里 `chmod +x` 该文件即可。

---

## 常见问题

**Q：双击 kmd 提示"既未找到可用的 Windows Terminal，回退终端也启动失败"？**
`Console/kmd64.exe` / `kmd32.exe` 被移动或更名了，检查 `Terminal.ini` 的 `Fallback64` / `Fallback32`。

**Q：想强制用 ConEmu 而不是 WT？**
`Terminal.ini` 设 `UseWindowsTerminal=no`，或直接运行 `Console/kmd64.exe`。

**Q：为什么命令行中文乱码？**
Komandare 启动时自动 `chcp 65001`（UTF-8）。如果是从外部终端（kBash/kCmd）进入的，请确认宿主终端字体与编码为 UTF-8；PowerShell 控制台建议改用 Windows Terminal 或 ConEmu 承载。

**Q：如何添加自己的工具？**
独立绿色程序丢进 `Binary/`（自动进 PATH）；需要复杂初始化（改环境变量、设缓存）的丢进 `Module/` 用 `kmod-setup install` 注册（或手写 `Config/Init.d/NN-名字.cmd`）；纯别名加到 `Config/UserAliases.ini`。

---

*Copyright (c) 2020 - 2026 xRetia Labs. All rights reserved.*