#include <Windows.h>

#include <string>

#include "core/config.hh"
#include "core/ime_guard.hh"
#include "core/keyboard_guard.hh"
#include "core/log.hh"
#include "core/reshade_overlay.hh"

static std::wstring GetModuleDirectory(HMODULE module)
{
	wchar_t path[MAX_PATH] = {};
	const DWORD length = GetModuleFileNameW(module, path, ARRAYSIZE(path));
	if (length == 0 || length >= ARRAYSIZE(path)) {
		return L".";
	}

	std::wstring dir(path, length);
	const size_t slash = dir.find_last_of(L"\\/");
	return slash == std::wstring::npos ? L"." : dir.substr(0, slash);
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(module);

		// Pin ourselves: the thread hooks and patched import point into this module.
		HMODULE pinned;
		GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&DllMain), &pinned);

		const std::wstring dir = GetModuleDirectory(module);
		config::Load(dir);

		if (gConfig.mLogging) {
			logger::Open(dir + L"\\SDInputFix.log");
		}

		LOG("SDInputFix loaded (DisableIME=%d BlockLanguageSwitch=%d OverlayTextInput=%d OverlayImeUI=%d)",
			gConfig.mDisableIME, gConfig.mBlockLanguageSwitch, gConfig.mOverlayTextInput, gConfig.mOverlayImeUI);

		ime::Install(module);
		keyboard::Install();
		overlay::Install(module);
	}

	return TRUE;
}
