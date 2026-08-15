#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_3.h>

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
    std::wstring glyph;
    std::wstring name;
    std::wstring keywords;
};

struct EmojiFont {
    std::wstring name;
    bool color;
};

HWND g_window{};
HWND g_edit{};
HWND g_list{};
WNDPROC g_editProc{};
WNDPROC g_listProc{};
HBRUSH g_backgroundBrush{};
HBRUSH g_inputBrush{};
HFONT g_emojiFont{};
size_t g_emojiFontIndex{};
std::vector<EmojiFont> g_emojiFonts;
NOTIFYICONDATAW g_tray{};
ID2D1Factory* g_d2dFactory{};
ID2D1DCRenderTarget* g_d2dTarget{};
IDWriteFactory3* g_dwriteFactory{};
IDWriteTextFormat* g_emojiFormat{};
std::vector<const Emoji*> g_visible;
std::vector<std::wstring> g_history;
std::vector<Emoji> g_emojis;

std::wstring Lower(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return text;
}

bool Matches(const Emoji& emoji, const std::wstring& query) {
    if (query.empty()) return true;
    const auto text = Lower(emoji.name + L" " + emoji.keywords);
    return text.find(query) != std::wstring::npos;
}

std::wstring EmojiCatalogPath() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    std::wstring executable(path);
    const size_t slash = executable.find_last_of(L"\\/");
    return (slash == std::wstring::npos ? L"" : executable.substr(0, slash + 1)) + L"emojis.txt";
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return L"";
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (!length) return L"";
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                        result.data(), length);
    return result;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return "";
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (!length) return "";
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                        result.data(), length, nullptr, nullptr);
    return result;
}

bool LoadEmojis() {
    const std::wstring path = EmojiCatalogPath();
    std::ifstream file(path.c_str(), std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t split = line.find_first_of(" \t");
        if (split == std::string::npos) continue;
        const size_t nameStart = line.find_first_not_of(" \t", split);
        if (nameStart == std::string::npos) continue;
        const std::wstring glyph = Utf8ToWide(line.substr(0, split));
        const std::wstring name = Utf8ToWide(line.substr(nameStart));
        if (!glyph.empty() && !name.empty()) g_emojis.push_back({glyph, name, name});
    }
    return !g_emojis.empty();
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
    std::ifstream file(path.c_str(), std::ios::binary);
    std::string line;
    while (std::getline(file, line) && g_history.size() < kMaxHistory) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::wstring emoji = Utf8ToWide(line);
        if (!emoji.empty()) g_history.push_back(emoji);
    }
}

void SaveHistory() {
    const std::wstring path = HistoryPath();
    std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);
    for (const auto& item : g_history) file << WideToUtf8(item) << '\n';
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
            for (const auto& emoji : g_emojis) {
                if (recent == emoji.glyph && added.insert(recent).second) {
                    g_visible.push_back(&emoji);
                    break;
                }
            }
        }
    }
    for (const auto& emoji : g_emojis) {
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

void SelectEmojiFont(size_t index) {
    if (g_emojiFonts.empty()) return;
    g_emojiFontIndex = index % g_emojiFonts.size();
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    if (g_emojiFont) DeleteObject(g_emojiFont);
    g_emojiFont = CreateFontW(-22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, font.name.c_str());
    if (g_emojiFormat) {
        g_emojiFormat->Release();
        g_emojiFormat = nullptr;
    }
    if (g_dwriteFactory) {
        g_dwriteFactory->CreateTextFormat(font.name.c_str(), nullptr,
                                          DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL, 22.0f, L"",
                                          &g_emojiFormat);
    }
    if (g_window) {
        const std::wstring title = std::wstring(L"WinMoji — ") + font.name +
                                   (font.color ? L" (color)" : L" (monochrome)");
        SetWindowTextW(g_window, title.c_str());
    }
    if (g_list) InvalidateRect(g_list, nullptr, TRUE);
}

void CycleEmojiFont() {
    SelectEmojiFont(g_emojiFontIndex + 1);
}

void InitializeColorEmojiDrawing() {
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory))) return;
    D2D1_RENDER_TARGET_PROPERTIES properties{};
    properties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    if (FAILED(g_d2dFactory->CreateDCRenderTarget(&properties, &g_d2dTarget))) return;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3),
                                   reinterpret_cast<IUnknown**>(&g_dwriteFactory)))) return;
}

