// Native controls and resources, with an in-memory profile and no desktop input.
#include "../vocabulary.cpp"
#include "test_support.h"
#include <fstream>
#include <filesystem>
using namespace SwashMoji;
void Command(HWND dialog, int id, int notification = BN_CLICKED) { SendMessageW(dialog, WM_COMMAND, MAKEWPARAM(id, notification), 0); }
RECT ControlRect(HWND dialog, int id) {
    RECT value{};
    CHECK(GetWindowRect(GetDlgItem(dialog, id), &value));
    MapWindowPoints(nullptr, dialog, reinterpret_cast<POINT*>(&value), 2);
    return value;
}
int Height(const RECT& value) { return value.bottom - value.top; }
int main(int argc, char** argv) {
    HWND dialog{};
    try {
        CHECK(argc == 2); Catalog catalog; std::ifstream file(std::filesystem::u8path(argv[1]), std::ios::binary); CHECK(catalog.Load(file));
        Profile profile; unsigned saves{}; std::function<bool()> persist = [&] { ++saves; return true; };
        Editor parent{catalog, profile, {}, {}, persist}; CombinationEditor state{parent};
        dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_COMBINATIONS), nullptr, CombinationProc, reinterpret_cast<LPARAM>(&state));
        CHECK(dialog);
        const auto results = ControlRect(dialog, IDC_COMBO_RESULTS);
        const auto variants = ControlRect(dialog, IDC_COMBO_VARIANTS);
        const auto entries = ControlRect(dialog, IDC_COMBO_ENTRIES);
        const auto remove = ControlRect(dialog, IDC_COMBO_REMOVE);
        const auto preview = ControlRect(dialog, IDC_COMBO_PREVIEW);
        const auto save = ControlRect(dialog, IDOK);
        const auto close = ControlRect(dialog, IDCANCEL);
        const auto status = ControlRect(dialog, IDC_COMBO_STATUS);
        CHECK(results.bottom < variants.top && variants.bottom < entries.top && entries.bottom < remove.top);
        CHECK(preview.top + preview.bottom == save.top + save.bottom && save.top == close.top && save.right < close.left);
        CHECK(Height(remove) == Height(save) && Height(save) == Height(close) && save.bottom < status.top);
        wchar_t saveText[32]{}; GetWindowTextW(GetDlgItem(dialog, IDOK), saveText, 32);
        CHECK(std::wstring(saveText) == L"&Save");
        LOGFONTW resultFont{}, previewFont{};
        CHECK(GetObjectW(reinterpret_cast<HFONT>(SendDlgItemMessageW(dialog, IDC_COMBO_RESULTS, WM_GETFONT, 0, 0)),
                         sizeof(resultFont), &resultFont));
        CHECK(GetObjectW(reinterpret_cast<HFONT>(SendDlgItemMessageW(dialog, IDC_COMBO_PREVIEW, WM_GETFONT, 0, 0)),
                         sizeof(previewFont), &previewFont));
        CHECK(std::wstring(resultFont.lfFaceName) == L"Segoe UI Emoji");
        CHECK(std::wstring(previewFont.lfFaceName) == L"Segoe UI Emoji" &&
              std::abs(previewFont.lfHeight) > std::abs(resultFont.lfHeight));
        CHECK((GetWindowLongPtrW(GetDlgItem(dialog, IDC_COMBO_RESULTS), GWL_STYLE) & LBS_OWNERDRAWFIXED) != 0);
        CHECK((GetWindowLongPtrW(GetDlgItem(dialog, IDC_COMBO_VARIANTS), GWL_STYLE) & CBS_OWNERDRAWFIXED) != 0);
        CHECK((GetWindowLongPtrW(GetDlgItem(dialog, IDC_COMBO_PREVIEW), GWL_STYLE) & SS_OWNERDRAW) != 0);
        Command(dialog, IDOK); CHECK(profile.combinations.empty() && saves == 0);
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L"launch");
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🚀"); Command(dialog, IDC_COMBO_ADD);
        CHECK(state.draft.entries.size() == 1 && state.draft.entries[0].payload == L"🚀");
        Command(dialog, IDOK); CHECK(profile.combinations.empty());
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"✨"); Command(dialog, IDC_COMBO_ADD);
        CHECK(Text(dialog, IDC_COMBO_PREVIEW) == L"🚀✨");
        Command(dialog, IDC_COMBO_LEFT); CHECK(Text(dialog, IDC_COMBO_PREVIEW) == L"✨🚀");
        Command(dialog, IDC_COMBO_RIGHT); CHECK(Text(dialog, IDC_COMBO_PREVIEW) == L"🚀✨");
        Command(dialog, IDOK); CHECK(saves == 1 && profile.combinations.size() == 1);
        const auto id = state.draft.id;
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L"lift off"); Command(dialog, IDOK);
        CHECK(state.draft.id == id && profile.combinations.at(id).name == L"lift off");
        Command(dialog, IDC_COMBO_NEW); CHECK(state.draft.id.empty() && state.draft.entries.empty());
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L"please");
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🥺"); Command(dialog, IDC_COMBO_ADD);
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🙏");
        for (size_t i = 0; i < state.variants.size(); ++i) if (state.variants[i]->glyph == L"🙏🏽")
            SendDlgItemMessageW(dialog, IDC_COMBO_VARIANTS, CB_SETCURSEL, i, 0);
        Command(dialog, IDC_COMBO_ADD); Command(dialog, IDOK);
        CHECK(state.draft.payload == L"🥺🙏🏽" && profile.combinations.size() == 2);
        const auto saved = state.draft.payload;
        for (int i = 0; i < 10; ++i) Command(dialog, IDC_COMBO_ADD);
        CHECK(state.draft.entries.size() == 8 && !IsWindowEnabled(GetDlgItem(dialog, IDC_COMBO_ADD)));
        Command(dialog, IDC_COMBO_REMOVE); CHECK(state.draft.entries.size() == 7);
        CHECK(profile.combinations.at(state.draft.id).payload == saved); // Draft edits have no persistent effect.
        DestroyWindow(dialog); dialog = nullptr;
        Editor vocabulary{catalog, profile, {}, {}, persist};
        dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_VOCABULARY), nullptr, DialogProc,
                                    reinterpret_cast<LPARAM>(&vocabulary));
        CHECK(dialog);
        const auto aliases = ControlRect(dialog, IDC_ALIASES);
        const auto pins = ControlRect(dialog, IDC_PINS);
        const auto phrase = ControlRect(dialog, IDC_PHRASE);
        const auto query = ControlRect(dialog, IDC_TARGET_QUERY);
        const auto matches = ControlRect(dialog, IDC_TARGET_RESULTS);
        const auto saveAlias = ControlRect(dialog, IDC_SAVE_ALIAS);
        const auto pin = ControlRect(dialog, IDC_PIN_TARGET);
        CHECK(aliases.bottom < pins.top && phrase.bottom < query.top && query.bottom < matches.top);
        CHECK(saveAlias.top == pin.top && Height(saveAlias) == Height(pin));
        CHECK(GetNextDlgTabItem(dialog, GetDlgItem(dialog, IDC_TARGET_RESULTS), FALSE) == GetDlgItem(dialog, IDC_SAVE_ALIAS));
        DestroyWindow(dialog); dialog = nullptr;
        CHECK(saves == 3);
        std::cout << "PASS: native editor layout, controls, validation, ordering, variants, rename and discarded drafts\n";
    } catch (const std::exception& e) { if (dialog) DestroyWindow(dialog); std::cerr << e.what() << '\n'; return 1; }
}
