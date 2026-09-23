// MANUAL: sends real Win+Space keystrokes to the desktop while a fake game
// window is in the foreground. Not run by tools\build.ps1 -Test, because a
// failure really switches your input method / opens the Start menu.
// Run it yourself: win_space_manual.exe <path to SDIMEFix.asi>
// Exit code 0 = pass.
#include <Windows.h>
#include <cstdio>
#pragma comment(lib, "user32.lib")

static bool gLangChanged = false;

static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	if (m == WM_INPUTLANGCHANGE) {
		gLangChanged = true;
	}
	return DefWindowProcA(h, m, w, l);
}

static void Pump(int n)
{
	MSG msg;
	for (int i = 0; i < n; ++i)
	{
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		Sleep(10);
	}
}

static void Key(WORD vk, bool up)
{
	INPUT in{};
	in.type = INPUT_KEYBOARD;
	in.ki.wVk = vk;
	in.ki.dwFlags = up ? KEYEVENTF_KEYUP : 0;
	SendInput(1, &in, sizeof(in));
}

int main(int argc, char** argv)
{
	if (argc < 2 || !LoadLibraryA(argv[1]))
	{
		printf("usage: win_space_manual <path to SDIMEFix.asi>\n");
		return 2;
	}

	WNDCLASSA wc{};
	wc.lpfnWndProc = Proc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.lpszClassName = "FakeGame";
	RegisterClassA(&wc);

	HWND h = CreateWindowExA(WS_EX_TOPMOST, "FakeGame", "Fake", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 320, 240, nullptr, nullptr, wc.hInstance, nullptr);
	SetForegroundWindow(h);
	Pump(30);
	if (GetForegroundWindow() != h)
	{
		printf("[SKIP] could not bring the test window to the foreground\n");
		return 2;
	}

	const HKL before = GetKeyboardLayout(0);
	Key(VK_LWIN, false); Pump(5);
	Key(VK_SPACE, false); Pump(5);
	Key(VK_SPACE, true); Pump(5);
	Key(VK_LWIN, true); Pump(80);

	const bool stillForeground = GetForegroundWindow() == h; // False = Start menu / switcher took focus.
	const bool layoutUnchanged = GetKeyboardLayout(0) == before && !gLangChanged;
	printf("[%s] game window kept the foreground\n", stillForeground ? "PASS" : "FAIL");
	printf("[%s] input method unchanged\n", layoutUnchanged ? "PASS" : "FAIL");
	return stillForeground && layoutUnchanged ? 0 : 1;
}
