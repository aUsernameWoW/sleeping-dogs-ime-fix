#include "reshade_overlay.hh"

#include "config.hh"
#include "ime_guard.hh"
#include "ime_text.hh"
#include "log.hh"

// Must match ReShade's own Dear ImGui build (see deps/ImGui.props in ReShade).
#define ImTextureID ImU64
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui.h>
#include <imgui_internal.h> // ImGuiContext::PlatformImeDataPrev (caret position), layout pinned by the version check.
#include <reshade.hpp>

static_assert(IMGUI_VERSION_NUM == 19250, "ImGui headers must match the ReShade build (6.8.0 uses 1.92.5)");

extern "C" __declspec(dllexport) const char* NAME = "SDInputFix";
extern "C" __declspec(dllexport) const char* DESCRIPTION = "Keeps the IME from kicking Sleeping Dogs out of fullscreen, and lets you type Chinese into ReShade.";

// ReShade draws its UI inside the game's swap chain, using the game window for
// input. So "typing into the overlay" is just the game window receiving IME
// input. We re-attach the IME only while ImGui has an active text box
// (io.WantTextInput), the same way Minecraft InputFix only enables the IME
// while a text field is focused.

namespace overlay
{
	static bool gRegisteredDraw = false;
	static bool gLastWantText = false;

	static ImU32 ToU32(const ImVec4& color, float alphaMul = 1.0f)
	{
		auto channel = [](float v) { return static_cast<ImU32>((v < 0.f ? 0.f : v > 1.f ? 1.f : v) * 255.0f + 0.5f); };
		return IM_COL32(channel(color.x), channel(color.y), channel(color.z), channel(color.w * alphaMul));
	}

	static void DrawImeUI(const ImGuiPlatformImeData& ime)
	{
		const imetext::Snapshot state = imetext::GetSnapshot();
		if (state.mComposition.empty() && state.mCandidates.empty()) {
			return;
		}

		const ImGuiIO& io = ImGui::GetIO();
		const ImGuiStyle& style = ImGui::GetStyle();
		ImDrawList* draw = ImGui::GetForegroundDrawList(static_cast<ImGuiViewport*>(nullptr));

		const float lineHeight = ImGui::GetFontSize();
		const ImVec2 padding = style.FramePadding;
		const float spacing = style.ItemSpacing.x * 1.5f;

		const ImU32 background = ToU32(style.Colors[ImGuiCol_PopupBg], 0.97f);
		const ImU32 border = ToU32(style.Colors[ImGuiCol_Border]);
		const ImU32 text = ToU32(style.Colors[ImGuiCol_Text]);
		const ImU32 dimText = ToU32(style.Colors[ImGuiCol_TextDisabled]);
		const ImU32 highlight = ToU32(style.Colors[ImGuiCol_HeaderActive]);

		// Lay out: composition on the first row, candidates on the second.
		std::string candidateLabels[16];
		const int candidateCount = static_cast<int>(state.mCandidates.size() < 16 ? state.mCandidates.size() : 16);
		float candidatesWidth = 0.0f;
		for (int i = 0; i < candidateCount; ++i)
		{
			candidateLabels[i] = std::to_string(i + 1) + " " + state.mCandidates[i];
			candidatesWidth += ImGui::CalcTextSize(candidateLabels[i].c_str(), nullptr, false, -1.0f).x + (i ? spacing : 0.0f);
		}

		const float compositionWidth = ImGui::CalcTextSize(state.mComposition.c_str(), nullptr, false, -1.0f).x;
		const int rows = (state.mComposition.empty() ? 0 : 1) + (candidateCount ? 1 : 0);
		const float boxWidth = (compositionWidth > candidatesWidth ? compositionWidth : candidatesWidth) + padding.x * 2.0f;
		const float boxHeight = rows * lineHeight + (rows - 1) * padding.y + padding.y * 2.0f;

		// Below the text caret; flip above it / clamp so it never leaves the screen.
		const float caretHeight = ime.InputLineHeight > 0.0f ? ime.InputLineHeight : lineHeight;
		ImVec2 pos(ime.InputPos.x, ime.InputPos.y + caretHeight + 2.0f);
		if (pos.y + boxHeight > io.DisplaySize.y) {
			pos.y = ime.InputPos.y - boxHeight - 2.0f;
		}
		if (pos.x + boxWidth > io.DisplaySize.x) {
			pos.x = io.DisplaySize.x - boxWidth;
		}
		if (pos.x < 0.0f) {
			pos.x = 0.0f;
		}
		if (pos.y < 0.0f) {
			pos.y = 0.0f;
		}

		draw->AddRectFilled(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), background, style.PopupRounding, 0);
		draw->AddRect(pos, ImVec2(pos.x + boxWidth, pos.y + boxHeight), border, style.PopupRounding, 0, 1.0f);

