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
enum class DialogAction { Create, Cancel, Edit, DeclineReplacement, Delete, CheckHeartPrefill };
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
        if (action == DialogAction::CheckHeartPrefill) {
            wchar_t targetQuery[32]{};
            GetDlgItemTextW(dialog, IDC_TARGET_QUERY, targetQuery, 32);
            CHECK(std::wstring(targetQuery) == L"❤️");
            CHECK(SendDlgItemMessageW(dialog, IDC_TARGET_RESULTS, LB_GETCOUNT, 0, 0) == 1);
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
        g_storage = ProfileStorage(directory / (L"vocabulary-test-" + std::to_wstring(GetCurrentProcessId())));
        g_profile = g_storage.Load().profile;
        g_uiFont = g_statusFont = g_emojiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_backgroundBrush = CreateSolidBrush(kBackground);
        g_inputBrush = CreateSolidBrush(kInputBackground);
        WNDCLASSW type{};
        type.hInstance = GetModuleHandleW(nullptr);
        type.lpszClassName = kClassName;
        type.lpfnWndProc = WindowProc;
        type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        CHECK(RegisterClassW(&type));
        g_window = CreateWindowExW(WS_EX_TOOLWINDOW, kClassName, L"SwashMoji M2 test picker", WS_POPUP,
            300, 300, kPickerWidth, PickerHeight(), nullptr, nullptr, type.hInstance, nullptr);
        CHECK(g_window);

        TargetProcess targetProcess;
        wchar_t executable[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executable, MAX_PATH);
        const auto title = L"SwashMoji M2 target " + std::to_wstring(GetCurrentProcessId());
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
        CHECK(g_inputPlatform.HeldModifiers().empty());
        // Uses the exact destination captured before all six editor visits.
        InsertSelection();
        CHECK(g_recoveryMessage.empty());
        CHECK(WaitText(GetDlgItem(targetProcess.window, kEdit), L"🚀"));
        CHECK(GetClipboardSequenceNumber() == clipboard);
        report("PASS: editor create/edit/delete persisted; cancelled draft and replacement left data intact; picker query, selection and original target survived; real target received exact emoji, clipboard unchanged.");
        PostMessageW(targetProcess.window, WM_CLOSE, 0, 0);
        DestroyWindow(g_window);
    } catch (const std::exception& error) {
        report(std::string("FAIL: ") + error.what() + (dialogFailure.empty() ? "" : " / " + dialogFailure));
        if (g_window) DestroyWindow(g_window);
        return 1;
    }
}
