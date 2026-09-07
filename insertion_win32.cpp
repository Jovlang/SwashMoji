#include "insertion_win32.h"
#include <cstring>
#include <limits>

namespace SwashMoji {
InputTarget CaptureExternalTarget(HWND window) {
    window = GetAncestor(window, GA_ROOT);
    if (!window || !IsWindow(window) || window == GetDesktopWindow() || window == GetShellWindow()) return {};
    DWORD process{};
    const DWORD thread = GetWindowThreadProcessId(window, &process);
    if (!thread || process == GetCurrentProcessId()) return {};
    wchar_t name[64]{};
    GetClassNameW(window, name, static_cast<int>(std::size(name)));
    if (std::wstring(name) == L"Shell_TrayWnd" || std::wstring(name) == L"Shell_SecondaryTrayWnd") return {};
    return {reinterpret_cast<WindowToken>(window), process, thread};
}

bool Win32InputPlatform::ValidTarget(const InputTarget& target) {
    const auto window = reinterpret_cast<HWND>(target.window);
    const auto current = CaptureExternalTarget(window);
    return current.window && current.window == target.window && current.process == target.process && current.thread == target.thread;
}
WindowToken Win32InputPlatform::Foreground() { return reinterpret_cast<WindowToken>(GetForegroundWindow()); }
void Win32InputPlatform::Activate(WindowToken window) {
    const auto target = reinterpret_cast<HWND>(window);
    if (SetForegroundWindow(target)) {
        // Activation of another input queue can complete asynchronously. Give
        // that queue a bounded chance to handle activation before verifying it.
        DWORD_PTR ignored{};
        SendMessageTimeoutW(target, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 100, &ignored);
    }
}
std::vector<std::uint16_t> Win32InputPlatform::HeldModifiers() {
    std::vector<std::uint16_t> keys;
    for (const auto key : {VK_LSHIFT, VK_RSHIFT, VK_LCONTROL, VK_RCONTROL, VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN}) {
        if (GetAsyncKeyState(key) & 0x8000) keys.push_back(static_cast<std::uint16_t>(key));
    }
    return keys;
}
size_t Win32InputPlatform::Send(const std::vector<KeyEvent>& events) {
    if (events.empty() || events.size() > std::numeric_limits<UINT>::max()) return 0;
    std::vector<INPUT> inputs;
    inputs.reserve(events.size());
    for (const auto& event : events) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        if (event.unicode) { input.ki.wScan = event.code; input.ki.dwFlags = KEYEVENTF_UNICODE; }
        else input.ki.wVk = event.code;
        if (event.up) input.ki.dwFlags |= KEYEVENTF_KEYUP;
        if (event.extended) input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        inputs.push_back(input);
    }
    return SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

Win32ClipboardPlatform::~Win32ClipboardPlatform() { if (open_) Close(); Release(); }
bool Win32ClipboardPlatform::Prepare(const std::wstring& text) {
    Release();
    if (!ValidInputText(text)) return false;
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    memory_ = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory_) return false;
    void* output = GlobalLock(memory_);
    if (!output) { Release(); return false; }
    memcpy(output, text.c_str(), bytes);
    SetLastError(ERROR_SUCCESS);
    const BOOL stillLocked = GlobalUnlock(memory_);
    if (stillLocked || GetLastError() != ERROR_SUCCESS) { Release(); return false; }
    return true;
}
bool Win32ClipboardPlatform::Open() { open_ = OpenClipboard(owner_) != FALSE; return open_; }
bool Win32ClipboardPlatform::Empty() { return EmptyClipboard() != FALSE; }
bool Win32ClipboardPlatform::Publish() {
    if (!memory_ || !SetClipboardData(CF_UNICODETEXT, memory_)) return false;
    memory_ = nullptr;
    return true;
}
bool Win32ClipboardPlatform::Close() {
    if (!open_) return true;
    if (!CloseClipboard()) return false;
    open_ = false;
    return true;
}
void Win32ClipboardPlatform::Release() { if (memory_) { GlobalFree(memory_); memory_ = nullptr; } }
}
