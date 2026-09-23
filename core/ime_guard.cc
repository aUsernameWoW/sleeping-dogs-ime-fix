#include "ime_guard.hh"

#include "config.hh"
#include "ime_text.hh"
#include "log.hh"

#include <imm.h>

#include <atomic>

// Why this exists:
// With a Chinese IME active, pressing Shift (中/英 toggle) or typing WASD in
// Chinese mode makes Windows show IME UI (mode flyout, composition/candidate
// windows). That UI takes focus/foreground from the game, DXGI drops out of
// exclusive fullscreen, and Sleeping Dogs never goes back.
//
// Fix: keep the IME detached from the game's windows (ImmAssociateContextEx
// with a NULL context), like the Minecraft 1.7.10 InputFix mods do outside of
// text fields. IMM calls only work on the window's own thread, so detach/attach
// happens inside thread-local message hooks, which also survive the game or
// other mods replacing the wndproc. A wndproc subclass is used only for what
// hooks can't do while typing into an overlay: rewriting WM_IME_SETCONTEXT and
// keeping WM_IME_* away from DefWindowProc (see ime_text.cc).

namespace ime
{
	static constexpr int kMaxWindows = 16;
	static constexpr int kMaxThreads = 8;

	struct TrackedWindow
	{
		HWND mHwnd;
		bool mDetached;
		WNDPROC mOriginalProc;
	};

	static SRWLOCK gLock = SRWLOCK_INIT;
	static TrackedWindow gWindows[kMaxWindows];
	static DWORD gThreads[kMaxThreads];
	static HHOOK gHooks[kMaxThreads][2];
	static int gWindowCount = 0;
	static int gThreadCount = 0;

	static HMODULE gSelf = nullptr;
	static UINT gEnforceMsg = 0;
	static std::atomic<bool> gAllowed = false;

	using CreateWindowExA_t = decltype(&CreateWindowExA);
	static CreateWindowExA_t gCreateWindowExA = nullptr;

	//------------------------------------------------------------------------
	//	Helpers
	//------------------------------------------------------------------------

	static TrackedWindow* FindWindowLocked(HWND hwnd)
	{
		for (int i = 0; i < gWindowCount; ++i)
		{
			if (gWindows[i].mHwnd == hwnd) {
				return &gWindows[i];
			}
		}
		return nullptr;
	}

	static bool IsTracked(HWND hwnd)
	{
		AcquireSRWLockShared(&gLock);
		const bool tracked = FindWindowLocked(hwnd) != nullptr;
		ReleaseSRWLockShared(&gLock);
		return tracked;
	}

	static void DescribeWindow(HWND hwnd, char* out, size_t outSize)
	{
		if (!hwnd)
		{
			snprintf(out, outSize, "<none>");
			return;
		}

		wchar_t className[128] = {};
		GetClassNameW(hwnd, className, ARRAYSIZE(className));

		wchar_t title[128] = {};
		InternalGetWindowText(hwnd, title, ARRAYSIZE(title)); // Never sends messages (safe under loader lock).

		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);

