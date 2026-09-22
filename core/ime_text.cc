#include "ime_text.hh"

#include "log.hh"

#include <imm.h>

// Why committed text doesn't take the normal route:
// Sleeping Dogs' window is ANSI (CreateWindowExA/PeekMessageA), so any WM_CHAR
// the game's message loop pulls out is converted to the input locale's DBCS
// code page (e.g. '你' U+4F60 -> 0xE3C4). ReShade reads WM_CHAR's wParam as
// UTF-16, so Chinese text turns into unknown glyphs ('?'). This happens no
// matter how the WM_CHAR is posted, so instead we read the result as UTF-16
// straight from the IME and hand it to the overlay, which feeds it to ImGui.

namespace imetext
{
	static constexpr int kDefaultPageSize = 9;

	static SRWLOCK gLock = SRWLOCK_INIT;
	static Snapshot gState;
	static std::wstring gCommitted;

	static std::wstring GetCompositionString(HIMC himc, DWORD index)
	{
		const LONG bytes = ImmGetCompositionStringW(himc, index, nullptr, 0);
		if (bytes <= 0) {
			return {};
		}

		std::wstring text(static_cast<size_t>(bytes) / sizeof(wchar_t), L'\0');
		ImmGetCompositionStringW(himc, index, text.data(), static_cast<DWORD>(bytes));
		return text;
	}

	// Byte offset in the UTF-8 form of 'text' for UTF-16 index 'index'.
	static int Utf8Offset(const std::wstring& text, int index)
	{
		if (index <= 0) {
			return 0;
		}
		if (index > static_cast<int>(text.size())) {
			index = static_cast<int>(text.size());
		}
		return WideCharToMultiByte(CP_UTF8, 0, text.data(), index, nullptr, 0, nullptr, nullptr);
	}

	static void UpdateComposition(HWND hwnd, LPARAM flags)
	{
		HIMC himc = ImmGetContext(hwnd);
		if (!himc) {
			return;
		}

		if (flags & GCS_RESULTSTR)
		{
			const std::wstring result = GetCompositionString(himc, GCS_RESULTSTR);
			LOG("IME result: '%s' (%zu code units)", logger::ToUtf8(result.c_str()).c_str(), result.size());

			AcquireSRWLockExclusive(&gLock);
			gCommitted += result;
			ReleaseSRWLockExclusive(&gLock);
		}

		if (flags & (GCS_COMPSTR | GCS_CURSORPOS))
		{
			const std::wstring composition = GetCompositionString(himc, GCS_COMPSTR);
			const LONG cursor = ImmGetCompositionStringW(himc, GCS_CURSORPOS, nullptr, 0);

			AcquireSRWLockExclusive(&gLock);
			gState.mComposition = logger::ToUtf8(composition.c_str());
			gState.mCaret = Utf8Offset(composition, cursor >= 0 ? cursor : static_cast<int>(composition.size()));
			ReleaseSRWLockExclusive(&gLock);
		}
		else if (flags & GCS_RESULTSTR)
		{
			// Result without a new composition string: the composition is done.
			AcquireSRWLockExclusive(&gLock);
			gState.mComposition.clear();
			gState.mCaret = 0;
			ReleaseSRWLockExclusive(&gLock);
		}

		ImmReleaseContext(hwnd, himc);
	}

	static void UpdateCandidates(HWND hwnd)
	{
		std::vector<std::string> candidates;
		int selection = -1;

		if (HIMC himc = ImmGetContext(hwnd))
		{
			const DWORD size = ImmGetCandidateListW(himc, 0, nullptr, 0);
			if (size >= sizeof(CANDIDATELIST))
			{
				std::vector<BYTE> buffer(size);
				auto list = reinterpret_cast<CANDIDATELIST*>(buffer.data());

				if (ImmGetCandidateListW(himc, 0, list, size) && list->dwCount > 0)
				{
					const DWORD pageSize = list->dwPageSize ? list->dwPageSize : kDefaultPageSize;
					const DWORD pageStart = list->dwPageStart < list->dwCount ? list->dwPageStart : 0;
					const DWORD pageEnd = (pageStart + pageSize < list->dwCount) ? pageStart + pageSize : list->dwCount;

					for (DWORD i = pageStart; i < pageEnd; ++i)
					{
						const DWORD offset = list->dwOffset[i];
						if (offset >= size) {
							break;
						}
						candidates.push_back(logger::ToUtf8(reinterpret_cast<const wchar_t*>(buffer.data() + offset)));
					}

					if (list->dwSelection >= pageStart && list->dwSelection < pageEnd) {
						selection = static_cast<int>(list->dwSelection - pageStart);
					}
				}
			}

			ImmReleaseContext(hwnd, himc);
		}

		AcquireSRWLockExclusive(&gLock);
		const bool wasEmpty = gState.mCandidates.empty();
		gState.mCandidates = std::move(candidates);
		gState.mSelection = selection;
		const size_t count = gState.mCandidates.size();
		ReleaseSRWLockExclusive(&gLock);

		if (wasEmpty && count) {
			LOG("IME candidates available via IMM (%zu on page)", count);
		}
	}

	static void ClearCandidates()
	{
		AcquireSRWLockExclusive(&gLock);
		gState.mCandidates.clear();
		gState.mSelection = -1;
		ReleaseSRWLockExclusive(&gLock);
	}

	static void Reset()
	{
		AcquireSRWLockExclusive(&gLock);
		gState = Snapshot{};
		ReleaseSRWLockExclusive(&gLock);
	}

	bool HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM& lParam, LRESULT& result)
	{
		result = 0;

		switch (msg)
		{
		case WM_IME_SETCONTEXT:
			// Tell the IME we draw composition and candidates ourselves.
			if (wParam) {
				lParam &= ~ISC_SHOWUIALL;
			}
			return false;

		case WM_IME_STARTCOMPOSITION:
			AcquireSRWLockExclusive(&gLock);
			gState.mComposing = true;
			gState.mComposition.clear();
			gState.mCaret = 0;
			ReleaseSRWLockExclusive(&gLock);
			return true; // DefWindowProc would open the IME's composition window.

		case WM_IME_COMPOSITION:
			UpdateComposition(hwnd, lParam);
			return true; // DefWindowProc would emit ANSI WM_IME_CHAR/WM_CHAR.

		case WM_IME_ENDCOMPOSITION:
			Reset();
			return true;

		case WM_IME_CHAR:
			return true; // Already taken as UTF-16 from GCS_RESULTSTR.

		case WM_IME_NOTIFY:
			switch (wParam)
			{
			case IMN_OPENCANDIDATE:
			case IMN_CHANGECANDIDATE:
				UpdateCandidates(hwnd);
				return true;

			case IMN_CLOSECANDIDATE:
				ClearCandidates();
				return true;
			}
			return false;
		}

		return false;
	}

	void Cancel(HWND hwnd)
	{
		if (HIMC himc = ImmGetContext(hwnd))
		{
			ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
			ImmReleaseContext(hwnd, himc);
		}
		Reset();
	}

	std::wstring TakeCommitted()
	{
		AcquireSRWLockExclusive(&gLock);
		std::wstring text = std::move(gCommitted);
		gCommitted.clear();
		ReleaseSRWLockExclusive(&gLock);
		return text;
	}

	Snapshot GetSnapshot()
	{
		AcquireSRWLockShared(&gLock);
		Snapshot copy = gState;
		ReleaseSRWLockShared(&gLock);
		return copy;
	}
}
