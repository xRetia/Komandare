<h1 align="center">Komandare</h1>

<p align="center"><img src="./Source/logo.png" alt="Komandare logo" width="120" /></p>

<p align="center"><strong>Windows 上的可移动开发沙箱。</strong><br>用 <code>kRun</code> 把完整的 Cygwin + Bash 工具链注入 IDE，无需把每个工具都安装进 Windows。</p>

<p align="center">英文说明：<a href="./README.md">README.md</a></p>

<p align="center"><a href="https://github.com/xRetia/Komandare/releases"><img src="https://badgen.net/github/tag/xRetia/Komandare?label=最新版本&color=blueviolet" alt="Latest Version" /></a> <a href="https://github.com/xRetia/Komandare/releases"><img src="https://badgen.net/github/release/xRetia/Komandare?label=下载&color=blue" alt="Download" /></a></p>

---

## Komandare 是什么？

Komandare 是一个 **Windows 上的可移动开发环境**。它把基于 Cygwin 的 Unix 用户态、Bash、常用命令行工具和可选语言工具链组织在一棵可搬迁的目录树中。它更适合被理解为**开发环境沙箱**，而不是另一个终端模拟器，也不是 Windows 的替代品。

核心思路很简单：**把环境注入你真正要使用的程序**。通过 `kRun` 启动 IDE、调试器、构建工具或脚本时，Komandare 会先引导模块，再把最终的 `PATH`、工具链变量、缓存位置和 junction 交给目标进程。

```bat
kRun code .
kRun "C:\Program Files\JetBrains\Toolbox\bin\jetbrains-toolbox.exe"
kRun powershell -NoExit
```

IDE 不需要再单独安装一份编译器、解释器、包管理器或 Unix 工具；它看到的就是 Komandare Shell 看到的同一套环境。

## 为什么使用它？

| 问题 | Komandare 的解决方式 |
| --- | --- |
| 每个项目需要不同工具链 | 用可移动的 `Module/` 模块按需安装和启用。 |
| IDE 看不到另一个 Shell 里配置的工具 | 用 `kRun` 启动 IDE，启动前自动注入 Shell 环境。 |
| 重装 Windows 后要重新搭建开发环境 | 保留或复制 Komandare 目录，从这棵目录树恢复环境。 |
| 包缓存和全局前缀散落到用户目录 | npm、pnpm、pip、Playwright、Go、Gradle、Ruby 状态收口到 `Module/`。 |
| 想在 Windows 上使用 Unix 命令 | 使用 Cygwin + Bash、`/kbin`、`apt-cyg` 和内置 Windows 桥接工具。 |
| 不想记复杂的 Shell 命令 | 可选的 `aishell` 模块把自然语言转换为确认后执行的命令。 |

> **目录树就是可迁移单位。** 启动器根据自身位置推导 `KMD_ROOT`，整棵树可以搬到另一个目录、盘符、机器或备份中，无需手动重写路径。

## 三个入口

### 1. `kRun`：把环境注入 IDE 或工具

这是 Komandare 最重要的工作流。`kRun.exe` 启动正常的 `kInitrd.cmd` 引导链，把命令交给它。引导链会将 Cygwin 和 `Binary/` 加入 `PATH`，按 `Config/Init.d/` 加载已注册模块，设置缓存和工具链变量，修复可移动 junction，然后启动目标程序。

```bat
kRun code .                 :: VS Code 可直接看到 Node、Python、Git、Bash 等
kRun devenv project.sln     :: 启动其他 Windows 开发工具
kRun my-build.cmd           :: 在相同环境中运行项目脚本
```

不带参数运行 `kRun.exe` 会打开原生 Windows 风格的运行对话框。需要目标始终显示控制台窗口时可使用 `--console`。`kRun` 的实现位于 `Source/kRun.c`，是零 CRT 的原生 PE。

### 2. `kmd`：启动完整 Shell

双击 `kmd.exe` 打开 Komandare 终端。它优先使用 Windows Terminal，失败时回退到内置终端，并可通过 Windows Terminal profile fragment 注入路径转换。`kmd.exe` 启动后即退出，不会常驻后台。

```bat
kmd.exe
```

首次启动时，`kmd-welcome` 会让你选择并安装模块；也可以跳过向导，稍后再管理模块。

### 3. `kBash` / `kCmd`：从现有终端进入

```bat
kBash                         :: 在当前窗口进入带 Komandare 环境的 Bash
kCmd                          :: 在当前窗口进入带 Komandare 环境的 cmd
kInitrd.cmd bash -c "git status"
```

所有启动器最终都会汇聚到 `kInitrd.cmd`，所以 Shell 与通过 `kRun` 启动的 IDE 会获得同一套环境。

## 安装与首次运行

