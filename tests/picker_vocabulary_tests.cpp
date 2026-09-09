// Exercise the real picker/editor integration without resolving the user's profile
// or registering the application's global hotkey. The included entry points are
// renamed; only this test's isolated setup is run.
#define wWinMain SwashMojiUnusedApplicationMain
#include "../main.cpp"
#undef wWinMain
#define wmain SwashMojiUnusedInputTestMain
#include "native_input_tests.cpp"
#undef wmain
#include "../vocabulary_ids.h"

namespace {
enum class DialogAction { Create, Cancel, Edit, DeclineReplacement, Delete, CheckHeartPrefill, Favorites };
DialogAction action;
std::string dialogFailure;

HWND OwnDialog(const wchar_t* caption) {
    struct Find { const wchar_t* caption; HWND found{}; } find{caption};
    EnumThreadWindows(GetCurrentThreadId(), [](HWND window, LPARAM parameter) -> BOOL {
        auto& find = *reinterpret_cast<Find*>(parameter);
        wchar_t title[128]{};
        GetWindowTextW(window, title, 128);
        if (std::wstring(title) == find.caption) { find.found = window; return FALSE; }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&find));
    return find.found;
}

void Command(HWND dialog, int id, int notification = BN_CLICKED) {
    SendMessageW(dialog, WM_COMMAND, MAKEWPARAM(id, notification), reinterpret_cast<LPARAM>(GetDlgItem(dialog, id)));
}

void CALLBACK Confirm(HWND, UINT, UINT_PTR timer, DWORD) {
    const bool deleting = action == DialogAction::Delete;
    if (const auto dialog = OwnDialog(deleting ? L"Delete alias" : L"Replace alias")) {
        KillTimer(nullptr, timer);
        Command(dialog, deleting ? IDYES : IDNO);
    }
}

void CALLBACK DriveDialog(HWND, UINT, UINT_PTR timer, DWORD) {
    const auto dialog = OwnDialog(L"My vocabulary - SwashMoji");
    if (!dialog) return;
    KillTimer(nullptr, timer);
    try {
        // Opening the global picker shortcut during a modal editor must not reset it.
        const auto query = g_session.query;
        SendMessageW(g_window, WM_HOTKEY, kHotkeyId, 0);
        CHECK(g_session.query == query);
        if (action == DialogAction::Favorites) {
            Command(dialog, IDC_PIN_TARGET);
            CHECK(IsPinned(g_profile, {ResultKind::Emoji, L"🚀"}));
            SetDlgItemTextW(dialog, IDC_TARGET_QUERY, L"thumbs up");
            SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_SETCURSEL, 0, 0);
            Command(dialog, IDC_TARGET_RESULTS, LBN_SELCHANGE);
            Command(dialog, IDC_PIN_TARGET);
            CHECK(g_profile.pins.size() == 2 && g_profile.pins[1].value == L"👍");
            Command(dialog, IDC_PIN_UP);
            CHECK(g_profile.pins[0].value == L"👍");
            CHECK(SendDlgItemMessageW(dialog, IDC_PINS, LB_GETCURSEL, 0, 0) == 0);
            Command(dialog, IDC_PIN_DOWN);
            CHECK(g_profile.pins[0].value == L"🚀");
            Command(dialog, IDC_UNPIN);
            CHECK(g_profile.pins.size() == 1);
            CHECK(g_storage.Load(&g_catalog).profile.pins == g_profile.pins);
        } else if (action == DialogAction::CheckHeartPrefill) {
            wchar_t targetQuery[32]{};
            GetDlgItemTextW(dialog, IDC_TARGET_QUERY, targetQuery, 32);
            CHECK(std::wstring(targetQuery) == L"red heart");
            CHECK(SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_GETCOUNT, 0, 0) >= 1);
            CHECK(SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_GETCURSEL, 0, 0) == 0);
        } else if (action == DialogAction::Create) {
            wchar_t prefill[256]{};
            GetDlgItemTextW(dialog, IDC_PHRASE, prefill, 256);
            CHECK(std::wstring(prefill) == L"rocket");
            SetDlgItemTextW(dialog, IDC_PHRASE, L"min rakett");
            Command(dialog, IDC_SAVE_ALIAS);
            CHECK(g_profile.aliases.at(L"min rakett").target.value == L"🚀");
        } else if (action == DialogAction::Cancel) {
            SetDlgItemTextW(dialog, IDC_PHRASE, L"discard this draft");
        } else {
            SendDlgItemMessageW(dialog, IDC_ALIASES, LB_SETCURSEL, 0, 0);
            Command(dialog, IDC_ALIASES, LBN_SELCHANGE);
            if (action == DialogAction::Edit) {
                SetDlgItemTextW(dialog, IDC_PHRASE, L"min romferd");
                SetDlgItemTextW(dialog, IDC_TARGET_QUERY, L"kaffe");
                SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_SETCURSEL, 0, 0);
                Command(dialog, IDC_TARGET_RESULTS, LBN_SELCHANGE);
                Command(dialog, IDC_SAVE_ALIAS);
                CHECK(!g_profile.aliases.count(L"min rakett"));
                CHECK(g_profile.aliases.at(L"min romferd").target.value == L"☕");
            } else {
                const auto before = EncodeProfile(g_profile);
                CHECK(SetTimer(nullptr, 0, 25, Confirm));
                if (action == DialogAction::Delete) {
                    Command(dialog, IDC_DELETE_ALIAS);
                    CHECK(!g_profile.aliases.count(L"min romferd"));
                } else {
                    SetDlgItemTextW(dialog, IDC_PHRASE, L"other phrase");
                    Command(dialog, IDC_SAVE_ALIAS);
                    CHECK(EncodeProfile(g_profile) == before);
                }
            }
        }
    } catch (const std::exception& error) { dialogFailure = error.what(); }
    Command(dialog, IDCANCEL);
}

