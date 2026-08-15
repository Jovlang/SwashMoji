#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

constexpr wchar_t kClassName[] = L"WinMojiPicker";
constexpr UINT kHotkeyId = 1;
constexpr int kPickerWidth = 500;
constexpr int kPickerHeight = 390;
constexpr int kEditId = 100;
constexpr int kListId = 101;
constexpr int kExitId = 200;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr size_t kMaxHistory = 40;
constexpr COLORREF kBackground = RGB(24, 24, 24);
constexpr COLORREF kInputBackground = RGB(35, 35, 35);
constexpr COLORREF kText = RGB(235, 235, 235);
constexpr COLORREF kSelected = RGB(38, 79, 120);

struct Emoji {
    const wchar_t* glyph;
    const wchar_t* name;
    const wchar_t* keywords;
};

// Keep this deliberately boring: one built-in table and ordinary Win32 controls.
constexpr Emoji kEmoji[] = {
    {L"😀", L"grinning face", L"smile happy"}, {L"😃", L"smiley face", L"happy joy"},
    {L"😄", L"smile face", L"happy grin"}, {L"😁", L"beaming face", L"happy teeth"},
    {L"😆", L"laughing face", L"happy lol"}, {L"😅", L"sweat smile", L"nervous"},
    {L"😂", L"face with tears", L"laugh cry funny"}, {L"🙂", L"slightly smiling face", L"smile"},
    {L"🙃", L"upside down face", L"silly"}, {L"😉", L"winking face", L"wink"},
    {L"😊", L"smiling eyes", L"blush happy"}, {L"😍", L"heart eyes", L"love"},
    {L"😘", L"face blowing kiss", L"love kiss"}, {L"😎", L"sunglasses", L"cool"},
    {L"🤔", L"thinking face", L"hmm question"}, {L"😢", L"crying face", L"sad tear"},
    {L"😭", L"loudly crying face", L"sad tears"}, {L"😡", L"angry face", L"mad rage"},
    {L"👍", L"thumbs up", L"like yes approve"}, {L"👎", L"thumbs down", L"dislike no"},
    {L"👏", L"clapping hands", L"applause bravo"}, {L"🙏", L"folded hands", L"please thanks pray"},
    {L"👋", L"waving hand", L"hello goodbye wave"}, {L"🤝", L"handshake", L"deal agree"},
    {L"💪", L"flexed biceps", L"strong muscle"}, {L"👌", L"ok hand", L"okay perfect"},
    {L"✌️", L"victory hand", L"peace two"}, {L"🤞", L"crossed fingers", L"luck hope"},
    {L"🔥", L"fire", L"lit hot flame"}, {L"✨", L"sparkles", L"stars magic"},
    {L"⭐", L"star", L"favorite"}, {L"❤️", L"red heart", L"love"},
    {L"💔", L"broken heart", L"sad love"}, {L"💯", L"hundred points", L"perfect score"},
    {L"✅", L"check mark", L"done yes success"}, {L"❌", L"cross mark", L"no close wrong"},
    {L"⚠️", L"warning", L"alert caution"}, {L"🎉", L"party popper", L"celebrate congratulations"},
    {L"🎂", L"birthday cake", L"birthday celebrate"}, {L"🎁", L"wrapped gift", L"present birthday"},
    {L"🚀", L"rocket", L"launch ship fast"}, {L"💡", L"light bulb", L"idea tip"},
    {L"📌", L"pushpin", L"pin important"}, {L"📎", L"paperclip", L"attachment"},
    {L"📅", L"calendar", L"date schedule"}, {L"💻", L"laptop", L"computer code"},
    {L"🐶", L"dog", L"puppy pet"}, {L"🐱", L"cat", L"kitten pet"},
    {L"🐼", L"panda", L"animal bear"}, {L"🦊", L"fox", L"animal"},
    {L"🍕", L"pizza", L"food"}, {L"🍔", L"hamburger", L"food burger"},
    {L"☕", L"hot beverage", L"coffee tea"}, {L"🍺", L"beer mug", L"drink cheers"},
    {L"🌍", L"globe", L"world earth"}, {L"☀️", L"sun", L"weather sunny"},
    {L"🌈", L"rainbow", L"weather color"}, {L"🎵", L"musical note", L"music sound"},
    {L"✅", L"check", L"success done"}, {L"➡️", L"right arrow", L"next forward"},
};

HWND g_window{};
HWND g_edit{};
HWND g_list{};
WNDPROC g_editProc{};
WNDPROC g_listProc{};
HBRUSH g_backgroundBrush{};
HBRUSH g_inputBrush{};
NOTIFYICONDATAW g_tray{};
std::vector<const Emoji*> g_visible;
std::vector<std::wstring> g_history;

