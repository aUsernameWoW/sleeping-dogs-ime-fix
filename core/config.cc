#include "config.hh"

#include <Windows.h>

#include <cstdio>

Config gConfig;

namespace config
{
	static constexpr char kDefaultIni[] =
		"; SDInputFix 配置 / configuration\n"
		"; 1 = 开启 (on), 0 = 关闭 (off)\n"
		"\n"
		"[General]\n"
		"; 游戏内禁用输入法，避免打字/按 Shift 弹出输入法界面导致全屏掉成窗口。\n"
		"; Detach the IME from the game window so it can't pop up UI in-game.\n"
		"DisableIME = 1\n"
		"\n"
		"; 游戏在前台时屏蔽 Ctrl+Shift / Alt+Shift / Win+Space 切换输入法。\n"
		"; Block input-method switching hotkeys (incl. Win+Space) while the game is in front.\n"
		"BlockLanguageSwitch = 1\n"
		"\n"
		"[Overlay]\n"
		"; 在 ReShade 的文本框里输入时临时启用输入法（可输入中文）。\n"
		"; Re-enable the IME while a ReShade text box is active.\n"
		"TextInput = 1\n"
		"\n"
		"; 输入时隐藏输入法自带的拼音框/候选窗，改为画在 ReShade 界面里（避免全屏掉成窗口）。\n"
		"; Hide the IME's own popups and draw composition/candidates inside the overlay.\n"
		"ImeUI = 1\n"
		"\n"
		"[Debug]\n"
		"; 在 .asi 旁边写 SDInputFix.log，用于排查是什么窗口抢走了焦点。\n"
		"; Write SDInputFix.log (records which window stole focus).\n"
		"Logging = 1\n";

	static bool ReadBool(const wchar_t* path, const wchar_t* section, const wchar_t* key, bool fallback)
	{
		return GetPrivateProfileIntW(section, key, fallback ? 1 : 0, path) != 0;
	}

	void Load(const std::wstring& dir)
	{
		const std::wstring path = dir + L"\\SDInputFix.ini";

		if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			FILE* file = nullptr;
			if (_wfopen_s(&file, path.c_str(), L"wb") == 0 && file)
			{
				fwrite(kDefaultIni, 1, sizeof(kDefaultIni) - 1, file);
				fclose(file);
			}
		}

		gConfig.mDisableIME = ReadBool(path.c_str(), L"General", L"DisableIME", gConfig.mDisableIME);
		gConfig.mBlockLanguageSwitch = ReadBool(path.c_str(), L"General", L"BlockLanguageSwitch", gConfig.mBlockLanguageSwitch);
		gConfig.mOverlayTextInput = ReadBool(path.c_str(), L"Overlay", L"TextInput", gConfig.mOverlayTextInput);
		gConfig.mOverlayImeUI = ReadBool(path.c_str(), L"Overlay", L"ImeUI", gConfig.mOverlayImeUI);
		gConfig.mLogging = ReadBool(path.c_str(), L"Debug", L"Logging", gConfig.mLogging);
	}
}
