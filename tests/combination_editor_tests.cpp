// Native controls and resources, with an in-memory profile and no desktop input.
#include "../src/vocabulary.cpp"
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
std::wstring DraftPayload(const Combination& draft) {
    std::wstring payload; for (const auto& entry : draft.entries) payload += entry.payload; return payload;
}
int Height(const RECT& value) { return value.bottom - value.top; }
void LocalizedDeleteConfirmation() {
    const auto parent = CreateWindowExW(0, L"STATIC", L"Test owner", WS_POPUP,
        0, 0, 400, 200, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    CHECK(parent);
    for (const auto* locale : {"nb", "de", "it", "fr", "es"}) {
        SetPropW(parent, L"SwashMojiUiLocale", const_cast<char*>(locale));
        const auto dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_COMBO_DELETE),
            parent, ConfirmCombinationDelete, reinterpret_cast<LPARAM>(L"Save\r\nrocket 🚀"));
        CHECK(dialog);
        wchar_t text[256]{};
        GetWindowTextW(dialog, text, 256);
        CHECK(std::wstring(text) == UiText(locale, L"Delete combination"));
        GetDlgItemTextW(dialog, IDC_COMBO_DEPENDENTS, text, 256);
        CHECK(std::wstring(text) == L"Save\r\nrocket 🚀");
        DestroyWindow(dialog);
    }
    DestroyWindow(parent);
}
int main(int argc, char** argv) {
    HWND dialog{};
    try {
        LocalizedDeleteConfirmation();
        CHECK(argc == 2); Catalog catalog; std::ifstream file(std::filesystem::u8path(argv[1]), std::ios::binary); CHECK(catalog.Load(file));
        Profile profile; unsigned saves{}; std::function<bool()> persist = [&] { ++saves; return true; };
        Editor parent{catalog, profile, {}, {}, persist}; CombinationEditor state{parent};
        dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_COMBINATIONS), nullptr, CombinationProc, reinterpret_cast<LPARAM>(&state));
        CHECK(dialog);
        CHECK(ControlRect(dialog, IDC_COMBO_SAVED_HEADING).top == ControlRect(dialog, IDC_COMBO_EDITOR).top);
        CHECK(ControlRect(dialog, IDC_COMBO_SAVED_HEADING).bottom < ControlRect(dialog, IDC_COMBO_SAVED).top);
        const auto variantsVisible = [&](bool expected) {
            for (const int id : {IDC_COMBO_VARIANT_LABEL, IDC_COMBO_VARIANTS})
                CHECK(((GetWindowLongPtrW(GetDlgItem(dialog, id), GWL_STYLE) & WS_VISIBLE) != 0) == expected);
        };
        const auto results = ControlRect(dialog, IDC_COMBO_RESULTS);
        const auto variants = ControlRect(dialog, IDC_COMBO_VARIANTS);
        const auto entries = ControlRect(dialog, IDC_COMBO_ENTRIES);
        const auto remove = ControlRect(dialog, IDC_COMBO_REMOVE);
        const auto save = ControlRect(dialog, IDOK);
        const auto close = ControlRect(dialog, IDCANCEL);
        const auto status = ControlRect(dialog, IDC_COMBO_STATUS);
        CHECK(results.bottom < variants.top && variants.bottom < entries.top && entries.bottom < remove.top);
        CHECK(remove.bottom < save.top && save.bottom < close.top);
        CHECK(Height(remove) == Height(close) && save.bottom < status.top && status.bottom < close.top);
        wchar_t saveText[32]{}; GetWindowTextW(GetDlgItem(dialog, IDOK), saveText, 32);
        CHECK(std::wstring(saveText) == L"&Save combination");
        LOGFONTW resultFont{};
        CHECK(GetObjectW(reinterpret_cast<HFONT>(SendDlgItemMessageW(dialog, IDC_COMBO_RESULTS, WM_GETFONT, 0, 0)),
                         sizeof(resultFont), &resultFont));
        CHECK(std::wstring(resultFont.lfFaceName) == L"Segoe UI Emoji");
        CHECK((GetWindowLongPtrW(GetDlgItem(dialog, IDC_COMBO_RESULTS), GWL_STYLE) & LBS_OWNERDRAWFIXED) != 0);
        CHECK((GetWindowLongPtrW(GetDlgItem(dialog, IDC_COMBO_VARIANTS), GWL_STYLE) & CBS_OWNERDRAWFIXED) != 0);
        CHECK(GetDlgItem(dialog, IDC_COMBO_DETAILS_PAYLOAD) == nullptr);
        CHECK(GetNextDlgTabItem(dialog,GetDlgItem(dialog,IDC_COMBO_REMOVE),FALSE)==GetDlgItem(dialog,IDOK));
        CHECK((GetWindowLongPtrW(GetDlgItem(dialog, IDC_COMBO_ENTRIES), GWL_STYLE) & LBS_MULTICOLUMN) != 0);
        CHECK(!IsWindowEnabled(GetDlgItem(dialog,IDC_COMBO_LEFT)) && !IsWindowEnabled(GetDlgItem(dialog,IDC_COMBO_RIGHT)));
        SetDlgItemTextW(dialog,IDC_COMBO_QUERY,L"zzzznoresultzzzz");
        CHECK(SendDlgItemMessageW(dialog,IDC_COMBO_RESULTS,LB_GETCOUNT,0,0)==0);
        CHECK(!IsWindowEnabled(GetDlgItem(dialog,IDC_COMBO_ADD)));
        variantsVisible(false);
        Command(dialog, IDOK); CHECK(profile.combinations.empty() && saves == 0);
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L"launch");
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🚀");
        CHECK(state.variants.size() == 1);
        variantsVisible(false);
        CHECK(GetNextDlgTabItem(dialog, GetDlgItem(dialog, IDC_COMBO_RESULTS), FALSE) == GetDlgItem(dialog, IDC_COMBO_ADD));
        Command(dialog, IDC_COMBO_ADD);
        CHECK(state.draft.entries.size() == 1 && state.draft.entries[0].payload == L"🚀");
        Command(dialog, IDOK); CHECK(profile.combinations.empty());
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"✨"); Command(dialog, IDC_COMBO_ADD);
        CHECK(DraftPayload(state.draft) == L"🚀✨");
        Command(dialog, IDC_COMBO_LEFT); CHECK(DraftPayload(state.draft) == L"✨🚀");
        Command(dialog, IDC_COMBO_RIGHT); CHECK(DraftPayload(state.draft) == L"🚀✨");
        Command(dialog, IDOK); CHECK(saves == 1 && profile.combinations.size() == 1);
        const auto id = state.draft.id;
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L"lift off"); Command(dialog, IDOK);
        CHECK(state.draft.id == id && profile.combinations.at(id).name == L"lift off");
        Command(dialog, IDC_COMBO_NEW); CHECK(state.draft.id.empty() && state.draft.entries.empty());
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L"please");
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🥺"); Command(dialog, IDC_COMBO_ADD);
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🙏");
        CHECK(state.variants.size() > 1);
        variantsVisible(true);
        CHECK(GetNextDlgTabItem(dialog, GetDlgItem(dialog, IDC_COMBO_RESULTS), FALSE) == GetDlgItem(dialog, IDC_COMBO_VARIANTS));
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🚀");
        variantsVisible(false);
        SetDlgItemTextW(dialog, IDC_COMBO_QUERY, L"🙏");
        variantsVisible(true);
        for (size_t i = 0; i < state.variants.size(); ++i) if (state.variants[i]->glyph == L"🙏🏽")
            SendDlgItemMessageW(dialog, IDC_COMBO_VARIANTS, CB_SETCURSEL, i, 0);
        Command(dialog, IDC_COMBO_ADD); Command(dialog, IDOK);
        CHECK(state.draft.payload == L"🥺🙏🏽" && profile.combinations.size() == 2);
        const auto saved = state.draft.payload;
        for (int i = 0; i < 10; ++i) Command(dialog, IDC_COMBO_ADD);
        CHECK(state.draft.entries.size() == 8 && !IsWindowEnabled(GetDlgItem(dialog, IDC_COMBO_ADD)));
        RECT firstTile{},lastTile{},sequenceClient{};
        SendDlgItemMessageW(dialog,IDC_COMBO_ENTRIES,LB_GETITEMRECT,0,reinterpret_cast<LPARAM>(&firstTile));
        SendDlgItemMessageW(dialog,IDC_COMBO_ENTRIES,LB_GETITEMRECT,7,reinterpret_cast<LPARAM>(&lastTile));
        GetClientRect(GetDlgItem(dialog,IDC_COMBO_ENTRIES),&sequenceClient);
        CHECK(firstTile.top==lastTile.top && lastTile.left>firstTile.left && lastTile.right<=sequenceClient.right);
        CHECK(!IsWindowEnabled(GetDlgItem(dialog,IDC_COMBO_RIGHT)));
        RECT originalWindow{}; GetWindowRect(dialog,&originalWindow);
        SetWindowPos(dialog,nullptr,0,0,originalWindow.right-originalWindow.left+240,originalWindow.bottom-originalWindow.top+120,SWP_NOMOVE|SWP_NOZORDER);
        CHECK(ControlRect(dialog,IDC_COMBO_RESULTS).right>results.right);
        CHECK(Height(ControlRect(dialog,IDC_COMBO_RESULTS))>Height(results));
        CHECK(ControlRect(dialog,IDC_COMBO_NAME).left==ControlRect(dialog,IDC_COMBO_RESULTS).left);
        CHECK(ControlRect(dialog,IDC_COMBO_STATUS).bottom<ControlRect(dialog,IDCANCEL).top);
        Command(dialog, IDC_COMBO_REMOVE); CHECK(state.draft.entries.size() == 7);
        CHECK(profile.combinations.at(state.draft.id).payload == saved); // Draft edits have no persistent effect.
        DestroyWindow(dialog); dialog = nullptr;
        Editor vocabulary{catalog, profile, L"learned phrase", {}, persist};
        dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_VOCABULARY), nullptr, DialogProc,
                                    reinterpret_cast<LPARAM>(&vocabulary));
        CHECK(dialog);
        const auto aliases = ControlRect(dialog, IDC_ALIASES);
        CHECK(ControlRect(dialog, IDC_ALIASES_HEADING).top == ControlRect(dialog, IDC_ALIAS_HEADING).top);
        CHECK(ControlRect(dialog, IDC_ALIASES_HEADING).bottom < aliases.top);
        const auto pins = ControlRect(dialog, IDC_PINS);
        const auto phrase = ControlRect(dialog, IDC_PHRASE);
        const auto query = ControlRect(dialog, IDC_TARGET_QUERY);
        const auto matches = ControlRect(dialog, IDC_TARGET_RESULTS);
        const auto saveAlias = ControlRect(dialog, IDC_SAVE_ALIAS);
        const auto pin = ControlRect(dialog, IDC_PIN_TARGET);
        CHECK(aliases.bottom < pins.top && phrase.bottom < query.top && query.bottom < matches.top);
        CHECK(saveAlias.top == pin.top && Height(saveAlias) == Height(pin));
        CHECK(GetNextDlgTabItem(dialog, GetDlgItem(dialog, IDC_TARGET_RESULTS), FALSE) == GetDlgItem(dialog, IDC_SAVE_ALIAS));
        CHECK(ControlRect(dialog, IDCANCEL).top > saveAlias.bottom);
        CHECK(ControlRect(dialog, IDC_COMBINATIONS).top == ControlRect(dialog, IDCANCEL).top);
        CHECK(matches.bottom < saveAlias.top);
        CHECK(!(GetWindowLongPtrW(GetDlgItem(dialog, IDC_PHRASE), GWL_EXSTYLE) & WS_EX_CLIENTEDGE));
        RECT window{}; GetWindowRect(dialog, &window);
        SetWindowPos(dialog, nullptr, 0, 0, window.right-window.left+240, window.bottom-window.top+160, SWP_NOMOVE|SWP_NOZORDER);
        const auto expanded = ControlRect(dialog, IDC_TARGET_RESULTS);
        CHECK(expanded.right-expanded.left > matches.right-matches.left && Height(expanded) > Height(matches));
        CHECK(ControlRect(dialog, IDC_PHRASE).left == expanded.left);
        CHECK(ControlRect(dialog, IDC_SAVE_ALIAS).top == ControlRect(dialog, IDC_PIN_TARGET).top);
        CHECK(expanded.bottom < ControlRect(dialog, IDC_SAVE_ALIAS).top);
        SetDlgItemTextW(dialog, IDC_TARGET_QUERY, L"zzzznoresultzzzz");
        CHECK(SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_GETCOUNT, 0, 0) == 0);
        CHECK(!IsWindowEnabled(GetDlgItem(dialog, IDC_PIN_TARGET)));
        SetDlgItemTextW(dialog, IDC_TARGET_QUERY, L"🚀");
        CHECK(vocabulary.target.value.empty());
        CHECK(!IsWindowEnabled(GetDlgItem(dialog, IDC_PIN_TARGET)));
        SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_SETCURSEL, 0, 0);
        Command(dialog, IDC_TARGET_RESULTS, LBN_SELCHANGE);
        CHECK(vocabulary.target == vocabulary.results.front().id);
        CHECK(IsWindowEnabled(GetDlgItem(dialog, IDC_PIN_TARGET)));
        SetDlgItemTextW(dialog, IDC_PHRASE, L"draft");
        CHECK(Text(dialog, IDC_VOCABULARY_STATUS) == L"Unsaved changes");
        DestroyWindow(dialog); dialog = nullptr;
        CombinationEditor tabCombination = MakeCombinationEditor(vocabulary);
        VocabularyPages pages{vocabulary, tabCombination};
        dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_VOCABULARY_TABS), nullptr,
                                    VocabularyPagesProc, reinterpret_cast<LPARAM>(&pages));
        CHECK(dialog && pages.vocabulary && pages.combination);
        CHECK(GetDlgItem(dialog, IDC_ALIASES_TAB) && GetDlgItem(dialog, IDC_COMBINATIONS_TAB));
        CHECK(IsDlgButtonChecked(dialog, IDC_ALIASES_TAB) == BST_CHECKED);
        CHECK((GetWindowLongPtrW(pages.vocabulary, GWL_STYLE) & WS_VISIBLE) != 0);
        CHECK((GetWindowLongPtrW(pages.combination, GWL_STYLE) & WS_VISIBLE) == 0);
        CHECK(Text(pages.vocabulary, IDC_PHRASE) == L"learned phrase");
        SendMessageW(GetDlgItem(dialog, IDC_ALIASES_TAB), WM_KEYDOWN, VK_RIGHT, 0);
        CHECK(IsDlgButtonChecked(dialog, IDC_COMBINATIONS_TAB) == BST_CHECKED);
        CHECK((GetWindowLongPtrW(pages.vocabulary, GWL_STYLE) & WS_VISIBLE) == 0);
        CHECK((GetWindowLongPtrW(pages.combination, GWL_STYLE) & WS_VISIBLE) != 0);
        CHECK(Text(pages.combination, IDC_COMBO_NAME) == L"learned phrase");
        CHECK((GetWindowLongPtrW(pages.combination, GWL_STYLE) & WS_CHILD) != 0);
        constexpr LONG_PTR pageEdges = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;
        CHECK((GetWindowLongPtrW(pages.vocabulary, GWL_EXSTYLE) & pageEdges) == 0);
        CHECK((GetWindowLongPtrW(pages.combination, GWL_EXSTYLE) & pageEdges) == 0);
        DestroyWindow(dialog); dialog = nullptr;
        CHECK(saves == 3);
        std::cout << "PASS: native tabbed editor layout, controls, validation, ordering, variants, rename and discarded drafts\n";
    } catch (const std::exception& e) { if (dialog) DestroyWindow(dialog); std::cerr << e.what() << '\n'; return 1; }
}
