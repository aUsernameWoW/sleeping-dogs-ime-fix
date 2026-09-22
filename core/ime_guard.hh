#pragma once

#include <Windows.h>

namespace ime
{
	// Hooks the game's CreateWindowExA import and attaches to any top-level
	// window the game already has. Safe to call from DllMain.
	bool Install(HMODULE self);

	// Phase 2 hook point: an overlay (e.g. ReShade) wants text input, so the IME
	// may stay attached to the game window while this is true.
	void SetAllowed(bool allowed);
	bool IsAllowed();

	// True when one of the game's tracked windows is the foreground window.
	bool IsGameForeground();
}