std::wstring Lower(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return text;
}

bool IsSubsequence(const std::wstring& needle, const std::wstring& haystack) {
    size_t at = 0;
    for (wchar_t c : haystack) if (at < needle.size() && c == needle[at]) ++at;
    return at == needle.size();
}

bool Matches(const Emoji& emoji, const std::wstring& query) {
    if (query.empty()) return true;
    const auto text = Lower(std::wstring(emoji.name) + L" " + emoji.keywords);
    return text.find(query) != std::wstring::npos || IsSubsequence(query, text);
}

std::wstring HistoryPath() {
    wchar_t path[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) return L"history.txt";
    std::wstring folder = std::wstring(path) + L"\\WinMoji";
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder + L"\\history.txt";
}

void LoadHistory() {
    const std::wstring path = HistoryPath();
    std::wifstream file(path.c_str());
    std::wstring line;
    while (std::getline(file, line) && g_history.size() < kMaxHistory) {
        if (!line.empty()) g_history.push_back(line);
    }
}

void SaveHistory() {
    const std::wstring path = HistoryPath();
    std::wofstream file(path.c_str(), std::ios::trunc);
    for (const auto& item : g_history) file << item << L'\n';
}

void Remember(const wchar_t* glyph) {
    std::wstring value(glyph);
    g_history.erase(std::remove(g_history.begin(), g_history.end(), value), g_history.end());
    g_history.insert(g_history.begin(), value);
    if (g_history.size() > kMaxHistory) g_history.resize(kMaxHistory);
    SaveHistory();
}

void RefreshList() {
    wchar_t input[256]{};
    GetWindowTextW(g_edit, input, static_cast<int>(std::size(input)));
    const std::wstring query = Lower(input);
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    g_visible.clear();
    std::unordered_set<std::wstring> added;

    // Empty searches lead with recently copied emoji; filtered results preserve table order.
    if (query.empty()) {
        for (const auto& recent : g_history) {
            for (const auto& emoji : kEmoji) {
                if (recent == emoji.glyph && added.insert(recent).second) {
                    g_visible.push_back(&emoji);
                    break;
                }
            }
        }
    }
    for (const auto& emoji : kEmoji) {
        if (Matches(emoji, query) && added.insert(emoji.glyph).second) g_visible.push_back(&emoji);
    }
    for (const Emoji* emoji : g_visible) {
        std::wstring row = std::wstring(emoji->glyph) + L"    " + emoji->name;
        SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
    if (!g_visible.empty()) SendMessageW(g_list, LB_SETCURSEL, 0, 0);
}

void CopySelection() {
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_visible.size())) return;
    const std::wstring value = g_visible[selected]->glyph;
    if (!OpenClipboard(g_window)) return;
    EmptyClipboard();
    const size_t bytes = (value.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
        auto* output = static_cast<wchar_t*>(GlobalLock(memory));
        if (output) {
            memcpy(output, value.c_str(), bytes);
            GlobalUnlock(memory);
            if (SetClipboardData(CF_UNICODETEXT, memory)) { // Clipboard owns memory after success.
                memory = nullptr;
                Remember(value.c_str());
            }
        }
        if (memory) GlobalFree(memory);
    }
    CloseClipboard();
    ShowWindow(g_window, SW_HIDE);
}

void CenterOnActiveMonitor() {
    HWND active = GetForegroundWindow();
    HMONITOR monitor = MonitorFromWindow(active, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    const RECT& area = info.rcWork;
    const int x = area.left + ((area.right - area.left) - kPickerWidth) / 2;
    const int y = area.top + ((area.bottom - area.top) - kPickerHeight) / 2;
    SetWindowPos(g_window, HWND_TOPMOST, x, y, kPickerWidth, kPickerHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetForegroundWindow(g_window);
    SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, -1);
}

void AddTrayIcon(HWND window) {
    g_tray = {};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = window;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_tray.uCallbackMessage = kTrayMessage;
    g_tray.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpyW(g_tray.szTip, L"WinMoji — Alt+E");
    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kExitId, L"Exit");
    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(g_window);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        point.x, point.y, 0, g_window, nullptr);
    DestroyMenu(menu);
    // Lets the taskbar dismiss the menu correctly after a tray interaction.
    PostMessageW(g_window, WM_NULL, 0, 0);
    if (command == kExitId) DestroyWindow(g_window);
}

void LayoutChildren(HWND window) {
    RECT area{};
    GetClientRect(window, &area);
    constexpr int margin = 12;
    constexpr int editHeight = 30;
    MoveWindow(g_edit, margin, margin, area.right - margin * 2, editHeight, TRUE);
    MoveWindow(g_list, margin, margin + editHeight + 8, area.right - margin * 2,
               area.bottom - (margin * 2 + editHeight + 8), TRUE);
}

