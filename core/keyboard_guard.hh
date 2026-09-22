#pragma once

namespace keyboard
{
	// Starts a low-level keyboard hook on its own thread that swallows Win+Space
	// while the game window is in the foreground. Safe to call from DllMain.
	bool Install();
}
