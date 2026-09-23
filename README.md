# Sleeping Dogs: Definitive Edition — IME fix (SDIMEFix)

[![Build](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/actions/workflows/build.yml/badge.svg)](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/actions/workflows/build.yml)

[中文](#中文) | [English](#english)

## 中文

修复中文（及其他 CJK）输入法把《热血无赖：终极版》踢出独占全屏的问题，并且可以在 ReShade 的界面里正常输入中文。

开着中文输入法时，在游戏里按 Shift 或打字会弹出输入法窗口，把游戏从独占全屏踢回窗口模式。本 mod 的思路和
Minecraft 1.7.10 的 "InputFix" 类 mod 一样：

- **游戏内**：让输入法和游戏窗口脱钩，中文模式下按 Shift / WASD 都不会再弹出输入法界面。
- **热键**：游戏在前台时，屏蔽切换输入法的热键（Ctrl+Shift、Alt+Shift、Win+Space）。
- **ReShade**：当 ReShade 的文本框处于激活状态时，重新接上输入法；拼音组字和候选词直接画在 ReShade 界面里，
  不用退出全屏就能在搜索框里输入中文。

状态：已完成，已在游戏内验证（独占全屏、微软拼音、ReShade 6.8.0）。

### 需求

- 《热血无赖：终极版》（Steam），Windows 10/11 x64。
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（例如作为 `dinput8.dll`）。
- 可选：支持插件的 [ReShade](https://reshade.me) 6.8.0，用于在 ReShade 界面里输入中文。

### 安装

从 [Releases](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/releases) 下载最新的 `SDIMEFix.asi`
（`main` 上每次提交都会自动编译、测试并发布为预发布版），放进游戏的 `plugins\` 文件夹。首次启动会生成带注释的
`SDIMEFix.ini`（中英双语），每项功能都有开关；日志写到 `SDIMEFix.log`。

本 mod 以前叫 SDInputFix。从旧版升级时，删掉 `plugins\SDInputFix.asi`（否则两个版本会同时加载），并把
`SDInputFix.ini` 改名为 `SDIMEFix.ini` 以保留设置。

### 编译

Visual Studio 2022（v143），Windows SDK 10.0.26100。项目需要放在工作区的 `mods\SDIMEFix`，工作区里要有
`reference\reshade`：ReShade v6.8.0 源码，并初始化 `deps\imgui` 子模块。设计说明见 `CLAUDE.md`（英文）。

GitHub Actions 会对推送和 PR 按同样的布局编译并运行自动测试，依赖的确切版本见 `.github/workflows/build.yml`。
推送到 `main` 且测试通过的构建会发布为预发布版 `build-<N>`，附带 `SDIMEFix.asi` 和 `.pdb`。

编译进 `SDIMEFix.asi` 的第三方代码及其许可证见 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)。

与 Square Enix、United Front Games、Microsoft 均无关联。

## English

With a Chinese (or other CJK) IME active, pressing Shift or typing in Sleeping Dogs: Definitive Edition pops
up IME windows, which knock the game out of exclusive fullscreen. Like the Minecraft 1.7.10 "InputFix" mods,
this mod:

- **In game**: detaches the IME from the game window, so Shift / WASD in Chinese mode never shows IME UI.
- **Hotkeys**: blocks input-language switching (Ctrl+Shift, Alt+Shift, Win+Space) while the game is in
  front.
- **ReShade**: re-attaches the IME while a ReShade text box is active. The composition and candidate list are
  drawn inside the overlay, so you can type Chinese into ReShade's search box without leaving fullscreen.

Status: done, verified in-game (exclusive fullscreen, Microsoft Pinyin, ReShade 6.8.0).

### Requirements

- Sleeping Dogs: Definitive Edition (Steam), Windows 10/11 x64.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (e.g. as `dinput8.dll`).
- Optional: [ReShade](https://reshade.me) 6.8.0 with add-on support, for typing into the overlay.

### Install

Download the latest `SDIMEFix.asi` from [Releases](https://github.com/aUsernameWoW/sleeping-dogs-ime-fix/releases)
(every commit on `main` is built, tested and published as a prerelease) and copy it into the game's
`plugins\` folder. On first start it writes a commented `SDIMEFix.ini` (bilingual, Chinese/English) with
switches for each feature, and logs to `SDIMEFix.log`.

This mod used to be called SDInputFix. When upgrading, delete `plugins\SDInputFix.asi` (otherwise both
versions load) and rename `SDInputFix.ini` to `SDIMEFix.ini` to keep your settings.

### Building

Visual Studio 2022 (v143), Windows SDK 10.0.26100. The project expects to sit at `mods\SDIMEFix` in a
workspace that has `reference\reshade`: ReShade v6.8.0 source, with the `deps\imgui` submodule initialized.
See `CLAUDE.md` for design notes.

GitHub Actions builds pushes and pull requests in that same layout and runs the automated tests;
`.github/workflows/build.yml` lists the exact dependency versions. Builds of `main` that pass are published
as prereleases `build-<N>` with `SDIMEFix.asi` and its `.pdb`.

The third-party code compiled into `SDIMEFix.asi` and its licenses are listed in
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

Not affiliated with Square Enix, United Front Games or Microsoft.
