# Sleeping Dogs: Definitive Edition — IME fix (SDInputFix)

修复中文输入法把《热血无赖：终极版》踢出独占全屏的问题，并且可以在 ReShade 的界面里正常输入中文。

With a Chinese (or other CJK) IME active, pressing Shift or typing in Sleeping Dogs pops up IME windows,
which knock the game out of exclusive fullscreen. Like the Minecraft 1.7.10 "InputFix" mods, this mod:

- **In game**: detaches the IME from the game window, so Shift / WASD in Chinese mode never shows IME UI.
- **Hotkeys**: blocks input-language switching (Ctrl+Shift, Alt+Shift, Win+Space) while the game is in
  front.
- **ReShade**: re-attaches the IME while a ReShade text box is active. The composition and candidate list are
  drawn inside the overlay, so you can type Chinese into ReShade's search box without leaving fullscreen.

Status: done, verified in-game (exclusive fullscreen, Microsoft Pinyin, ReShade 6.8.0).

## Requirements

- Sleeping Dogs: Definitive Edition (Steam), Windows 10/11 x64.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (e.g. as `dinput8.dll`).
- Optional: [ReShade](https://reshade.me) 6.8.0 with add-on support, for typing into the overlay.

## Install

Copy `SDInputFix.asi` into the game's `plugins\` folder. On first start it writes a commented
`SDInputFix.ini` (bilingual, 中文/English) with switches for each feature, and logs to `SDInputFix.log`.

## Building

Visual Studio 2022 (v143), Windows SDK 10.0.26100. The project expects to sit at `mods\SDInputFix` in a
workspace that has `reference\reshade`: ReShade v6.8.0 source, with the `deps\imgui` submodule initialized.
See `CLAUDE.md` for design notes.

Not affiliated with Square Enix, United Front Games or Microsoft.