void MoveSelection(int direction) {
    if (g_visible.empty()) return;
    int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected == LB_ERR) selected = 0;
    selected = std::clamp(selected + direction, 0, static_cast<int>(g_visible.size()) - 1);
    SendMessageW(g_list, LB_SETCURSEL, selected, 0);
}

LRESULT CALLBACK InputProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN) {
        if (wParam == VK_ESCAPE) {
            ShowWindow(g_window, SW_HIDE);
            return 0;
        }
        if (wParam == VK_RETURN) {
            CopySelection();
            return 0;
        }
        if (wParam == VK_UP) {
            MoveSelection(-1);
            return 0;
        }
        if (wParam == VK_DOWN) {
            MoveSelection(1);
            return 0;
        }
    }
    const WNDPROC original = control == g_edit ? g_editProc : g_listProc;
    return CallWindowProcW(original, control, message, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, window, reinterpret_cast<HMENU>(kEditId), nullptr, nullptr);
        g_list = CreateWindowExW(0, L"LISTBOX", L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                 LBS_OWNERDRAWFIXED,
                                 0, 0, 0, 0, window, reinterpret_cast<HMENU>(kListId), nullptr, nullptr);
        SendMessageW(g_edit, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SendMessageW(g_list, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        SetWindowTheme(g_edit, L"DarkMode_Explorer", nullptr);
        SetWindowTheme(g_list, L"DarkMode_Explorer", nullptr);
        g_editProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_edit, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        g_listProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_list, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        LayoutChildren(window);
        RefreshList();
        AddTrayIcon(window);
        return 0;
    }
    case WM_SIZE: LayoutChildren(window); return 0;
    case WM_MEASUREITEM:
        if (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlID == kListId) {
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = 32;
            return TRUE;
        }
        break;
    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID != kListId || item->itemID >= g_visible.size()) break;
        const bool selected = (item->itemState & ODS_SELECTED) != 0;
        HBRUSH fill = CreateSolidBrush(selected ? kSelected : kBackground);
        FillRect(item->hDC, &item->rcItem, fill);
        DeleteObject(fill);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, kText);
        const Emoji& emoji = *g_visible[item->itemID];
        const std::wstring row = std::wstring(emoji.glyph) + L"    " + emoji.name;
        RECT text = item->rcItem;
        text.left += 10;
        DrawTextW(item->hDC, row.c_str(), -1, &text, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
        if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
        return TRUE;
    }
    case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wParam), kText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kInputBackground);
        return reinterpret_cast<LRESULT>(g_inputBrush);
    case WM_CTLCOLORLISTBOX:
        SetTextColor(reinterpret_cast<HDC>(wParam), kText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kBackground);
        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    case WM_COMMAND:
        if (LOWORD(wParam) == kEditId && HIWORD(wParam) == EN_CHANGE) RefreshList();
        if (LOWORD(wParam) == kListId && HIWORD(wParam) == LBN_DBLCLK) CopySelection();
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) { ShowWindow(window, SW_HIDE); return 0; }
        if (wParam == VK_RETURN) { CopySelection(); return 0; }
        break;
    case WM_HOTKEY:
        if (wParam == kHotkeyId) {
            SetWindowTextW(g_edit, L"");
            CenterOnActiveMonitor();
        }
        return 0;
    case kTrayMessage:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
        case NIN_SELECT:
        case NIN_KEYSELECT:
            CenterOnActiveMonitor();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu();
            break;
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE) { ShowWindow(window, SW_HIDE); return 0; }
        break;
    case WM_DESTROY:
        UnregisterHotKey(window, kHotkeyId);
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    LoadHistory();
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.lpfnWndProc = WindowProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    g_backgroundBrush = CreateSolidBrush(kBackground);
    g_inputBrush = CreateSolidBrush(kInputBackground);
    wc.hbrBackground = g_backgroundBrush;
    if (!RegisterClassW(&wc)) return 1;

    g_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kClassName, L"WinMoji",
                               WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                               kPickerWidth, kPickerHeight, nullptr, nullptr, instance, nullptr);
    if (!g_window) return 1;
    if (!RegisterHotKey(g_window, kHotkeyId, MOD_ALT | MOD_NOREPEAT, 'E')) {
        MessageBoxW(nullptr, L"Alt+E er allerede i bruk av et annet program.", L"WinMoji", MB_ICONWARNING);
    }

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    DeleteObject(g_inputBrush);
    DeleteObject(g_backgroundBrush);
    return static_cast<int>(message.wParam);
}
