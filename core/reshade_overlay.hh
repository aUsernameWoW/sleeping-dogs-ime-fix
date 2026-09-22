#pragma once

#include <Windows.h>

namespace overlay
{
	// Registers with ReShade (if it is loaded) as an add-on, so the IME is
	// re-enabled while a ReShade text box is active. Safe to call from DllMain.
	bool Install(HMODULE self);
}
