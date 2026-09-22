#pragma once

#include <Windows.h>

#include <string>
#include <vector>

// IME composition handling for overlays (ReShade) drawn inside the game window.
//
// While an overlay wants text input the IME is attached again, but its own UI
// (composition + candidate windows) is suppressed, since those popups are what
// knock the game out of exclusive fullscreen. We keep the composition and
// candidate state here so the overlay can draw it itself.
namespace imetext
{
	struct Snapshot
	{
		bool mComposing = false;
		std::string mComposition; // UTF-8
		int mCaret = 0;           // Caret position in mComposition, in bytes.

		std::vector<std::string> mCandidates; // Current page only, UTF-8.
		int mSelection = -1;                  // Index into mCandidates, -1 if not on this page.
	};

	// Called from the game window's wndproc while an overlay wants text input.
	// Returns true if the message was consumed (result in 'result'). May modify
	// 'lParam' for messages it passes on.
	bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM& lParam, LRESULT& result);

	// Cancels any in-flight composition (must run on the window's thread).
	void Cancel(HWND hwnd);

	Snapshot GetSnapshot();

	// Returns and clears the text committed by the IME since the last call.
	// The overlay feeds it straight into its UI (see reshade_overlay.cc).
	std::wstring TakeCommitted();
}