void ExerciseDialog(DialogAction next) {
    action = next;
    dialogFailure.clear();
    const auto inputTarget = g_inputTarget;
    const auto query = g_session.query;
    const auto selected = g_session.selected;
    CHECK(SetTimer(nullptr, 0, 25, DriveDialog));
    OpenVocabulary(true);
    CHECK(dialogFailure.empty());
    CHECK(g_session.query == query && g_session.selected == selected);
    CHECK(g_inputTarget.window == inputTarget.window && g_inputTarget.process == inputTarget.process &&
          g_inputTarget.thread == inputTarget.thread && g_session.originalTarget == inputTarget.window);
    CHECK(g_storage.Load().profile.aliases.size() == g_profile.aliases.size());
}

struct StubInput : InputPlatform {
    bool valid{true}, denied{};
    int accept{-1};
    WindowToken foreground{};
    bool ValidTarget(const InputTarget&) override { return valid; }
    WindowToken Foreground() override { return foreground; }
    void Activate(WindowToken window) override { if (!denied) foreground = window; }
    std::vector<std::uint16_t> HeldModifiers() override { return {}; }
    size_t Send(const std::vector<KeyEvent>& events) override { return accept < 0 ? events.size() : std::min(events.size(), static_cast<size_t>(accept)); }
};

struct StubClipboard : ClipboardPlatform {
    int failure{};
    bool Prepare(const std::wstring&) override { return failure != 1; }
    bool Open() override { return failure != 2; }
    bool Empty() override { return failure != 3; }
    bool Publish() override { return failure != 4; }
    bool Close() override { return failure != 5; }
    void Release() override {}
};

void SelectResult(const ResultId& target) {
    for (size_t i = 0; i < g_displayVisible.size(); ++i) if (g_displayVisible[i].id == target) {
        SendMessageW(g_list, LB_SETCURSEL, i, 0);
        g_session.selected = target;
        return;
    }
    CHECK(false);
}