1. 从 5.2 release 下载 **[Komandare-Setup-5.2.2026.0912.exe](https://github.com/xRetia/Komandare/releases/tag/5.2.2026.0912)**。
2. 安装到你控制的目录。安装器包含基础运行时，可选模块在安装后下载。
3. 首次运行 `kmd.exe`，在 `kmd-welcome` 中选择模块，或手动安装：

```bash
kmod-setup sync
kmod-setup install -r git
kmod-setup install -r python3x
kmod-setup install -r nodejs
```

4. 从该环境打开项目，或通过 `kRun` 启动 IDE：

```bat
cd C:\work\my-project
kRun code .
```

安装器提供基础运行时；模块包（`.kmp`）和 `registry.kp` 单独发布在 Releases 中，由 `kmod-setup` 下载，模块不会嵌入安装器。

## 收口哪些状态？

Komandare 不承诺 Windows API 或第三方程序永远不会写出目录树之外。它的承诺更明确也更实用：**由它管理的开发工具链把状态留在沙箱内**。

| 工具链 | 收口内容 |
| --- | --- |
| Node.js 22 | npm 前缀/缓存、pnpm store/缓存、corepack |
| Python 3.12 | pip 缓存、Playwright 浏览器、user site |
| Go | `GOPATH`、`GOMODCACHE`、`GOCACHE`、`GOENV` |
| Ruby 4 | gems 与 SSL 证书 |
| Gradle | `GRADLE_USER_HOME` |
| PHP 8.3 | 运行体与 Composer 工具 |

`z-cache-clean` 可依据 `Config/CacheClean.ini` 维护缓存大小。复制目录会带走已配置的工具链和缓存；删除目录会删除 Komandare 管理的状态，但 Windows 本身和被启动的程序仍受其自身行为影响。

## Shell 与内置工具

Komandare 提供 Cygwin 用户态、Bash、Vim、Git 支持、`curl`、gcc 类开发工具、`apt-cyg` 以及 `/kbin` 命令层。常用内置命令包括：

| 命令 | 作用 |
| --- | --- |
| `kmod-setup` | 安装、更新、导出、删除和切换模块源 |
| `ver` / `version` | 显示 Komandare 和系统信息 |
| `apt-cyg` / `apt` | 管理 Cygwin 软件包 |
| `sudo` / `su` | 基于 Windows UAC 的提权流程 |
| `service` | 分发 `Config/Service/` 下定义的服务 |
| `webd` | 快速启动 `python3 -m http.server` |
| `wcmdbox` / `mobabox` | 桥接 Windows 命令及硬件/网络信息 |
| `binwalk` / `jefferson` | 固件和文件系统分析 |

`Binary/` 还提供 Caddy、mitmproxy、SSH 客户端、`tcping`、`RawCap`、`cygwin-setup`、Composer、PHPUnit 等便携工具。

## 模块与 `kmod-setup`

模块是自包含目录，由 `Config/Init.d/` 下的数字编号 loader 注册。5.2 模块系统支持远端 registry、多镜像、SHA-256 校验、依赖、安装钩子、离线容错缓存、根布局包以及核心模块保护。

```bash
kmod-setup list
kmod-setup list -r
kmod-setup install -r python3x
kmod-setup update [name]
kmod-setup export <name> [out.kmp]
kmod-setup remove <name>
kmod-setup mirror [url]
```

要创建私有模块，可将文件放在 `Module/<name>/`，添加 `Config/Init.d/` loader，或运行 `kmod-setup install <name>`。`.kmp` 是包含 `kmp.ini`、`loader.cmd` 和 `module/` 的 zip 包。

## 可选 AI Shell

`aishell` 将自然语言需求转换为建议的 Shell 命令，并要求明确输入 `RUN` 后才执行。在 `Config/AIShell.ini` 中配置端点和模型。**不要提交 API key**；release 构建会从安装器载荷中剥离这份用户配置。

```bash
aishell "查找过去 7 天修改的日志文件并按大小排序"
```

## 目录结构

| 路径 | 作用 |
| --- | --- |
| `kRun.exe` | 引导环境并注入 Komandare 环境后启动任意程序 |
| `kmd.exe` | 打开完整 Komandare Shell |
| `kBash.bat` / `kCmd.bat` | 从现有终端进入 |
| `kInitrd.cmd` | 公共引导链与环境设置 |
| `Config/` | loader、别名、缓存策略、服务和设置 |
| `Module/` | 可移动工具链状态与模块包 |
| `Binary/` | 独立 Windows 工具和包装器 |
| `Unixlike/` | Cygwin 用户态、Bash、`/kbin`、`etc/` 和 `home/` |

## 重要边界

Komandare 不是 VM、WSL 发行版或安全边界。它使用 Cygwin 用户态和 Windows 进程；通过它启动的程序仍然拥有当前用户或 UAC 提权后的 Windows 权限。“沙箱”指可移动、可收口的开发工作区和引导环境，不代表进程隔离。

## 源码与发布

- **最新版本：** [Komandare 5.2](https://github.com/xRetia/Komandare/releases/tag/5.2.2026.0912)
- **主页：** <https://komandare.github.io/>
- **仓库：** <https://github.com/xRetia/Komandare>
- **厂商：** xRetia Labs

本仓库包含主页、文档、启动器源码和发布脚本。完整运行时目录树通过安装器和模块包分发，不提交到仓库。

*Copyright (c) 2020–2026 xRetia Labs. All rights reserved.*
