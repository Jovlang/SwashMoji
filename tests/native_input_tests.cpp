#include "insertion_win32.h"
#include "test_support.h"
#include <filesystem>
#include <fstream>
#include <string>

using namespace SwashMoji;
constexpr wchar_t kTargetClass[] = L"SwashMojiM1TestTarget";
constexpr int kEdit = 101;

LRESULT CALLBACK TargetProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE,
            12, 12, 450, 100, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEdit)), GetModuleHandleW(nullptr), nullptr);
        SetTimer(window, 1, 15000, nullptr); // crash-safe lifetime for this test target
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) SetFocus(GetDlgItem(window, kEdit));
        return 0;
    case WM_SETFOCUS: SetFocus(GetDlgItem(window, kEdit)); return 0;
    case WM_TIMER: DestroyWindow(window); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int RunTarget(const wchar_t* title) {
    WNDCLASSW type{};
    type.hInstance = GetModuleHandleW(nullptr);
    type.lpszClassName = kTargetClass;
    type.lpfnWndProc = TargetProc;
    type.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&type)) return 1;
    const HWND window = CreateWindowExW(WS_EX_TOOLWINDOW, kTargetClass, title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 500, 170, nullptr, nullptr, type.hInstance, nullptr);
    if (!window) return 1;
    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(GetDlgItem(window, kEdit));
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}

struct TargetProcess {
    PROCESS_INFORMATION process{};
    HWND window{}, previous = GetForegroundWindow();
    ~TargetProcess() {
        if (window && GetForegroundWindow() == window && IsWindow(previous)) SetForegroundWindow(previous);
        if (window) PostMessageW(window, WM_CLOSE, 0, 0);
        if (process.hProcess) {
            if (WaitForSingleObject(process.hProcess, 2000) == WAIT_TIMEOUT) TerminateProcess(process.hProcess, 1);
            CloseHandle(process.hProcess);
        }
        if (process.hThread) CloseHandle(process.hThread);
    }
};

bool WaitText(HWND edit, const std::wstring& expected) {
    const auto deadline = GetTickCount64() + 1500;
    do {
        wchar_t text[4096]{};
        DWORD_PTR result{};
        if (SendMessageTimeoutW(edit, WM_GETTEXT, std::size(text), reinterpret_cast<LPARAM>(text),
                                SMTO_ABORTIFHUNG, 100, &result) && text == expected) return true;
        Sleep(10);
    } while (GetTickCount64() < deadline);
    return false;
}

int wmain(int argc, wchar_t** argv) {
    if (argc == 3 && std::wstring(argv[1]) == L"--target") return RunTarget(argv[2]);
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    const auto report = [executablePath = std::filesystem::path(executable)](const std::string& message) {
        std::ofstream file(executablePath.parent_path() / L"native-input-result.txt", std::ios::trunc);
        file << message << '\n';
    };
    report("RUNNING");
    try {
        Win32InputPlatform platform;
        if (!platform.HeldModifiers().empty()) {
            std::cout << "SKIPPED: release modifier keys before running the native input test.\n";
            report("SKIPPED: modifier keys were held.");
            return 77;
        }
        TargetProcess targetProcess;
        CHECK(executable[0]);
        const auto title = L"SwashMoji input verification " + std::to_wstring(GetCurrentProcessId());
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
        if (!targetProcess.window) {
            DWORD code{};
            GetExitCodeProcess(targetProcess.process.hProcess, &code);
            std::cerr << "Native target did not appear; process exit/status: " << code << '\n';
        }
        CHECK(targetProcess.window);
        const auto target = CaptureExternalTarget(targetProcess.window);
        CHECK(target.window && platform.ValidTarget(target));
        const HWND edit = GetDlgItem(targetProcess.window, kEdit);
        CHECK(edit);
        if (platform.Foreground() != target.window) {
            platform.Activate(target.window);
            if (platform.Foreground() != target.window) {
                std::cout << "SKIPPED: this desktop did not grant foreground activation.\n";
                report("SKIPPED: desktop did not grant foreground activation.");
                return 77;
            }
        }
        const auto clipboardSequence = GetClipboardSequenceNumber();
        const std::wstring text = L"👍🏽👩‍💻🚀✨";
        CHECK(InsertText(platform, target, text).status == InsertionStatus::FullySubmitted);
        CHECK(WaitText(edit, text));
        CHECK(InsertText(platform, target, L"✅").status == InsertionStatus::FullySubmitted);
        CHECK(WaitText(edit, text + L"✅"));
        CHECK(GetClipboardSequenceNumber() == clipboardSequence);

        // Internal windows and stale identities must never become destinations.
        const HWND own = CreateWindowExW(0, L"STATIC", L"M1 internal window", WS_POPUP,
            0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        CHECK(own);
        const bool rejectedOwn = !CaptureExternalTarget(own).window;
        DestroyWindow(own);
        CHECK(rejectedOwn);
        auto stale = target;
        ++stale.process;
        CHECK(!platform.ValidTarget(stale));
        PostMessageW(targetProcess.window, WM_CLOSE, 0, 0);
        CHECK(WaitForSingleObject(targetProcess.process.hProcess, 2000) == WAIT_OBJECT_0);
        CHECK(InsertText(platform, target, text).status == InsertionStatus::NoTarget);
        std::cout << "Native Win32 edit received exact emoji sequences and repeat input; clipboard unchanged; stale/internal targets rejected.\n";
        report("PASS: native Win32 edit received exact multi-code-point emoji and repeat input; clipboard unchanged; stale/internal targets rejected.");
    } catch (const std::exception& error) { report(std::string("FAIL: ") + error.what()); std::cerr << error.what() << '\n'; return 1; }
}