void LearningIntegration() {
    const ResultId thumbs{ResultKind::Emoji, L"👍"}, okay{ResultKind::Emoji, L"👌"};
    ClearHistory(g_profile);
    SwashMoji::Remember(g_profile, thumbs);
    BeginPickerSession();
    SetWindowTextW(g_edit, L"nice");
    CHECK(g_visible[0].id == thumbs);
    SelectResult(okay);
    const auto originalOrder = g_session.rankingSnapshot;
    const auto before = EncodeProfile(g_profile);
    for (int failure = 0; failure < 4; ++failure) {
        StubInput input;
        input.valid = failure != 0;
        input.denied = failure == 1;
        if (failure >= 2) input.accept = failure - 2; // Zero and partial submission.
        InsertSelection(true, &input);
        CHECK(EncodeProfile(g_profile) == before);
        CHECK(g_session.selected == okay && g_session.rankingSnapshot == originalOrder);
    }
    for (int failure = 1; failure <= 5; ++failure) {
        StubClipboard clipboard;
        clipboard.failure = failure;
        CopySelection(&clipboard);
        CHECK(EncodeProfile(g_profile) == before);
    }
    StubInput success;
    InsertSelection(true, &success);
    InsertSelection(true, &success);
    CHECK(QueryCount(g_profile, L"nice", okay) == 2 && UsageCount(g_profile, okay) == 2);
    CHECK(g_session.rankingSnapshot == originalOrder && g_session.selected == okay);
    // Re-rendering and changing query within this session still use frozen preferences.
    RefreshList();
    CHECK(g_session.rankingSnapshot == originalOrder && g_session.selected == okay);
    SetEmojiRows(3);
    CHECK(g_session.rankingSnapshot == originalOrder && g_session.selected == okay);
    CycleSkinTone();
    CHECK(g_session.rankingSnapshot == originalOrder && g_session.selected == okay);
    g_emojiFonts = {{L"Segoe UI Emoji", true}};
    g_emojiFont = nullptr; // The test initially used a stock font, which it must not delete.
    CycleEmojiFont();
    CHECK(g_session.rankingSnapshot == originalOrder && g_session.selected == okay);
    SetWindowTextW(g_edit, L"bra");
    SetWindowTextW(g_edit, L"nice");
    CHECK(g_visible[0].id == thumbs);
    SelectResult(okay);
    StubClipboard copied;
    CopySelection(&copied);
    CHECK(QueryCount(g_profile, L"nice", okay) == 3 && UsageCount(g_profile, okay) == 3);
    CHECK(QueryCount(g_profile, L"bra", okay) == 0);
    BeginPickerSession();
    CHECK(g_visible[0].id == okay);
    const auto selectedBeforePin = g_session.selected;
    ToggleSelectedPin();
    CHECK(IsPinned(g_profile, okay) && g_session.selected == selectedBeforePin);
    CHECK(g_storage.Load(&g_catalog).profile.pins == g_profile.pins);
    g_profile.settings.skinTone = 0;
    SetEmojiRows(1);
    SetRecoveryMessage(L"");
}

bool acceptDetails{};
std::wstring chosenVariant;
void CALLBACK DriveDetails(HWND, UINT, UINT_PTR timer, DWORD) {
    auto dialog = OwnDialog(L"Details - SwashMoji");
    if (!dialog) return;
    KillTimer(nullptr, timer);
    auto* state = reinterpret_cast<DetailsState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    for (size_t i = 0; i < state->variants.size(); ++i) {
        const auto* emoji = state->variants[i];
        if (SkinToneIndex(emoji->glyph) && !UsesOnlySkinTone(emoji->glyph, SkinToneIndex(emoji->glyph))) {
            chosenVariant = emoji->glyph;
            SendDlgItemMessageW(dialog, 403, LB_SETCURSEL, i, 0);
            Command(dialog, 403, LBN_SELCHANGE);
            break;
        }
    }
    Command(dialog, acceptDetails ? IDOK : IDCANCEL);
}

void DetailsChoice(bool accept) {
    acceptDetails = accept;
    CHECK(SetTimer(nullptr, 0, 25, DriveDetails));
    const auto target = g_session.originalTarget;
    const auto query = g_session.query;
    const auto selected = g_session.selected;
    OpenDetails();
    CHECK(!chosenVariant.empty());
    CHECK(g_session.originalTarget == target && g_session.query == query && g_session.selected == selected);
}

