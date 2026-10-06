# SDIMEFix — advanced users and developers

[![Build](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/actions/workflows/build.yml/badge.svg)](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/actions/workflows/build.yml)

> [!IMPORTANT]
> **关于这个 mod**：它完全是用 Claude Code 里的 Claude Fable 和 Opus vibe coding 写出来的，几乎没有经过审查，请当作实验性质的 mod 使用，发现异常请反馈。作为一个长期缺少 mod 而自己下场做 mod 的普通玩家，我在 vibe 的过程中收获了许多快乐；如果你也有想实现的灵感，不妨也试着 vibe 一下。由于这些代码都是 vibe 出来的，所以我不会以我的 mod 盈利，也不接受捐助。如果你喜欢我的作品，请给[致谢](#致谢)中提到的开源项目点个 star，也可以考虑向这些项目和作者捐赠（如果他们接受捐赠的话），祝你游玩愉快！
>
> **About this mod**: it was fully vibe-coded with Claude Fable and Opus in Claude Code, with little review, so treat
> it as experimental and please report anything unusual. I'm just an ordinary player who went a long time without
> mods for this game and finally started making them myself. Vibe coding them has been a lot of fun; if you have an
> idea of your own, it might be for you too. Since all this code is vibe-coded, I won't make money from my mods and
> don't accept donations. If you like my work, please star the open-source projects listed in the
> [Credits](#credits) instead, and consider donating to them and their authors if they accept donations. Have fun!

[中文](#中文) | [English](#english)

新手安装说明见 [README.md](README.md)。 · Step-by-step install for players: [README.md](README.md).

## 中文

### 原理

这个游戏在设计时没有考虑输入法：开着中文输入法时，在游戏里按 Shift 或打字会弹出输入法窗口，把游戏从独占全屏踢回窗口模式。本 mod 的思路和 Minecraft 1.7.10 的 "InputFix" 类 mod 一样：

- **游戏内**：让输入法和游戏窗口脱钩（`ImmAssociateContextEx(hwnd, NULL, 0)`），中文模式下按 Shift / WASD
  都不会再弹出输入法界面。
- **热键**：游戏在前台时，屏蔽切换输入法的热键（Ctrl+Shift、Alt+Shift、Win+Space）。
- **游戏内界面（可选，需要 ReShade）**：作为 ReShade 插件运行，在 ReShade 的文本框里打字时重新接上输入法，拼音组字和候选词直接画在界面里，不用退出全屏就能用输入法打字。

各部分的实现和设计取舍见 [CLAUDE.md](CLAUDE.md)（英文）。

状态：已完成，已在游戏内验证（独占全屏、微软拼音、ReShade 6.8.0）。

### 需求

- 任何版本的《热血无赖：终极版》，Windows 10/11 x64。mod 只用 Win32 层面的钩子（exe 的导入表、窗口消息、IMM32），不含任何硬编码的游戏地址，所以和游戏的 exe 版本、发行平台无关。
- 任意 ASI 加载器，例如 [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（`SDIMEFix.zip`
  里自带一份，作为 `dinput8.dll`）。
- 可选：支持插件的 [ReShade](https://reshade.me) **6.8.0**，只有在 ReShade 界面里输入中文这一项需要。ReShade
  只接受用同一版本 Dear ImGui 编译的插件，版本不符时日志里会写 "ReShade not found (or incompatible)"，其余功能照常工作。

### 下载

[Releases](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/releases) 里每个版本都有：

| 文件 | 内容 |
| --- | --- |
| `SDIMEFix.zip` | 解压到游戏目录：`dinput8.dll`（Ultimate ASI Loader）+ `plugins\SDIMEFix.asi` + 许可声明 |
| `SDIMEFix.asi` | 只有 mod 本体，放进已有加载器的 `plugins\` |
| `SDIMEFix.pdb` | 调试符号，只在分析崩溃转储时需要 |
| `THIRD-PARTY-NOTICES.md` | 第三方代码的许可证 |

`main` 上每次提交都会自动编译、测试并发布为预发布版 `build-<N>`（没有在游戏里测过）。在游戏里验证过的构建会被转为正式版；README 里的下载链接指向最新的正式版。Nexus Mods 上主文件 “SDIMEFix” 是正式版，“SDIMEFix GitHub CI Build” 是每次的预发布版，都是同一个 `SDIMEFix.zip`。

2026 年 10 月以后的构建里，`SDIMEFix.zip`、`SDIMEFix.asi`、`SDIMEFix.pdb` 都附有 GitHub 签名的[构建来源证明](https://docs.github.com/zh/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations)（artifact attestation）。装了 [GitHub CLI](https://cli.github.com/) 的话，可以用 `gh attestation verify SDIMEFix.zip -R aUsernameWoW/sleeping-dogs-ime-fix` 确认下载到的文件确实是这个仓库的 CI 编译的，以及来自哪个提交。

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

没有测试过。Wine 默认优先加载自带的 `dinput8.dll`，要用 `SDIMEFix.zip` 里的加载器，需要在 Steam 的启动选项里填 `WINEDLLOVERRIDES="dinput8=n,b" %command%`。

### 编译

Visual Studio 2022（v143），Windows SDK 10.0.26100。项目需要放在工作区的 `mods\SDIMEFix`，工作区里要有
`reference\reshade`：ReShade v6.8.0 源码，并初始化 `deps\imgui` 子模块。

GitHub Actions 会对推送和 PR 按同样的布局编译（`-warnAsError`）并运行自动测试，依赖的确切版本见
`.github/reference.env`；然后打包 `SDIMEFix.zip`，其中 Ultimate ASI Loader 的版本和 SHA-256 固定在
`.github/asi-loader.env`。推送到 `main` 且测试通过的构建会发布为预发布版 `build-<N>`，并作为新版本上传到
Nexus Mods；在 GitHub 上把预发布版转为正式版，会把它上传到 Nexus 的主文件（`nexus-release.yml`）。`asi-loader.yml` 每月检查一次 Ultimate ASI Loader 的新版本，有新版时开 PR 更新 `asi-loader.env`；`reference.yml` 对编译所用的依赖做同样的检查，开 PR 更新 `reference.env`；Dependabot 每月更新 Actions 的版本。

### 致谢

这个 mod 用到或参考了下面这些人和项目的成果，在此致谢。

**研究资料**

- [SDmodding](https://github.com/SDmodding)，几乎全部出自 [sneakyevil](https://github.com/sneakyevil) 一人之手。这个 mod 用到了：
  - SDmodding 随 [SDK](https://github.com/SDmodding/SDK) 发布的 [Visual Studio 2022 项目模板](https://github.com/SDmodding/SDK/releases/tag/vs2022)：这个 mod 的 Visual Studio 工程源自这个模板，编译设置和以 `dllmain.cc` 为起点的源文件结构都来自它；
  - SDmodding 分享的游戏 v1.0 版 exe 和调试符号（PDB，Steam 首发版自带）：用来研究游戏怎样创建窗口、读取键盘输入；
  - [BigFileSystem](https://github.com/SDmodding/BigFileSystem)、[TheoryEngine](https://github.com/SDmodding/TheoryEngine)，以及 sneakyevil 的 [SD-BigFileExplorer](https://github.com/sneakyevil/SD-BigFileExplorer) 和 [Ekey](https://github.com/Ekey) 的 SDDEUnpacker 里的文件名列表：读取游戏资源包（`.big`）的工具是照着它们写的，横幅图参照的游戏界面贴图就是用它取出的。

**mod 里包含的代码**（许可证全文见 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)）

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（ThirteenAG）：压缩包里的 `dinput8.dll`，让游戏加载 mod。它本身还包含 MinHook、[miniz](https://github.com/richgel999/miniz)（Rich Geldreich 等）和 [praydog](https://github.com/praydog) 的 FunctionHookMinHook。
- [ReShade](https://github.com/crosire/reshade)（crosire）的插件接口和 [Dear ImGui](https://github.com/ocornut/imgui)（Omar Cornut）：在 ReShade 界面里输入中文；把文字交给 ImGui 的那段代码改编自 Dear ImGui。

**参考与灵感**

- [InputFix](https://github.com/zlainsama/InputFix)（zlainsama）等 Minecraft 输入法修复 mod：这个 mod 的思路来自它们。
- [PowerToys](https://github.com/microsoft/PowerToys)（Microsoft）：屏蔽 Win+空格时不让开始菜单弹出，用的是和它一样的办法。
- Microsoft Learn 上的[输入法管理器（IMM32）文档](https://learn.microsoft.com/windows/win32/intl/input-method-manager)。

**工具**

- [IDA Pro](https://hex-rays.com/ida-pro)（Hex-Rays）和 [ida-pro-mcp](https://github.com/mrexodia/ida-pro-mcp)（mrexodia）：分析游戏程序。
- [Claude Code](https://claude.com/claude-code)（Anthropic）：这个 mod 完全是用 Claude Fable 和 Opus vibe coding 写出来的，代码、文档和逆向分析都出自 Claude，几乎没有经过人工审查。
- 字体 [Noto Sans SC/TC](https://fonts.google.com/noto)、[Teko](https://fonts.google.com/specimen/Teko)、[Barlow Condensed](https://fonts.google.com/specimen/Barlow+Condensed)：横幅图和图标。

**游戏与商标**

《热血无赖：终极版》（Sleeping Dogs: Definitive Edition）由 United Front Games 开发、Square Enix 发行，游戏及其内容的版权归 Square Enix 所有。横幅图和图标仿照游戏的菜单界面重新绘制，没有使用游戏原图。Windows 和 PowerToys 是 Microsoft 的商标。

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

Builds since October 2026 carry a signed [build provenance attestation](https://docs.github.com/en/actions/how-tos/secure-your-work/use-artifact-attestations/use-artifact-attestations) for the
`.zip`, `.asi` and `.pdb`: with the [GitHub CLI](https://cli.github.com/),
`gh attestation verify SDIMEFix.zip -R aUsernameWoW/sleeping-dogs-ime-fix` checks that a downloaded file was built by
this repository's CI, and from which commit.

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
automated tests; `.github/reference.env` lists the exact dependency versions. It then packages
`SDIMEFix.zip`, with the Ultimate ASI Loader version and SHA-256 pinned in `.github/asi-loader.env`. Builds
of `main` that pass are published as prereleases `build-<N>` and uploaded to Nexus Mods as a new version;
promoting a prerelease to a full release on GitHub uploads it to the Nexus main file (`nexus-release.yml`).
`asi-loader.yml` checks monthly for a new Ultimate ASI Loader release and opens a PR that updates
`asi-loader.env`, `reference.yml` does the same for the libraries the build compiles against
(`reference.env`), and Dependabot updates the Actions monthly.

### Credits

This mod uses or builds on the work of these people and projects. Thank you.

**Research**

- [SDmodding](https://github.com/SDmodding), almost all of it the work of one person, [sneakyevil](https://github.com/sneakyevil). This mod used:
  - the [Visual Studio 2022 project template](https://github.com/SDmodding/SDK/releases/tag/vs2022) released with SDmodding's [SDK](https://github.com/SDmodding/SDK): the mod's Visual Studio project derives from it, including its build settings and the source layout that starts at `dllmain.cc`;
  - the game's v1.0 exe and its debug symbols (PDB, shipped with the original Steam release), shared by
    SDmodding: used to study how the game creates its window and reads the keyboard;
  - [BigFileSystem](https://github.com/SDmodding/BigFileSystem), [TheoryEngine](https://github.com/SDmodding/TheoryEngine), and the file name lists in sneakyevil's [SD-BigFileExplorer](https://github.com/sneakyevil/SD-BigFileExplorer) and in [Ekey](https://github.com/Ekey)'s
    SDDEUnpacker: the tool that reads the game's `.big` archives follows them; the game's UI textures the banner is modelled on were taken out with it.

**Code in the mod** (full license texts in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md))

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (ThirteenAG): the `dinput8.dll` in the zip, which makes the game load mods.
  It contains MinHook, [miniz](https://github.com/richgel999/miniz) (Rich Geldreich and others) and [praydog](https://github.com/praydog)'s FunctionHookMinHook.
- [ReShade](https://github.com/crosire/reshade) (crosire) add-on API and [Dear ImGui](https://github.com/ocornut/imgui) (Omar Cornut): typing Chinese into ReShade's overlay; the
  code that hands the text to ImGui is adapted from Dear ImGui.

**References and inspiration**

- [InputFix](https://github.com/zlainsama/InputFix) (zlainsama) and the other Minecraft IME fix mods: where the idea
  came from.
- [PowerToys](https://github.com/microsoft/PowerToys) (Microsoft): blocking Win+Space without the Start menu popping
  up works the same way it does there.
- Microsoft Learn's [Input Method Manager (IMM32) documentation](https://learn.microsoft.com/windows/win32/intl/input-method-manager).

**Tools**

- [IDA Pro](https://hex-rays.com/ida-pro) (Hex-Rays) and [ida-pro-mcp](https://github.com/mrexodia/ida-pro-mcp) (mrexodia): analyzing the game's code.
- [Claude Code](https://claude.com/claude-code) (Anthropic): this mod was fully vibe-coded with Claude Fable and Opus; its code,
  documentation and reverse engineering are all Claude's, with little human review.
- The fonts [Noto Sans SC/TC](https://fonts.google.com/noto), [Teko](https://fonts.google.com/specimen/Teko) and [Barlow Condensed](https://fonts.google.com/specimen/Barlow+Condensed): the banner and the icon.

**The game and trademarks**

Sleeping Dogs: Definitive Edition was developed by United Front Games and published by Square Enix; the game
and its content are © Square Enix. The banner and the icon redraw the look of the game's menus; no game art is used in them. Windows and PowerToys are trademarks of Microsoft.

Not affiliated with Square Enix, United Front Games or Microsoft.
