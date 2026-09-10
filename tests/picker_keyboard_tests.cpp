// Exercise the actual child-window key handler without foreground activation,
// global hotkeys, synthetic desktop input, or the user's profile.
#define wWinMain SwashMojiUnusedApplicationMain
#include "../main.cpp"
#undef wWinMain
#include "test_support.h"

std::wstring StatusText() {
    std::wstring text(GetWindowTextLengthW(g_status)+1,L'\0');
    text.resize(GetWindowTextW(g_status,text.data(),static_cast<int>(text.size())));
    return text;
}
void CheckSelectedStatus() {
    const auto index=SendMessageW(g_list,LB_GETCURSEL,0,0);
    const auto* emoji=g_catalog.Find(g_displayVisible.at(index).payload);
    CHECK(emoji);
    CHECK(StatusText()==FormatEmojiDisplayName(*emoji, g_profile.settings.displayLanguages));
}

void LanguageDialogControls() {
    Profile profile;
    unsigned saves{};
    bool canSave = false;
    LanguagePreferencesState state{profile, [&] { ++saves; return canSave; }};
    const auto dialog = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kLanguageDialog), nullptr,
        LanguagePreferencesProc, reinterpret_cast<LPARAM>(&state));
    CHECK(dialog);
    CHECK(SelectedLocale(dialog, kPrimaryLanguage) == 0 && SelectedLocale(dialog, kSecondaryLanguage) == 1);
    CHECK(SendDlgItemMessageW(dialog, kPrimaryLanguage, CB_GETCOUNT, 0, 0) == SupportedLocales().size());
    // Selecting the old secondary as primary removes it from secondary choices.
    SendDlgItemMessageW(dialog, kPrimaryLanguage, CB_SETCURSEL, 1, 0);
    SendMessageW(dialog, WM_COMMAND, MAKEWPARAM(kPrimaryLanguage, CBN_SELCHANGE), 0);
    CHECK(SelectedLocale(dialog, kSecondaryLanguage) == -1);
    CHECK(profile.settings.displayLanguages.Locales() == (std::vector<std::string>{"en", "nb"})); // draft only
    for (int i = 0; i < SendDlgItemMessageW(dialog, kSecondaryLanguage, CB_GETCOUNT, 0, 0); ++i)
        CHECK(SendDlgItemMessageW(dialog, kSecondaryLanguage, CB_GETITEMDATA, i, 0) != 1);
    CHECK(!ApplyLanguageChoices(dialog, state));
    CHECK(saves == 1 && profile.settings.displayLanguages.Locales() == std::vector<std::string>{"nb"});
    CHECK(GetWindowTextLengthW(GetDlgItem(dialog, kLanguageStatus)) > 0);
    canSave = true;
    CHECK(ApplyLanguageChoices(dialog, state) && saves == 2);
    DestroyWindow(dialog);
    const auto reopened = CreateDialogParamW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(kLanguageDialog), nullptr,
        LanguagePreferencesProc, reinterpret_cast<LPARAM>(&state));
    CHECK(reopened && SelectedLocale(reopened, kPrimaryLanguage) == 1 && SelectedLocale(reopened, kSecondaryLanguage) == -1);
    DestroyWindow(reopened);
}

struct CombinationInput : InputPlatform {
    bool valid{true}; size_t accepted{SIZE_MAX}; WindowToken foreground{};
    std::vector<std::vector<KeyEvent>> batches;
    bool ValidTarget(const InputTarget&) override { return valid; }
    WindowToken Foreground() override { return foreground; }
    void Activate(WindowToken window) override { foreground = window; }
    std::vector<std::uint16_t> HeldModifiers() override { return {}; }
    size_t Send(const std::vector<KeyEvent>& events) override { batches.push_back(events); return std::min(accepted, events.size()); }
};
struct CombinationClipboard : ClipboardPlatform {
    bool success{true}; std::wstring payload;
    bool Prepare(const std::wstring& value) override { payload = value; return success; }
    bool Open() override { return true; } bool Empty() override { return true; }
    bool Publish() override { return true; } bool Close() override { return true; } void Release() override {}
};
void CombinationInsertion() {
    // Save only to an isolated directory, never resolve the real local profile.
    const auto path = std::filesystem::current_path() / (L"combination-picker-test-" + std::to_wstring(GetCurrentProcessId()));
    g_storage = ProfileStorage(path);
    g_profile = {}; g_inputTarget = {42, 1, 1};
    Combination c; c.name = L"launch";
    for (const auto* glyph : {L"🚀", L"✨", L"👩🏽‍💻", L"❤️"}) {
        const auto* emoji = g_catalog.Find(glyph); CHECK(emoji); c.entries.push_back({emoji->family.value, emoji->glyph});
    }
    std::wstring error; CHECK(SaveCombination(g_profile, g_catalog, c, error));
    const ResultId id{ResultKind::Combination, c.id};
    BeginPickerSession(); SetWindowTextW(g_edit, L"launch");
    CHECK(g_displayVisible[0].id == id && g_displayVisible[0].payload == c.payload);
    CHECK(StatusText()==L"launch");
    const auto before = EncodeProfile(g_profile);
    CombinationInput partial; partial.accepted = 1;
    InsertSelection(true, &partial);
    CHECK(EncodeProfile(g_profile) == before && g_session.query == L"launch" && g_session.selected == id);
    CHECK(g_displayVisible[0].payload == c.payload && partial.batches.size() == 2);
    for (const auto& event : partial.batches[1]) CHECK(event.up);
    CombinationClipboard failed; failed.success = false; CopySelection(&failed);
    CHECK(failed.payload == c.payload && EncodeProfile(g_profile) == before);
    CombinationInput full; InsertSelection(true, &full);
    CHECK(full.batches.size() == 1);
    std::wstring submitted;
    for (const auto& event : full.batches[0]) if (event.unicode && !event.up) submitted += static_cast<wchar_t>(event.code);
    CHECK(submitted == c.payload && UsageCount(g_profile, id) == 1 && QueryCount(g_profile, L"launch", id) == 1);
    CHECK(g_profile.usage.size() == 1 && g_session.selected == id);
    CombinationClipboard copied; CopySelection(&copied);
    CHECK(copied.payload == c.payload && UsageCount(g_profile, id) == 2 && g_profile.usage.size() == 1);
    CHECK(g_storage.Load(&g_catalog).profile.combinations.at(c.id).payload == c.payload);
    // Remove only this test's explicitly named direct child.
    CHECK(path.parent_path() == std::filesystem::current_path());
    std::filesystem::remove_all(path);
}

