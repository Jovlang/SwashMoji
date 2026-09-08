// Native controls and resources, with an in-memory profile and no desktop input.
#include "../vocabulary.cpp"
#include "test_support.h"
#include <fstream>
#include <filesystem>
using namespace SwashMoji;
void Command(HWND dialog, int id, int notification = BN_CLICKED) { SendMessageW(dialog, WM_COMMAND, MAKEWPARAM(id, notification), 0); }
int main(int argc, char** argv) {
    HWND dialog{};
    try {
        CHECK(argc == 2); Catalog catalog; std::ifstream file(std::filesystem::u8path(argv[1]), std::ios::binary); CHECK(catalog.Load(file));
        Profile profile; unsigned saves{}; std::function<bool()> persist = [&] { ++saves; return true; };
        Editor parent{catalog, profile, {}, {}, persist}; CombinationEditor state{parent};
        dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDD_COMBINATIONS), nullptr, CombinationProc, reinterpret_cast<LPARAM>(&state));
        CHECK(dialog);
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
        CHECK(saves == 3);
        std::cout << "PASS: native combination controls, validation, ordering, variants, rename and discarded drafts\n";
    } catch (const std::exception& e) { if (dialog) DestroyWindow(dialog); std::cerr << e.what() << '\n'; return 1; }
}
