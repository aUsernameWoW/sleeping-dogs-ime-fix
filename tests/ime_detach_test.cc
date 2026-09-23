// Mimics the game: an ANSI window created via the exe's CreateWindowExA import
// and pumped with PeekMessageA. Loads SDIMEFix.asi (argv[1]) and checks the
// IME stays detached and layout-switch requests never reach the window.
// Exit code 0 = pass.
#include <Windows.h>
#include <imm.h>
#include <cstdio>
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "user32.lib")

static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l) { return DefWindowProcA(h, m, w, l); }

static void Pump()
{
	MSG msg;
	for (int i = 0; i < 20; ++i)
	{
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		Sleep(10);
	}
}

static bool HasContext(HWND h)
{
	HIMC c = ImmGetContext(h);
	if (c) {
		ImmReleaseContext(h, c);
	}
	return c != nullptr;
}

static int gFailures = 0;

static void Check(bool ok, const char* what)
{
	printf("[%s] %s\n", ok ? "PASS" : "FAIL", what);
	gFailures += ok ? 0 : 1;
}

int main(int argc, char** argv)
{
	if (argc < 2 || !LoadLibraryA(argv[1]))
	{
		printf("usage: ime_detach_test <path to SDIMEFix.asi> (load failed: %lu)\n", GetLastError());
		return 2;
	}

	WNDCLASSA wc{};
	wc.lpfnWndProc = Proc;
	wc.hInstance = GetModuleHandleA(nullptr);
	wc.lpszClassName = "FakeGame";
	RegisterClassA(&wc);

	HWND h = CreateWindowExA(0, "FakeGame", "Fake", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 320, 240, nullptr, nullptr, wc.hInstance, nullptr);
	Pump();
	Check(!HasContext(h), "IME detached after window creation");

	// Something re-associates the default context; the fix must detach it again on refocus.
	ImmAssociateContextEx(h, nullptr, IACE_DEFAULT);
	SetFocus(nullptr);
	SetFocus(h);
	Pump();
	Check(!HasContext(h), "IME detached again after refocus");

	PostMessageA(h, WM_INPUTLANGCHANGEREQUEST, 0, 0);
	MSG msg;
	bool seen = false;
	while (PeekMessageA(&msg, h, 0, 0, PM_REMOVE)) {
		seen |= msg.message == WM_INPUTLANGCHANGEREQUEST;
	}
	Check(!seen, "WM_INPUTLANGCHANGEREQUEST blocked");

	return gFailures ? 1 : 0;
}