int main() {
    try {
        // Synthetic messages must not inherit modifiers held on the user's desktop.
        struct KeyboardState {
            BYTE saved[256]{};
            KeyboardState() { BYTE released[256]{}; CHECK(GetKeyboardState(saved)); CHECK(SetKeyboardState(released)); }
            ~KeyboardState() { SetKeyboardState(saved); }
        } keyboardState;
        CHECK(LoadEmojis());
        g_uiFont = g_statusFont = g_emojiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_backgroundBrush = CreateSolidBrush(kBackground);
        g_inputBrush = CreateSolidBrush(kInputBackground);
        WNDCLASSW type{};
        type.hInstance = GetModuleHandleW(nullptr);
        type.lpszClassName = L"SwashMojiKeyboardTest";
        type.lpfnWndProc = WindowProc;
        CHECK(RegisterClassW(&type));
        g_window = CreateWindowExW(0, type.lpszClassName, L"Keyboard test", WS_POPUP,
            0, 0, kPickerWidth, PickerHeight(), nullptr, nullptr, type.hInstance, nullptr);
        CHECK(g_window);
        CHECK((GetWindowLongPtrW(g_status,GWL_STYLE)&SS_ENDELLIPSIS)==SS_ENDELLIPSIS);
        BeginPickerSession();
        SetWindowTextW(g_edit,L"slightly smiling face");
        CHECK(StatusText()==L"slightly smiling face · smiler litt");
        CHECK(g_profile.settings.displayLanguages.Set({"nb"}));
        RefreshList();
        CHECK(StatusText()==L"smiler litt");
        CHECK(g_session.query == L"slightly smiling face");
        CHECK(g_profile.settings.displayLanguages.Set({"en", "nb"}));
        RefreshList();
        LanguageDialogControls();
        const auto statusStyle=GetWindowLongPtrW(g_status,GWL_STYLE);
        RECT statusBounds{}; GetWindowRect(g_status,&statusBounds);
        SetWindowTextW(g_status,(std::wstring(400,L'E') + L" · " + std::wstring(400,L'N')).c_str());
        RECT longBounds{}; GetWindowRect(g_status,&longBounds);
        CHECK(EqualRect(&statusBounds,&longBounds) && GetWindowLongPtrW(g_status,GWL_STYLE)==statusStyle);
        SetWindowTextW(g_edit,L"");
        for (int rows = 1; rows <= 3; ++rows) {
            g_emojiRows = rows;
            BeginPickerSession();
            CHECK(g_visible.size() > 30);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            // Launch focuses search: no preliminary Tab or Down is required.
            SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
            CheckSelectedStatus();
            SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            SendMessageW(g_edit, WM_KEYDOWN, VK_DOWN, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
            SendMessageW(g_edit, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            CHECK(g_session.selected == g_displayVisible[0].id);
            // Native list selection notification is also used by pointer selection.
            SendMessageW(g_list,LB_SETCURSEL,1,0);
            SendMessageW(g_window,WM_COMMAND,MAKEWPARAM(kListId,LBN_SELCHANGE),reinterpret_cast<LPARAM>(g_list));
            CheckSelectedStatus();
            // After typing, every plain arrow still navigates the same grid.
            SetWindowTextW(g_edit, L"face");
            CHECK(g_visible.size() > 30);
            SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
            SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            SendMessageW(g_edit, WM_KEYDOWN, VK_DOWN, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
            SendMessageW(g_edit, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            CHECK(g_session.query == L"face");
            CHECK(g_session.selected == g_displayVisible[0].id);
            SendMessageW(g_list, WM_KEYDOWN, VK_DOWN, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
            SendMessageW(g_list, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            // Ctrl+all four arrows work in search and results, empty or typed.
            BYTE savedKeys[256]{}, controlKeys[256]{};
            CHECK(GetKeyboardState(savedKeys));
            controlKeys[VK_CONTROL] = 0x80;
            CHECK(SetKeyboardState(controlKeys));
            for (const auto* query : {L"", L"face"}) {
                SetWindowTextW(g_edit, query);
                SendMessageW(g_edit, EM_SETSEL, 0, 0);
                for (const auto control : {g_edit, g_list}) {
                    SendMessageW(g_list, LB_SETCURSEL, 0, 0);
                    SendMessageW(control, WM_KEYDOWN, VK_RIGHT, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
                    SendMessageW(control, WM_KEYDOWN, VK_LEFT, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
                    SendMessageW(control, WM_KEYDOWN, VK_DOWN, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
                    SendMessageW(control, WM_KEYDOWN, VK_UP, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
                    CHECK(g_session.query == query);
                }
                DWORD caret{};
                SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&caret), 0);
                CHECK(caret == 0);
            }
            CHECK(SetKeyboardState(savedKeys));
            SetWindowTextW(g_edit, L"");
        }
        g_emojiRows = 3;
        // Fixed alias fixtures keep this layout check independent of catalog languages.
        CHECK(SetAlias(g_profile, g_catalog, L"compact regression one", {ResultKind::Emoji, L"🚀"}) == AliasResult::Saved);
        CHECK(SetAlias(g_profile, g_catalog, L"compact regression two", {ResultKind::Emoji, L"✨"}) == AliasResult::Saved);
        CHECK(SetAlias(g_profile, g_catalog, L"compact regression three", {ResultKind::Emoji, L"🙂"}) == AliasResult::Saved);
        SetWindowTextW(g_edit, L"compact regression");
        CHECK(g_visible.size() == 3);
        RECT compact{}, expanded{}, first{}, second{}, list{};
        GetWindowRect(g_window, &compact);
        GetClientRect(g_list, &list);
        CHECK(list.bottom == Px(kResultSize));
        CHECK(g_emojiRows == 3); // The preference survives automatic compaction.
        SendMessageW(g_list, LB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&first));
        SendMessageW(g_list, LB_GETITEMRECT, 1, reinterpret_cast<LPARAM>(&second));
        CHECK(first.top == second.top && second.left > first.left);
        SendMessageW(g_edit, WM_KEYDOWN, VK_DOWN, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 2);
        const auto compactSelection = g_session.selected;
        RefreshList();
        CHECK(g_session.selected == compactSelection);
        CHECK(DeleteAlias(g_profile, L"compact regression one"));
        CHECK(DeleteAlias(g_profile, L"compact regression two"));
        CHECK(DeleteAlias(g_profile, L"compact regression three"));
        SetWindowTextW(g_edit, L"face");
        GetWindowRect(g_window, &expanded);
        GetClientRect(g_list, &list);
        CHECK(list.bottom == Px(3 * kResultSize));
        CHECK(expanded.bottom - expanded.top == compact.bottom - compact.top + Px(2 * kResultSize));
        SendMessageW(g_edit, EM_SETSEL, 2, 2);
        const auto selected = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
        // SetKeyboardState affects only this test thread, never desktop input.
        BYTE originalKeys[256]{}, modifiedKeys[256]{};
        CHECK(GetKeyboardState(originalKeys));
        modifiedKeys[VK_SHIFT] = 0x80;
        CHECK(SetKeyboardState(modifiedKeys));
        SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
        DWORD start{}, end{};
        SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
        CHECK(SetKeyboardState(originalKeys));
        CHECK(start == 1 && end == 2);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == selected);
        // Ctrl+Shift still selects words rather than moving the result.
        modifiedKeys[VK_SHIFT] = 0x80;
        modifiedKeys[VK_CONTROL] = 0x80;
        CHECK(SetKeyboardState(modifiedKeys));
        SendMessageW(g_edit, EM_SETSEL, 0, 0);
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
        CHECK(SetKeyboardState(originalKeys));
        CHECK(start == 0 && end == 4);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == selected);
        SendMessageW(g_edit, EM_SETSEL, 4, 4);
        SendMessageW(g_edit, WM_CHAR, L's', 0);
        CHECK(g_session.query == L"faces");
        // Clearing the query restores immediate navigation in the same session.
        SetWindowTextW(g_edit, L"");
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == g_emojiRows);
        CHECK(g_profile.history.empty());
        CombinationInsertion();
        DestroyWindow(g_window);
        std::cout << "PASS: launch and typed-query arrows, all row counts, modified text editing and cleared search\n";
    } catch (const std::exception& error) {
        if (g_window) DestroyWindow(g_window);
        std::cerr << error.what() << '\n'; return 1;
    }
}