void LoadInstalledColorEmojiFonts() {
    if (!g_dwriteFactory) {
        g_emojiFonts.push_back({L"Segoe UI Emoji", false});
        return;
    }
    IDWriteFontCollection* collection{};
    IDWriteFactory* baseFactory = g_dwriteFactory;
    if (FAILED(baseFactory->GetSystemFontCollection(&collection, FALSE))) return;
    for (UINT32 i = 0; i < collection->GetFontFamilyCount(); ++i) {
        IDWriteFontFamily* family{};
        IDWriteFont* font{};
        IDWriteFontFace* face{};
        IDWriteFontFace4* colorFace{};
        IDWriteLocalizedStrings* names{};
        if (FAILED(collection->GetFontFamily(i, &family)) ||
            FAILED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                                DWRITE_FONT_STYLE_NORMAL, &font)) ||
            FAILED(font->CreateFontFace(&face)) ||
            FAILED(face->QueryInterface(&colorFace)) || !colorFace->IsColorFont()) {
            if (colorFace) colorFace->Release();
            if (face) face->Release();
            if (font) font->Release();
            if (family) family->Release();
            continue;
        }
        const BOOL containsGrinningFace = colorFace->HasCharacter(0x1F600);
        if (containsGrinningFace && SUCCEEDED(family->GetFamilyNames(&names))) {
            UINT32 locale = 0;
            BOOL foundLocale = FALSE;
            names->FindLocaleName(L"en-us", &locale, &foundLocale);
            if (!foundLocale) locale = 0;
            UINT32 length = 0;
            names->GetStringLength(locale, &length);
            std::wstring name(length + 1, L'\0');
            names->GetString(locale, name.data(), length + 1);
            name.resize(length);
            g_emojiFonts.push_back({name, true});
            names->Release();
        }
        colorFace->Release();
        face->Release();
        font->Release();
        family->Release();
    }
    collection->Release();
    // Keep genuine GDI-style monochrome rendering available after the color choices.
    g_emojiFonts.push_back({L"Segoe UI Emoji", false});
    g_emojiFonts.push_back({L"Segoe UI Symbol", false});
}

void DrawColorEmoji(HDC dc, const RECT& bounds, const std::wstring& glyph) {
    if (!g_d2dTarget || !g_emojiFormat || FAILED(g_d2dTarget->BindDC(dc, &bounds))) {
        const HFONT previous = static_cast<HFONT>(SelectObject(dc, g_emojiFont));
        DrawTextW(dc, glyph.c_str(), -1, const_cast<RECT*>(&bounds),
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        SelectObject(dc, previous);
        return;
    }
    g_d2dTarget->BeginDraw();
    ID2D1SolidColorBrush* brush{};
    const D2D1_COLOR_F textColor{0.92f, 0.92f, 0.92f, 1.0f};
    if (SUCCEEDED(g_d2dTarget->CreateSolidColorBrush(textColor, &brush))) {
        // A DCRenderTarget's coordinate system starts at its current BindDC clip rectangle.
        const D2D1_RECT_F textBounds{0.0f, 0.0f,
                                     static_cast<float>(bounds.right - bounds.left),
                                     static_cast<float>(bounds.bottom - bounds.top)};
        const auto options = g_emojiFonts[g_emojiFontIndex].color
                                 ? D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
                                 : D2D1_DRAW_TEXT_OPTIONS_NONE;
        g_d2dTarget->DrawTextW(glyph.c_str(), static_cast<UINT32>(glyph.size()), g_emojiFormat,
                                textBounds, brush, options,
                                DWRITE_MEASURING_MODE_NATURAL);
        brush->Release();
    }
    g_d2dTarget->EndDraw();
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
        if (wParam == 'F' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            CycleEmojiFont();
            return 0;
        }
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
        RECT glyph = item->rcItem;
        glyph.left += 10;
        glyph.right = glyph.left + 30;
        DrawColorEmoji(item->hDC, glyph, emoji.glyph);

        RECT text = item->rcItem;
        text.left += 50;
        text.right -= 10;
        DrawTextW(item->hDC, emoji.name.c_str(), -1, &text, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
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
    if (!LoadEmojis()) {
        MessageBoxW(nullptr, L"Kunne ikke lese emojis.txt ved siden av WinMoji.exe.", L"WinMoji", MB_ICONERROR);
        return 1;
    }
    LoadHistory();
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.lpfnWndProc = WindowProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    g_backgroundBrush = CreateSolidBrush(kBackground);
    g_inputBrush = CreateSolidBrush(kInputBackground);
    InitializeColorEmojiDrawing();
    LoadInstalledColorEmojiFonts();
    // Only the glyph column uses this font; labels retain the regular UI font.
    SelectEmojiFont(0);
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
    if (g_emojiFormat) g_emojiFormat->Release();
    if (g_dwriteFactory) g_dwriteFactory->Release();
    if (g_d2dTarget) g_d2dTarget->Release();
    if (g_d2dFactory) g_d2dFactory->Release();
    DeleteObject(g_emojiFont);
    DeleteObject(g_inputBrush);
    DeleteObject(g_backgroundBrush);
    return static_cast<int>(message.wParam);
}