void SelectionIntegration() {
    SetWindowTextW(g_edit, L"handshake");
    SelectResult({ResultKind::Emoji, L"🤝"});
    const auto profile = EncodeProfile(g_profile);
    DetailsChoice(false);
    CHECK(g_variantPayload.empty() && EncodeProfile(g_profile) == profile);
    DetailsChoice(true);
    CHECK(g_variantPayload == chosenVariant && EncodeProfile(g_profile) == profile);
    const auto variant = chosenVariant;
    wchar_t label[512]{};
    SendMessageW(g_list, LB_GETTEXT, SendMessageW(g_list, LB_GETCURSEL, 0, 0), reinterpret_cast<LPARAM>(label));
    CHECK(std::wstring(label) == FormatEmojiDisplayName(*g_catalog.Find(variant)));
    StubInput failed; failed.accept = 0;
    InsertSelection(true, &failed);
    CHECK(g_variantPayload == variant && EncodeProfile(g_profile) == profile);
    StubClipboard busy; busy.failure = 2;
    CopySelection(&busy);
    CHECK(g_variantPayload == variant);
    StubInput success;
    InsertSelection(true, &success);
    CHECK(g_variantPayload.empty() && g_profile.settings.skinTone == 0);
    SetRecoveryMessage(L"");
    SetWindowTextW(g_edit, L"rocket");
    const auto beforeClick = EncodeProfile(g_profile);
    SendMessageW(g_list, WM_LBUTTONDOWN, 0, MAKELPARAM(Px(200), Px(20)));
    SendMessageW(g_list, WM_LBUTTONUP, 0, MAKELPARAM(Px(200), Px(20)));
    CHECK(EncodeProfile(g_profile) == beforeClick && g_recoveryMessage.empty());
    SendMessageW(g_edit, EM_SETSEL, 3, 3);
    SendMessageW(g_edit, WM_KEYDOWN, VK_HOME, 0);
    DWORD caret{}; SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&caret), 0);
    CHECK(caret == 0);
    for (UINT dpi : {96u, 120u, 144u, 192u}) {
        UpdateDpi(dpi);
        SetWindowPos(g_window, nullptr, 0, 0, Px(kPickerWidth), PickerHeight(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        for (int rows = 1; rows <= 3; ++rows) {
            SetEmojiRows(rows);
            SetWindowTextW(g_edit, L"");
            CHECK(g_displayVisible.size() == g_visible.size());
            RECT item{}; CHECK(SendMessageW(g_list, LB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&item)) != LB_ERR);
            CHECK(item.bottom - item.top == Px(kResultSize));
            CHECK(HitResult({item.left + 1, item.top + 1}) == 0);
            SendMessageW(g_list, LB_SETCURSEL, 0, 0);
            SendMessageW(g_list, WM_KEYDOWN, VK_RIGHT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
        }
    }
    UpdateDpi(GetDpiForWindow(g_window));
    SetEmojiRows(1);
    SetWindowTextW(g_edit, L"rocket");
}
}