		wchar_t image[MAX_PATH] = L"?";
		if (HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid))
		{
			DWORD size = ARRAYSIZE(image);
			if (QueryFullProcessImageNameW(process, 0, image, &size))
			{
				if (const wchar_t* slash = wcsrchr(image, L'\\')) {
					memmove(image, slash + 1, (wcslen(slash + 1) + 1) * sizeof(wchar_t));
				}
			}
			CloseHandle(process);
		}

		snprintf(out, outSize, "%p class='%s' title='%s' pid=%lu exe='%s'", hwnd,
			logger::ToUtf8(className).c_str(), logger::ToUtf8(title).c_str(), pid, logger::ToUtf8(image).c_str());
	}

	//------------------------------------------------------------------------
	//	IME association (must run on the window's thread)
	//------------------------------------------------------------------------

	// Recorded before the IMM call, since ImmAssociateContextEx sends
	// WM_IME_SETCONTEXT to the window synchronously.
	static void SetDetached(HWND hwnd, bool detached)
	{
		AcquireSRWLockExclusive(&gLock);
		if (TrackedWindow* w = FindWindowLocked(hwnd)) {
			w->mDetached = detached;
		}
		ReleaseSRWLockExclusive(&gLock);
	}

	static void Enforce(HWND hwnd)
	{
		if (!gConfig.mDisableIME) {
			return;
		}

		const bool wantDetached = !gAllowed.load();

		AcquireSRWLockExclusive(&gLock);
		TrackedWindow* window = FindWindowLocked(hwnd);
		const bool isDetached = window && window->mDetached;
		ReleaseSRWLockExclusive(&gLock);

		if (!window) {
			return;
		}

		if (wantDetached)
		{
			// Something (focus change, TSF) may have handed the window a context again.
			HIMC current = ImmGetContext(hwnd);
			if (current) {
				ImmReleaseContext(hwnd, current);
			}

			if (isDetached && !current) {
				return;
			}

			if (!isDetached) {
				imetext::Cancel(hwnd);
			}

			SetDetached(hwnd, true);
			const BOOL ok = ImmAssociateContextEx(hwnd, nullptr, 0);
			LOG("IME detached from %p (%s)%s", hwnd, ok ? "ok" : "FAILED", isDetached ? " [re-applied]" : "");
		}
		else
		{
			if (!isDetached) {
				return;
			}

			SetDetached(hwnd, false);
			const BOOL ok = ImmAssociateContextEx(hwnd, nullptr, IACE_DEFAULT);
			LOG("IME restored on %p (%s)", hwnd, ok ? "ok" : "FAILED");
		}
	}

	// Defers Enforce() to the window's thread via its message queue. Used from
	// other threads and from inside SendMessage-time hooks, where re-entering
	// IMM (which itself sends WM_IME_SETCONTEXT) is best avoided.
	static void RequestEnforce(HWND hwnd)
	{
		PostMessageW(hwnd, gEnforceMsg, 0, 0);
	}

	//------------------------------------------------------------------------
	//	Thread message hooks
	//------------------------------------------------------------------------

	static LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam)
	{
		if (code == HC_ACTION)
		{
			const CWPSTRUCT* cwp = reinterpret_cast<const CWPSTRUCT*>(lParam);

			switch (cwp->message)
			{
			case WM_ACTIVATEAPP:
				if (IsTracked(cwp->hwnd))
				{
					if (cwp->wParam)
					{
						LOG("WM_ACTIVATEAPP active hwnd=%p", cwp->hwnd);
					}
					else
					{
						char info[512];
						DescribeWindow(GetForegroundWindow(), info, sizeof(info));
						LOG("WM_ACTIVATEAPP inactive hwnd=%p, other thread=%lu, foreground now: %s", cwp->hwnd, static_cast<DWORD>(cwp->lParam), info);
					}
				}
				break;

			case WM_ACTIVATE:
				if (IsTracked(cwp->hwnd))
				{
					char info[512];
					DescribeWindow(reinterpret_cast<HWND>(cwp->lParam), info, sizeof(info));
					LOG("WM_ACTIVATE state=%u minimized=%u hwnd=%p other: %s", LOWORD(cwp->wParam), HIWORD(cwp->wParam), cwp->hwnd, info);

					if (LOWORD(cwp->wParam) != WA_INACTIVE) {
						RequestEnforce(cwp->hwnd);
					}
				}
				break;

			case WM_SETFOCUS:
				if (IsTracked(cwp->hwnd)) {
					RequestEnforce(cwp->hwnd);
				}
				break;

			case WM_KILLFOCUS:
				if (IsTracked(cwp->hwnd))
				{
					char info[512];
					DescribeWindow(reinterpret_cast<HWND>(cwp->wParam), info, sizeof(info));
					LOG("WM_KILLFOCUS hwnd=%p new focus: %s", cwp->hwnd, info);
				}
				break;

			case WM_IME_SETCONTEXT:
				if (cwp->wParam && IsTracked(cwp->hwnd))
				{
					LOG("WM_IME_SETCONTEXT(active) hwnd=%p flags=0x%llx", cwp->hwnd, static_cast<unsigned long long>(cwp->lParam));
					RequestEnforce(cwp->hwnd);
				}
				break;

			case WM_IME_STARTCOMPOSITION:
				if (IsTracked(cwp->hwnd)) {
					LOG("WM_IME_STARTCOMPOSITION hwnd=%p (IME %s)", cwp->hwnd, gAllowed.load() ? "allowed" : "SHOULD BE DETACHED");
				}
				break;

			case WM_INPUTLANGCHANGE:
				if (IsTracked(cwp->hwnd)) {
					LOG("WM_INPUTLANGCHANGE hwnd=%p hkl=%p", cwp->hwnd, reinterpret_cast<void*>(cwp->lParam));
				}
				break;

			case WM_SIZE:
				if (IsTracked(cwp->hwnd)) {
					LOG("WM_SIZE hwnd=%p type=%llu %ux%u", cwp->hwnd, static_cast<unsigned long long>(cwp->wParam), LOWORD(cwp->lParam), HIWORD(cwp->lParam));
				}
				break;
			}
		}

		return CallNextHookEx(nullptr, code, wParam, lParam);
	}

	static LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
	{
		if (code == HC_ACTION)
		{
			MSG* msg = reinterpret_cast<MSG*>(lParam);

			if (msg->message == gEnforceMsg && gEnforceMsg != 0)
			{
				if (wParam == PM_REMOVE) {
					Enforce(msg->hwnd);
				}
				msg->message = WM_NULL; // Never let the game see our private message.
			}
			else if ((msg->message == WM_CHAR || msg->message == WM_IME_CHAR) && wParam == PM_REMOVE && gAllowed.load())
			{
				LOG("posted %s 0x%llx", msg->message == WM_CHAR ? "WM_CHAR" : "WM_IME_CHAR", static_cast<unsigned long long>(msg->wParam));
			}
			else if (msg->message == WM_INPUTLANGCHANGEREQUEST && gConfig.mBlockLanguageSwitch && !gAllowed.load() && IsTracked(msg->hwnd))
			{
				if (wParam == PM_REMOVE) {
					LOG("Blocked WM_INPUTLANGCHANGEREQUEST hwnd=%p hkl=%p", msg->hwnd, reinterpret_cast<void*>(msg->lParam));
				}
				msg->message = WM_NULL;
			}
		}

		return CallNextHookEx(nullptr, code, wParam, lParam);
	}

	//------------------------------------------------------------------------
	//	Window tracking
	//------------------------------------------------------------------------

	static void HookThread(DWORD threadId)
	{
		AcquireSRWLockExclusive(&gLock);
		for (int i = 0; i < gThreadCount; ++i)
		{
			if (gThreads[i] == threadId)
			{
				ReleaseSRWLockExclusive(&gLock);
				return;
			}
		}

		if (gThreadCount >= kMaxThreads)
		{
			ReleaseSRWLockExclusive(&gLock);
			LOG("Too many window threads, not hooking thread %lu", threadId);
			return;
		}

		const int slot = gThreadCount++;
		gThreads[slot] = threadId;
		gHooks[slot][0] = SetWindowsHookExW(WH_CALLWNDPROC, CallWndProc, gSelf, threadId);
		gHooks[slot][1] = SetWindowsHookExW(WH_GETMESSAGE, GetMsgProc, gSelf, threadId);
		ReleaseSRWLockExclusive(&gLock);

		LOG("Hooked thread %lu (callwndproc=%p getmessage=%p, err=%lu)", threadId, gHooks[slot][0], gHooks[slot][1], GetLastError());
	}

	static bool IsImeMessage(UINT msg)
	{
		return (msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) || (msg >= WM_IME_SETCONTEXT && msg <= WM_IME_KEYUP);
	}

	static const char* ImeMessageName(UINT msg)
	{
		switch (msg)
		{
		case WM_IME_STARTCOMPOSITION: return "WM_IME_STARTCOMPOSITION";
		case WM_IME_ENDCOMPOSITION: return "WM_IME_ENDCOMPOSITION";
		case WM_IME_COMPOSITION: return "WM_IME_COMPOSITION";
		case WM_IME_SETCONTEXT: return "WM_IME_SETCONTEXT";
		case WM_IME_NOTIFY: return "WM_IME_NOTIFY";
		case WM_IME_CONTROL: return "WM_IME_CONTROL";
		case WM_IME_COMPOSITIONFULL: return "WM_IME_COMPOSITIONFULL";
		case WM_IME_SELECT: return "WM_IME_SELECT";
		case WM_IME_CHAR: return "WM_IME_CHAR";
		case WM_IME_REQUEST: return "WM_IME_REQUEST";
		case WM_IME_KEYDOWN: return "WM_IME_KEYDOWN";
		case WM_IME_KEYUP: return "WM_IME_KEYUP";
		}
		return "WM_IME_?";
	}

	static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		AcquireSRWLockShared(&gLock);
		const TrackedWindow* window = FindWindowLocked(hwnd);
		const WNDPROC original = window ? window->mOriginalProc : nullptr;
		ReleaseSRWLockShared(&gLock);

		if (!original) {
			return DefWindowProcA(hwnd, msg, wParam, lParam);
		}

		const bool allowed = gAllowed.load();
		if (allowed && IsImeMessage(msg)) {
			LOG("wndproc %s wParam=0x%llx lParam=0x%llx", ImeMessageName(msg), static_cast<unsigned long long>(wParam), static_cast<unsigned long long>(lParam));
		}

		if (allowed && gConfig.mOverlayImeUI)
		{
			LRESULT result;
			if (imetext::HandleMessage(hwnd, msg, wParam, lParam, result)) {
				return result;
			}
		}

		return CallWindowProcA(original, hwnd, msg, wParam, lParam);
	}

	static void AttachWindow(HWND hwnd, bool onOwnThread)
	{
		if (!hwnd || (GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_CHILD)) {
			return;
		}

		AcquireSRWLockExclusive(&gLock);
		if (FindWindowLocked(hwnd) || gWindowCount >= kMaxWindows)
		{
			ReleaseSRWLockExclusive(&gLock);
			return;
		}
		TrackedWindow& tracked = gWindows[gWindowCount++];
		tracked = { hwnd, false, nullptr };

		// Subclass with the A variant so the window stays ANSI, as the game expects.
		// Needed to strip ISC_SHOWUI* from WM_IME_SETCONTEXT and to keep IME
		// messages away from DefWindowProc, which a hook can't do.
		tracked.mOriginalProc = reinterpret_cast<WNDPROC>(GetWindowLongPtrA(hwnd, GWLP_WNDPROC));
		SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&SubclassProc));
		ReleaseSRWLockExclusive(&gLock);

		char info[512];
		DescribeWindow(hwnd, info, sizeof(info));
		LOG("Tracking window %s", info);

		HookThread(GetWindowThreadProcessId(hwnd, nullptr));

		if (onOwnThread) {
			Enforce(hwnd);
		}
		else {
			RequestEnforce(hwnd);
		}
	}

	static HWND WINAPI HookedCreateWindowExA(DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style,
		int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
	{
		HWND hwnd = gCreateWindowExA(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param);

		if (hwnd && !(style & WS_CHILD)) {
			AttachWindow(hwnd, true);
		}

		return hwnd;
	}

	//------------------------------------------------------------------------
	//	Import patching
	//------------------------------------------------------------------------

	// Patches the main executable's import of user32!CreateWindowExA. This only
	// catches windows the game itself creates, which is exactly what we want, and
	// plays nicely with ReShade's own export hooks.
	static bool PatchImport(HMODULE module, const char* dllName, const char* funcName, void* replacement, void** original)
	{
		auto base = reinterpret_cast<BYTE*>(module);
		auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
		auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
		const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		if (!dir.VirtualAddress) {
			return false;
		}

		for (auto desc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress); desc->Name; ++desc)
		{
			if (_stricmp(reinterpret_cast<const char*>(base + desc->Name), dllName) != 0 || !desc->OriginalFirstThunk) {
				continue;
			}

			auto names = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->OriginalFirstThunk);
			auto funcs = reinterpret_cast<IMAGE_THUNK_DATA*>(base + desc->FirstThunk);

			for (; names->u1.AddressOfData; ++names, ++funcs)
			{
				if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) {
					continue;
				}

				auto byName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData);
				if (strcmp(reinterpret_cast<const char*>(byName->Name), funcName) != 0) {
					continue;
				}

				DWORD oldProtect;
				if (!VirtualProtect(&funcs->u1.Function, sizeof(funcs->u1.Function), PAGE_READWRITE, &oldProtect)) {
					return false;
				}

				*original = reinterpret_cast<void*>(funcs->u1.Function);
				funcs->u1.Function = reinterpret_cast<ULONG_PTR>(replacement);
				VirtualProtect(&funcs->u1.Function, sizeof(funcs->u1.Function), oldProtect, &oldProtect);
				return true;
			}
		}

		return false;
	}

	static BOOL CALLBACK EnumExistingWindow(HWND hwnd, LPARAM)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		if (pid != GetCurrentProcessId()) {
			return TRUE;
		}

		wchar_t className[64] = {};
		GetClassNameW(hwnd, className, ARRAYSIZE(className));

		// Skip IME/TSF helper windows and other invisible plumbing.
		if (!IsWindowVisible(hwnd) || wcscmp(className, L"IME") == 0 || wcscmp(className, L"MSCTFIME UI") == 0) {
			return TRUE;
		}

		AttachWindow(hwnd, false);
		return TRUE;
	}

	//------------------------------------------------------------------------
	//	Public
	//------------------------------------------------------------------------

	bool Install(HMODULE self)
	{
		gSelf = self;
		gEnforceMsg = RegisterWindowMessageW(L"SDIMEFix.EnforceIME");

		if (!PatchImport(GetModuleHandleW(nullptr), "USER32.dll", "CreateWindowExA", reinterpret_cast<void*>(&HookedCreateWindowExA), reinterpret_cast<void**>(&gCreateWindowExA)))
		{
			LOG("Failed to patch CreateWindowExA import");
			return false;
		}

		LOG("Patched CreateWindowExA import (original=%p)", gCreateWindowExA);

		// In case we were loaded after the game window already exists.
		EnumWindows(EnumExistingWindow, 0);
		return true;
	}

	void SetAllowed(bool allowed)
	{
		if (gAllowed.exchange(allowed) == allowed) {
			return;
		}

		AcquireSRWLockShared(&gLock);
		for (int i = 0; i < gWindowCount; ++i) {
			RequestEnforce(gWindows[i].mHwnd);
		}
		ReleaseSRWLockShared(&gLock);
	}

	bool IsAllowed()
	{
		return gAllowed.load();
	}

	bool IsGameForeground()
	{
		return IsTracked(GetForegroundWindow());
	}
}