		ImVec2 cursor(pos.x + padding.x, pos.y + padding.y);

		if (!state.mComposition.empty())
		{
			const char* begin = state.mComposition.c_str();
			draw->AddText(cursor, text, begin, nullptr);

			// Underline the composition and show the caret inside it.
			draw->AddLine(ImVec2(cursor.x, cursor.y + lineHeight), ImVec2(cursor.x + compositionWidth, cursor.y + lineHeight), dimText, 1.0f);
			const float caretX = cursor.x + ImGui::CalcTextSize(begin, begin + state.mCaret, false, -1.0f).x;
			draw->AddLine(ImVec2(caretX, cursor.y), ImVec2(caretX, cursor.y + lineHeight), text, 1.0f);

			cursor.y += lineHeight + padding.y;
		}

		for (int i = 0; i < candidateCount; ++i)
		{
			const char* label = candidateLabels[i].c_str();
			const float width = ImGui::CalcTextSize(label, nullptr, false, -1.0f).x;

			if (i == state.mSelection) {
				draw->AddRectFilled(ImVec2(cursor.x - 2.0f, cursor.y), ImVec2(cursor.x + width + 2.0f, cursor.y + lineHeight), highlight, style.FrameRounding, 0);
			}

			draw->AddText(cursor, text, label, nullptr);
			cursor.x += width + spacing;
		}
	}

	// Same as ImGuiIO::AddInputCharacter(), which isn't in ReShade's function
	// table. Allocation goes through ReShade's ImGui::MemAlloc, so the queue's
	// memory stays on ReShade's heap.
	static void AddInputCharacter(ImGuiIO& io, unsigned int c)
	{
		if (c == 0 || !io.AppAcceptingEvents || !io.Ctx) {
			return;
		}

		ImGuiContext& g = *io.Ctx;
		ImGuiInputEvent e;
		e.Type = ImGuiInputEventType_Text;
		e.Source = ImGuiInputSource_Keyboard;
		e.EventId = g.InputEventsNextEventId++;
		e.Text.Char = c;
		g.InputEventsQueue.push_back(e);
	}

	static void FeedCommittedText(ImGuiIO& io)
	{
		const std::wstring text = imetext::TakeCommitted();

		for (size_t i = 0; i < text.size(); ++i)
		{
			unsigned int c = text[i];
			if ((c & 0xFC00) == 0xD800 && i + 1 < text.size() && (text[i + 1] & 0xFC00) == 0xDC00)
			{
				c = ((c - 0xD800) << 10) + (text[i + 1] - 0xDC00) + 0x10000;
				++i;
			}
			AddInputCharacter(io, c);
		}
	}

	static void OnOverlay(reshade::api::effect_runtime*)
	{
		ImGuiIO& io = ImGui::GetIO();
		const bool wantText = io.WantTextInput;

		if (wantText != gLastWantText)
		{
			gLastWantText = wantText;
			LOG("ReShade text input %s", wantText ? "active -> IME on" : "inactive -> IME off");
			ime::SetAllowed(wantText);
		}

		if (wantText) {
			FeedCommittedText(io);
		}

		if (wantText && gConfig.mOverlayImeUI && io.Ctx) {
			DrawImeUI(io.Ctx->PlatformImeDataPrev);
		}
	}

	static void SetDrawRegistered(bool registered)
	{
		if (registered == gRegisteredDraw) {
			return;
		}

		gRegisteredDraw = registered;
		if (registered) {
			reshade::register_event<reshade::addon_event::reshade_overlay>(OnOverlay);
		}
		else {
			reshade::unregister_event<reshade::addon_event::reshade_overlay>(OnOverlay);
		}
	}

	static bool OnOpenOverlay(reshade::api::effect_runtime*, bool open, reshade::api::input_source)
	{
		LOG("ReShade overlay %s", open ? "opened" : "closed");

		// Registering 'reshade_overlay' stops ReShade from skipping its UI pass,
		// so only listen while the overlay is actually open.
		SetDrawRegistered(open);

		if (!open) {
			imetext::TakeCommitted(); // Don't replay stale text into the next text box.
		}

		if (!open && gLastWantText)
		{
			gLastWantText = false;
			ime::SetAllowed(false);
		}

		return false; // Don't block the overlay from opening/closing.
	}

	bool Install(HMODULE self)
	{
		if (!gConfig.mOverlayTextInput) {
			return false;
		}

		if (!reshade::register_addon(self))
		{
			LOG("ReShade not found (or incompatible), overlay text input disabled");
			return false;
		}

		reshade::register_event<reshade::addon_event::reshade_open_overlay>(OnOpenOverlay);
		LOG("Registered as ReShade add-on");
		return true;
	}
}
