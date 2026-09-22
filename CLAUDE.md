# SDInputFix

Like the Minecraft 1.7.10 "InputFix" mods, for Sleeping Dogs DE: a Chinese IME can no longer knock the game
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
- `core/config.*`, `core/log.*` — `SDInputFix.ini` / `SDInputFix.log` next to the `.asi`.
- `tests/ime_detach_test.cc` (automated), `tests/win_space_manual.cc` (sends real keys; run by hand).

## Design decisions and why (don't undo without reason)

- **IMM calls only work on the window's own thread.** Everything that changes the association runs on the
  game thread: other threads post a registered `SDInputFix.EnforceIME` message that `GetMsgProc` handles and
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
`PlatformImeDataPrev` against `imgui.cpp`'s `AddInputCharacter`. ReShade refuses add-ons built against a
different ImGui version (`ReShadeGetImGuiFunctionTable` returns null → logged as "ReShade not found (or
incompatible)").

## Config (`SDInputFix.ini`)

`[General] DisableIME`, `BlockLanguageSwitch` · `[Overlay] TextInput`, `ImeUI` · `[Debug] Logging`.
While the overlay has text input, the log records every `WM_IME_*` message; that's intentionally verbose
for diagnosis.

## In-game test checklist

Exclusive fullscreen, Chinese IME in Chinese mode: spam Shift, walk with WASD, press Win+Space → no
popups, no drop to windowed, no camera jump. Open ReShade (Home), type `你好` / `abc你好123` into the
search box → correct text, candidates drawn under the box, still fullscreen; close ReShade → log shows
`IME detached`.
