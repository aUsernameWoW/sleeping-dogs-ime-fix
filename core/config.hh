#pragma once

#include <string>

struct Config
{
	// Detach the IME from the game window so typing/Shift never pops IME UI in-game.
	bool mDisableIME = true;

	// Block Ctrl+Shift / Alt+Shift (WM_INPUTLANGCHANGEREQUEST) and Win+Space (LL keyboard hook) while the game is in front.
	bool mBlockLanguageSwitch = true;

	// Re-attach the IME while a ReShade text box is active.
	bool mOverlayTextInput = true;

	// While typing into an overlay, hide the IME's own popups and draw composition/candidates inside the overlay.
	bool mOverlayImeUI = true;

	// Write SDInputFix.log next to the .asi.
	bool mLogging = true;
};

extern Config gConfig;

namespace config
{
	// Loads <dir>\SDInputFix.ini, writing a default one if it doesn't exist.
	void Load(const std::wstring& dir);
}
