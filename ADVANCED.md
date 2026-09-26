# SDIMEFix — advanced users and developers

[![Build](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/actions/workflows/build.yml/badge.svg)](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/actions/workflows/build.yml)

[中文](#中文) | [English](#english)

新手安装说明见 [README.md](README.md)。 · Step-by-step install for players: [README.md](README.md).

## 中文

### 原理

这个游戏在设计时没有考虑输入法：开着中文输入法时，在游戏里按 Shift 或打字会弹出输入法窗口，把游戏从独占全屏
踢回窗口模式。本 mod 的思路和 Minecraft 1.7.10 的 "InputFix" 类 mod 一样：

- **游戏内**：让输入法和游戏窗口脱钩（`ImmAssociateContextEx(hwnd, NULL, 0)`），中文模式下按 Shift / WASD
  都不会再弹出输入法界面。
- **热键**：游戏在前台时，屏蔽切换输入法的热键（Ctrl+Shift、Alt+Shift、Win+Space）。
- **游戏内界面（可选，需要 ReShade）**：作为 ReShade 插件运行，在 ReShade 的文本框里打字时重新接上输入法，
  拼音组字和候选词直接画在界面里，不用退出全屏就能用输入法打字。

各部分的实现和设计取舍见 [CLAUDE.md](CLAUDE.md)（英文）。

状态：已完成，已在游戏内验证（独占全屏、微软拼音、ReShade 6.8.0）。

### 需求

- 任何版本的《热血无赖：终极版》，Windows 10/11 x64。mod 只用 Win32 层面的钩子（exe 的导入表、窗口消息、
  IMM32），不含任何硬编码的游戏地址，所以和游戏的 exe 版本、发行平台无关。
- 任意 ASI 加载器，例如 [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（`SDIMEFix.zip`
  里自带一份，作为 `dinput8.dll`）。
- 可选：支持插件的 [ReShade](https://reshade.me) **6.8.0**，只有在 ReShade 界面里输入中文这一项需要。ReShade
  只接受用同一版本 Dear ImGui 编译的插件，版本不符时日志里会写 "ReShade not found (or incompatible)"，
  其余功能照常工作。

### 下载

[Releases](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/releases) 里每个版本都有：

| 文件 | 内容 |
| --- | --- |
| `SDIMEFix.zip` | 解压到游戏目录：`dinput8.dll`（Ultimate ASI Loader）+ `plugins\SDIMEFix.asi` + 许可声明 |
| `SDIMEFix.asi` | 只有 mod 本体，放进已有加载器的 `plugins\` |
| `SDIMEFix.pdb` | 调试符号，只在分析崩溃转储时需要 |
| `THIRD-PARTY-NOTICES.md` | 第三方代码的许可证 |

`main` 上每次提交都会自动编译、测试并发布为预发布版 `build-<N>`（没有在游戏里测过）。在游戏里验证过的构建会被
转为正式版；README 里的下载链接指向最新的正式版。Nexus Mods 上主文件 “SDIMEFix” 是正式版，
“SDIMEFix GitHub CI Build” 是每次的预发布版，都是同一个 `SDIMEFix.zip`。

### 装进已有的 mod 环境

已经有 ASI 加载器（不论叫 `dinput8.dll`、`winmm.dll`、`version.dll` 还是别的名字）时，只需要把 `SDIMEFix.asi`
放进它加载插件的目录（通常是 `plugins\`），不要再放一份 `dinput8.dll`。`SDIMEFix.ini` 和 `SDIMEFix.log` 写在
`.asi` 旁边。

本 mod 以前叫 SDInputFix。从旧版升级时，删掉 `plugins\SDInputFix.asi`（否则两个版本会同时加载），并把
`SDInputFix.ini` 改名为 `SDIMEFix.ini` 以保留设置。

### 设置（`SDIMEFix.ini`）

首次启动时生成带注释的 `SDIMEFix.ini`（中英双语）。`1` 开，`0` 关，改完重启游戏生效。

| 段 | 键 | 默认 | 作用 |
| --- | --- | --- | --- |
| `[General]` | `DisableIME` | 1 | 游戏内让输入法和窗口脱钩 |
| `[General]` | `BlockLanguageSwitch` | 1 | 游戏在前台时屏蔽 Ctrl+Shift / Alt+Shift / Win+Space |
| `[Overlay]` | `TextInput` | 1 | 在 ReShade 文本框里输入时临时接上输入法 |
| `[Overlay]` | `ImeUI` | 1 | 隐藏输入法自带的组字/候选窗，改画在 ReShade 界面里 |
| `[Debug]` | `Logging` | 1 | 在 `.asi` 旁边写 `SDIMEFix.log` |

日志记录加载了哪些钩子、窗口焦点变化和每次挂接/脱钩输入法；ReShade 文本框有输入时还会记录每条 `WM_IME_*`
消息，所以比较长，排查问题时附上它即可。

### Wine / Proton

没有测试过。Wine 默认优先加载自带的 `dinput8.dll`，要用 `SDIMEFix.zip` 里的加载器，需要在 Steam 的启动选项里
填 `WINEDLLOVERRIDES="dinput8=n,b" %command%`。

### 编译

Visual Studio 2022（v143），Windows SDK 10.0.26100。项目需要放在工作区的 `mods\SDIMEFix`，工作区里要有
`reference\reshade`：ReShade v6.8.0 源码，并初始化 `deps\imgui` 子模块。

GitHub Actions 会对推送和 PR 按同样的布局编译（`-warnAsError`）并运行自动测试，依赖的确切版本见
`.github/workflows/build.yml`；然后打包 `SDIMEFix.zip`，其中 Ultimate ASI Loader 的版本和 SHA-256 固定在
`.github/asi-loader.env`。推送到 `main` 且测试通过的构建会发布为预发布版 `build-<N>`，并作为新版本上传到
Nexus Mods；在 GitHub 上把预发布版转为正式版，会把它上传到 Nexus 的主文件（`nexus-release.yml`）。
`asi-loader.yml` 每月检查一次 Ultimate ASI Loader 的新版本，有新版时开 PR 更新 `asi-loader.env`；
Dependabot 每月更新 Actions 的版本。

第三方代码及其许可证见 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)。

与 Square Enix、United Front Games、Microsoft 均无关联。

## English

### How it works

The game was not designed with IMEs in mind: with a Chinese IME active, pressing Shift or typing in the game
pops up IME windows, which knock the game out of exclusive fullscreen. Like the Minecraft 1.7.10 "InputFix"
mods, this mod:

- **In game**: detaches the IME from the game window (`ImmAssociateContextEx(hwnd, NULL, 0)`), so Shift /
  WASD in Chinese mode never shows IME UI.
- **Hotkeys**: blocks input-language switching (Ctrl+Shift, Alt+Shift, Win+Space) while the game is in
  front.
- **In-game overlays (optional, needs ReShade)**: runs as a ReShade add-on; while a ReShade text box is
  active, the IME is re-attached and its composition and candidate list are drawn inside the overlay, so you
  can type with the IME without leaving fullscreen.

[CLAUDE.md](CLAUDE.md) describes the implementation and why it is built the way it is.

Status: done, verified in-game (exclusive fullscreen, Microsoft Pinyin, ReShade 6.8.0).

### Requirements

- Any version of Sleeping Dogs: Definitive Edition, Windows 10/11 x64. The mod only hooks at the Win32 level
  (the exe's import table, window messages, IMM32) and has no hard-coded game addresses, so it doesn't care
  about the exe build or the store it came from.
- Any ASI loader, e.g. [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
  (`SDIMEFix.zip` includes it as `dinput8.dll`).
- Optional: [ReShade](https://reshade.me) **6.8.0** with add-on support, only for typing into its overlay.
  ReShade only accepts add-ons built against its own Dear ImGui version; with another version the log says
  "ReShade not found (or incompatible)" and everything else keeps working.

### Downloads

Every version on [Releases](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/releases) has:

| File | Contents |
| --- | --- |
| `SDIMEFix.zip` | Unpacks into the game folder: `dinput8.dll` (Ultimate ASI Loader) + `plugins\SDIMEFix.asi` + notices |
| `SDIMEFix.asi` | The mod alone, for the `plugins\` folder of an existing loader |
| `SDIMEFix.pdb` | Debug symbols, only needed to read crash dumps |
| `THIRD-PARTY-NOTICES.md` | Licenses of the third-party code |

Every commit on `main` is built, tested and published as a prerelease `build-<N>` (not tested in game).
Builds verified in game are promoted to full releases; the README's download link points to the newest one.
On Nexus Mods the main file "SDIMEFix" is the full release and "SDIMEFix GitHub CI Build" follows the
prereleases; both are the same `SDIMEFix.zip`.

### Adding it to an existing mod setup

If you already have an ASI loader (whether it's called `dinput8.dll`, `winmm.dll`, `version.dll` or
something else), just put `SDIMEFix.asi` where it loads plugins from (usually `plugins\`) and don't add
another `dinput8.dll`. `SDIMEFix.ini` and `SDIMEFix.log` are written next to the `.asi`.

This mod used to be called SDInputFix. When upgrading, delete `plugins\SDInputFix.asi` (otherwise both
versions load) and rename `SDInputFix.ini` to `SDIMEFix.ini` to keep your settings.

### Settings (`SDIMEFix.ini`)

On first start the mod writes a commented `SDIMEFix.ini` (Chinese/English). `1` is on, `0` off; restart
the game after changing it.

| Section | Key | Default | Effect |
| --- | --- | --- | --- |
| `[General]` | `DisableIME` | 1 | Detach the IME from the game window |
| `[General]` | `BlockLanguageSwitch` | 1 | Block Ctrl+Shift / Alt+Shift / Win+Space while the game is in front |
| `[Overlay]` | `TextInput` | 1 | Re-attach the IME while a ReShade text box is active |
| `[Overlay]` | `ImeUI` | 1 | Hide the IME's own composition/candidate windows and draw them in the overlay |
| `[Debug]` | `Logging` | 1 | Write `SDIMEFix.log` next to the `.asi` |

The log records the hooks installed, focus changes and every IME attach/detach; while a ReShade text box
has input it also records every `WM_IME_*` message, so it gets long. Attach it when reporting a problem.

### Wine / Proton

Untested. Wine prefers its own `dinput8.dll`; to use the loader from `SDIMEFix.zip`, set the Steam launch
options to `WINEDLLOVERRIDES="dinput8=n,b" %command%`.

### Building

Visual Studio 2022 (v143), Windows SDK 10.0.26100. The project expects to sit at `mods\SDIMEFix` in a
workspace that has `reference\reshade`: ReShade v6.8.0 source, with the `deps\imgui` submodule initialized.

GitHub Actions builds pushes and pull requests in that same layout (with `-warnAsError`) and runs the
automated tests; `.github/workflows/build.yml` lists the exact dependency versions. It then packages
`SDIMEFix.zip`, with the Ultimate ASI Loader version and SHA-256 pinned in `.github/asi-loader.env`. Builds
of `main` that pass are published as prereleases `build-<N>` and uploaded to Nexus Mods as a new version;
promoting a prerelease to a full release on GitHub uploads it to the Nexus main file (`nexus-release.yml`).
`asi-loader.yml` checks monthly for a new Ultimate ASI Loader release and opens a PR that updates
`asi-loader.env`; Dependabot updates the Actions monthly.

The third-party code and its licenses are listed in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

Not affiliated with Square Enix, United Front Games or Microsoft.
