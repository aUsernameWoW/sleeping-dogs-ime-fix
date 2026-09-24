# SDIMEFix

Called SDInputFix until 2026-09-23 (renamed so it isn't mistaken for a controller/input mod). Like the Minecraft 1.7.10 "InputFix" mods, for Sleeping Dogs DE: a Chinese IME can no longer knock the game
out of exclusive fullscreen, and Chinese can still be typed into ReShade's overlay. Status: done, verified
in-game (fullscreen, Microsoft Pinyin, ReShade 6.8.0).

## What it does

1. **In-game**: detaches the IME from the game window (`ImmAssociateContextEx(hwnd, NULL, 0)`), so Shift /
   WASD in Chinese mode never pops IME UI. Re-applied on every activate/focus/`WM_IME_SETCONTEXT`.
2. **Hotkeys**: swallows `WM_INPUTLANGCHANGEREQUEST` (Ctrl+Shift, Alt+Shift) and Win+Space while the game is
   in front.
3. **ReShade**: registers itself as a ReShade add-on. While ImGui has an active text box
   (`io.WantTextInput`) the IME is re-attached, its own composition/candidate windows are hidden, and the
   composition + candidates are drawn inside the overlay at the text caret.

## Files

- `dllmain.cc` — loads config/log, installs the three modules below.
- `core/ime_guard.*` — finds the game window (patches the exe's `CreateWindowExA` import), installs
  `WH_CALLWNDPROC` + `WH_GETMESSAGE` thread hooks and an ANSI wndproc subclass; owns attach/detach state.
- `core/ime_text.*` — while typing into the overlay: handles `WM_IME_*` in the wndproc, keeps
  composition/candidates (IMM32 `ImmGetCompositionStringW` / `ImmGetCandidateListW`) and committed text.
- `core/keyboard_guard.*` — `WH_KEYBOARD_LL` hook on its own thread for Win+Space.
- `core/reshade_overlay.*` — ReShade add-on glue: `reshade_open_overlay` / `reshade_overlay` events,
  ImGui drawing, feeding committed text to ImGui.
- `core/config.*`, `core/log.*` — `SDIMEFix.ini` / `SDIMEFix.log` next to the `.asi`.
- `tests/ime_detach_test.cc` (automated), `tests/win_space_manual.cc` (sends real keys; run by hand).
- `.github/workflows/build.yml` — CI on GitHub Actions (`windows-2025-vs2026`): recreates the workspace
  layout with ReShade at the pinned v6.8.0 commit (`RESHADE_REF`; sparse: `include` + `deps/imgui`, cached
  under the pin), builds Release x64 with `-warnAsError`, runs `tests\*_test.cc` like `build.ps1 -Test`,
  uploads `.asi` + `.pdb`. On `main` a second job publishes them with `THIRD-PARTY-NOTICES.md` (licenses of
  the code compiled in; keep it in step with the dependencies) as prerelease `build-<N>` (N = commit count).
  A third job uploads the same build to [Nexus Mods](https://www.nexusmods.com/sleepingdogsdefinitiveedition/mods/172)
  as the next version of the main file (`Nexus-Mods/upload-action`, v3 API): a zip with
  `plugins\SDIMEFix.asi` + the notices, version `build-<N>`, the commit subjects since the version Nexus has
  as changelog; the previous version is archived. Repo variables `NEXUS_MOD_ID` / `NEXUS_FILE_ID` (v3 IDs from
  the file's "Advanced" dialog, not the `172` in the URL) and secret `NEXUSMODS_API_KEY`; without
  `NEXUS_FILE_ID` the job is skipped, and a build Nexus already has is not uploaded again.
  Actions are pinned by commit SHA; `.github/dependabot.yml` proposes updates monthly.
- `assets/` — `banner.png` (README header and the GitHub social preview, 1280×640, keep under 1 MB) and
  `icon.png` (512×512, transparent corners), both rendered from `assets/branding/logo.html`: open it with
  `?export=banner` / `?export=icon` in headless Edge (`--screenshot --window-size=W,H
  --default-background-color=00000000 --virtual-time-budget=10000`). The look is redrawn from the game's
  menu UI in CSS/SVG (the page lists which textures); no game art is embedded. Copy says the game lacked IME
  support; don't phrase it as fixing ReShade.
- `.claude/settings.json` — Claude Code plugins for this repo: `clangd-lsp` (reads
  `build\compile_commands.json` from the workspace's `tools\compile-commands.ps1`), `microsoft-docs`
  (IMM32/Win32 reference), `ida-pro-mcp` (game binary).

## Design decisions and why (don't undo without reason)

- **IMM calls only work on the window's own thread.** Everything that changes the association runs on the
  game thread: other threads post a registered `SDIMEFix.EnforceIME` message that `GetMsgProc` handles and
  turns into `WM_NULL`. `SetDetached()` records state *before* `ImmAssociateContextEx`, which synchronously
  sends `WM_IME_SETCONTEXT` (whose `ISC_SHOWUI*` bits the subclass must already strip).
- **The subclass uses `SetWindowLongPtrA`** so the window stays ANSI; `W` would silently make it Unicode.
- **Win+Space is handled by the shell**, the game gets no message. The LL hook drops Space while Win is
  held and injects VK `0xFF` (marked via `dwExtraInfo`) so the shell doesn't open the Start menu on Win-up.
  It runs on a dedicated thread: Windows silently drops LL hooks whose thread stalls (loading screens).
- **Committed text bypasses the message queue.** Any `WM_CHAR` pulled through the game's `PeekMessageA` is
  converted to the input locale's DBCS (`你` U+4F60 → `0xE3C4`), and ReShade reads `wParam` as UTF-16 →
  `?`. Posting via A or W makes no difference. So `GCS_RESULTSTR` is read as UTF-16 and pushed directly
  into `ImGuiContext::InputEventsQueue` on the render thread (a copy of `ImGuiIO::AddInputCharacter`,
  which isn't in ReShade's function table; allocations go through ReShade's `ImGui::MemAlloc`).
- **Caret position** comes from `ImGuiContext::PlatformImeDataPrev` via `imgui_internal.h`. That's an
  internal layout, which is why `reshade_overlay.cc` static-asserts `IMGUI_VERSION_NUM == 19250`.
- **`reshade_overlay` is registered only while the overlay is open**: registering it makes ReShade run its
  ImGui pass every frame.
- Registering as an add-on from an `.asi` works because ReShade (`dxgi.dll`) is a static import of the exe
  and is loaded before the ASI loader runs. Without ReShade the overlay module logs and stays off.

## Updating for a new ReShade version

Check out the matching ReShade tag in `reference\reshade`, `git submodule update --init deps/imgui`, then
update the `IMGUI_VERSION_NUM` assert and re-verify `ImGuiInputEvent` / `InputEventsQueue` /
`PlatformImeDataPrev` against `imgui.cpp`'s `AddInputCharacter`. Pin the same ReShade commit in
`.github/workflows/build.yml` (`RESHADE_REF`; CI takes ImGui from it). ReShade refuses add-ons built against a
different ImGui version (`ReShadeGetImGuiFunctionTable` returns null → logged as "ReShade not found (or
incompatible)").

## Config (`SDIMEFix.ini`)

`[General] DisableIME`, `BlockLanguageSwitch` · `[Overlay] TextInput`, `ImeUI` · `[Debug] Logging`.
While the overlay has text input, the log records every `WM_IME_*` message; that's intentionally verbose
for diagnosis.

## In-game test checklist

Exclusive fullscreen, Chinese IME in Chinese mode: spam Shift, walk with WASD, press Win+Space → no
popups, no drop to windowed, no camera jump. Open ReShade (Home), type `你好` / `abc你好123` into the
search box → correct text, candidates drawn under the box, still fullscreen; close ReShade → log shows
`IME detached`.