int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--target") return RunTarget(argv[2]);
    const auto directory = EmojiCatalogPath().parent_path();
    auto report = [&](const std::string& message) {
        std::ofstream file(directory / L"picker-vocabulary-result.txt", std::ios::trunc);
        file << message << '\n';
    };
    report("RUNNING");
    try {
        CHECK(LoadEmojis());
        g_storage = ProfileStorage(directory / (L"vocabulary-test-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64())));
        g_profile = g_storage.Load(&g_catalog).profile;
        g_uiFont = g_statusFont = g_emojiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_backgroundBrush = CreateSolidBrush(kBackground);
        g_inputBrush = CreateSolidBrush(kInputBackground);
        WNDCLASSW type{};
        type.hInstance = GetModuleHandleW(nullptr);
        type.lpszClassName = kClassName;
        type.lpfnWndProc = WindowProc;
        type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        CHECK(RegisterClassW(&type));
        g_window = CreateWindowExW(WS_EX_TOOLWINDOW, kClassName, L"SwashMoji M3 test picker", WS_POPUP,
            300, 300, kPickerWidth, PickerHeight(), nullptr, nullptr, type.hInstance, nullptr);
        CHECK(g_window);

        if (argc == 2 && std::wstring(argv[1]) == L"--preview") {
            CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            InitializeColorEmojiDrawing(); LoadInstalledColorEmojiFonts();
            g_emojiFont = nullptr;
            UpdateDpi(GetDpiForWindow(g_window));
            WNDCLASSW help{}; help.hInstance = type.hInstance; help.lpszClassName = kHelpClassName;
            help.lpfnWndProc = HelpWindowProc; help.hCursor = type.hCursor;
            RegisterClassW(&help);
            Pin(g_profile, g_catalog, {ResultKind::Emoji, L"🚀"});
            Pin(g_profile, g_catalog, {ResultKind::Emoji, L"👍"});
            g_emojiRows = 3;
            BeginPickerSession();
            SetWindowPos(g_window, nullptr, 300, 300, Px(kPickerWidth), PickerHeight(), SWP_NOZORDER | SWP_SHOWWINDOW);
            SetWindowTextW(g_window, L"SwashMoji M4 isolated preview");
            ShowWindow(g_window, SW_SHOW);
            ShowWindow(g_window, SW_SHOW); // Honor an initial hidden launcher, then show the requested preview.
            SetForegroundWindow(g_window); SetFocus(g_edit);
            MSG message{};
            while (GetMessageW(&message, nullptr, 0, 0) > 0) {
                if (ProcessAppMessage(message)) continue;
                TranslateMessage(&message); DispatchMessageW(&message);
            }
            return 0;
        }

        TargetProcess targetProcess;
        wchar_t executable[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executable, MAX_PATH);
        const auto title = L"SwashMoji M3 target " + std::to_wstring(GetCurrentProcessId());
        std::wstring command = L"\"" + std::wstring(executable) + L"\" --target \"" + title + L"\"";
        STARTUPINFOW startup{sizeof(startup)};
        CHECK(CreateProcessW(executable, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, nullptr, &startup, &targetProcess.process));
        WaitForInputIdle(targetProcess.process.hProcess, 3000);
        const auto deadline = GetTickCount64() + 3000;
        do {
            targetProcess.window = FindWindowW(kTargetClass, title.c_str());
            if (targetProcess.window || WaitForSingleObject(targetProcess.process.hProcess, 0) == WAIT_OBJECT_0) break;
            Sleep(10);
        } while (GetTickCount64() < deadline);
        CHECK(targetProcess.window);
        g_inputTarget = CaptureExternalTarget(targetProcess.window);
        CHECK(g_inputTarget.window);
        g_session.originalTarget = g_inputTarget.window;
        SetWindowTextW(g_edit, L"rocket");
        ExerciseDialog(DialogAction::Favorites);
        LearningIntegration();
        SelectionIntegration();
        SetWindowTextW(g_edit, L"rocket");
        ExerciseDialog(DialogAction::Create);
        const auto saved = EncodeProfile(g_profile);
        ExerciseDialog(DialogAction::Cancel);
        CHECK(EncodeProfile(g_profile) == saved);
        ExerciseDialog(DialogAction::Edit);
        CHECK(SetAlias(g_profile, g_catalog, L"other phrase", {ResultKind::Emoji, L"🚀"}) == AliasResult::Saved);
        SaveProfile();
        ExerciseDialog(DialogAction::DeclineReplacement);
        ExerciseDialog(DialogAction::Delete);
        CHECK(DeleteAlias(g_profile, L"other phrase"));
        SaveProfile();
        CHECK(g_storage.Load().profile.aliases.empty());
        SetWindowTextW(g_edit, L"red heart");
        ExerciseDialog(DialogAction::CheckHeartPrefill);
        SetWindowTextW(g_edit, L"rocket");

        const auto clipboard = GetClipboardSequenceNumber();
        if (!g_inputPlatform.HeldModifiers().empty()) {
            report("SKIPPED: editor/state checks completed; release modifier keys before real input verification.");
            DestroyWindow(g_window);
            return 77;
        }
        ShowWindow(g_window, SW_SHOW);
        SetForegroundWindow(g_window);
        // The driver must own foreground, as the real picker would after a user
        // hotkey/click. A background shell may be denied that permission. Do not
        // report its synthetic setup as an application insertion regression.
        if (GetForegroundWindow() != g_window) {
            report("SKIPPED: editor/state checks completed; desktop did not grant picker foreground activation. Actual insertion remains unverified.");
            DestroyWindow(g_window);
            return 77;
        }
        // Uses the exact destination captured before all editor visits and learning checks.
        InsertSelection();
        CHECK(g_recoveryMessage.empty());
        CHECK(WaitText(GetDlgItem(targetProcess.window, kEdit), L"🚀"));
        CHECK(GetClipboardSequenceNumber() == clipboard);
        CHECK(QueryCount(g_profile, L"rocket", {ResultKind::Emoji, L"🚀"}) == 1);
        report("PASS: alias/favorite editing persisted; failures did not learn; successful insert/copy learned once; picker order and selection survived repeat insertion, rows, tone and font changes; next session applied learning; original target received exact emoji; clipboard unchanged.");
        PostMessageW(targetProcess.window, WM_CLOSE, 0, 0);
        DestroyWindow(g_window);
    } catch (const std::exception& error) {
        report(std::string("FAIL: ") + error.what() + (dialogFailure.empty() ? "" : " / " + dialogFailure) + " / " + WideToUtf8(g_recoveryMessage));
        if (g_window) DestroyWindow(g_window);
        return 1;
    }
}
