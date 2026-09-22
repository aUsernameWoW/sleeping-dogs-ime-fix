#include "keyboard_guard.hh"

#include "config.hh"
#include "ime_guard.hh"
#include "log.hh"

#include <Windows.h>

// Win+Space (switch input method) is handled by the shell, so the game window
// never sees a WM_INPUTLANGCHANGEREQUEST for it and ime_guard can't block it.
// It pops the input switcher flyout over the game and makes the camera jump.
//
// A low-level keyboard hook sees keys before the shell's hotkey handling, so
// we drop Space while a Win key is held and the game is in the foreground.
// Dropping only Space would leave the shell seeing a lone Win press+release and
// opening the Start menu, so we also inject a no-op key (VK 0xFF) in between;
// the shell then treats Win as "used in a combination", the same trick
// PowerToys uses.

namespace keyboard
{
	static constexpr ULONG_PTR kInjectedMarker = 0x53444946; // 'SDIF'
	static constexpr WORD kDummyKey = 0xFF;

	static HHOOK gHook = nullptr;
	static bool gSwallowingSpace = false;

	static bool IsWinDown()
	{
		return (GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000);
	}

	static void SendDummyKey()
	{
		INPUT inputs[2] = {};
		for (INPUT& input : inputs)
		{
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = kDummyKey;
			input.ki.dwExtraInfo = kInjectedMarker;
		}
		inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(2, inputs, sizeof(INPUT));
	}

	static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam)
	{
		if (code == HC_ACTION)
		{
			const KBDLLHOOKSTRUCT* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

			if (key->vkCode == VK_SPACE && key->dwExtraInfo != kInjectedMarker)
			{
				const bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

				if (down && IsWinDown() && !ime::IsAllowed() && ime::IsGameForeground())
				{
					if (!gSwallowingSpace) {
						LOG("Blocked Win+Space");
					}
					gSwallowingSpace = true;
					SendDummyKey();
					return 1;
				}

				// Swallow the matching key-up too, even if Win was released first.
				if (!down && gSwallowingSpace)
				{
					gSwallowingSpace = false;
					return 1;
				}
			}
		}

		return CallNextHookEx(gHook, code, wParam, lParam);
	}

	// Low-level hooks are called on the installing thread's message loop, so give
	// the hook its own thread: the game's thread stalls during loads, and Windows
	// silently drops LL hooks that time out.
	static DWORD WINAPI HookThread(LPVOID)
	{
		gHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleW(nullptr), 0);
		LOG("Keyboard hook %p (err=%lu)", gHook, gHook ? 0 : GetLastError());
		if (!gHook) {
			return 0;
		}

		MSG msg;
		while (GetMessageW(&msg, nullptr, 0, 0) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		UnhookWindowsHookEx(gHook);
		return 0;
	}

	bool Install()
	{
		if (!gConfig.mBlockLanguageSwitch) {
			return true;
		}

		// Can't wait on the thread here (DllMain); it starts once the loader lock is released.
		HANDLE thread = CreateThread(nullptr, 0, HookThread, nullptr, 0, nullptr);
		if (!thread) {
			return false;
		}

		CloseHandle(thread);
		return true;
	}
}
