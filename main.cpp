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
#include <dwmapi.h>
#include <uiautomation.h>

#include "search.h"
#include "storage.h"
#include "insertion_win32.h"
#include "vocabulary.h"
#include <windowsx.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
using namespace SwashMoji;

constexpr wchar_t kClassName[] = L"SwashMojiWindow";
constexpr wchar_t kHelpClassName[] = L"SwashMojiHelpWindow";
constexpr UINT kHotkeyId = 1;
constexpr int kPickerWidth = 500;
constexpr int kPickerHeight = 136;
constexpr int kInputHeight = 34;
constexpr int kResultSize = 48;
constexpr int kEmojiColumns = 10;
constexpr int kMinEmojiRows = 1;
constexpr int kMaxEmojiRows = 3;
constexpr int kStatusHeight = 18;
constexpr int kHelpWidth = 720;
constexpr int kHelpHeight = 840;
constexpr int kEditId = 100;
constexpr int kListId = 101;
constexpr int kStatusId = 102;
constexpr int kRecoveryId = 103;
constexpr int kCopyInsteadId = 104;
constexpr int kTeachPhraseId = 105;
constexpr int kVocabularyId = 205;
constexpr int kRecoveryHeight = 68;
constexpr int kExitId = 200;
constexpr int kPositionAboveTextFieldId = 201;
constexpr int kSortRecentId = 202;
constexpr int kSortMostUsedId = 203;
constexpr int kClearUsageHistoryId = 204;
constexpr int kAppIconId = 101;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kShowPickerMessage = WM_APP + 2;
constexpr UINT_PTR kStatusTimerId = 1;
constexpr UINT_PTR kReturnTimerBase = 16;

constexpr COLORREF kBackground = RGB(24, 24, 24);
constexpr COLORREF kInputBackground = RGB(35, 35, 35);
constexpr COLORREF kText = RGB(235, 235, 235);
constexpr COLORREF kMutedText = RGB(155, 155, 155);
constexpr COLORREF kSelected = RGB(38, 79, 120);

struct EmojiFont {
    std::wstring name;
    bool color;
};

HWND g_window{};
HWND g_edit{};
HWND g_list{};
HWND g_status{};
HWND g_helpWindow{};
HWND g_recoveryLabel{};
HWND g_copyInstead{};
HWND g_teachPhrase{};
bool g_vocabularyOpen{};
std::wstring g_recoveryMessage;
Win32InputPlatform g_inputPlatform;
InputTarget g_inputTarget;
FocusReturn g_focusReturn;
UINT_PTR g_returnTimer{};
DWORD g_returnArmedAt{};
HWINEVENTHOOK g_foregroundHook{};
PickerSession g_session;
Catalog g_catalog;
Profile g_profile;
ProfileStorage g_storage;
bool g_profileUnsaved{};
std::wstring g_storageDiagnostic;
bool& g_positionAboveTextField = g_profile.settings.positionAboveTextField;
bool& g_sortByUsage = g_profile.settings.sortByUsage;
bool g_statusVisible{true};
int& g_emojiRows = g_profile.settings.emojiRows;
int& g_skinToneIndex = g_profile.settings.skinTone;
WNDPROC g_editProc{};
WNDPROC g_listProc{};
HBRUSH g_backgroundBrush{};
HBRUSH g_inputBrush{};
HICON g_appIcon{};
HFONT g_uiFont{};
HFONT g_statusFont{};
HFONT g_helpTitleFont{};
HFONT g_helpHeadingFont{};
HFONT g_helpBodyFont{};
HFONT g_emojiFont{};
size_t g_emojiFontIndex{};
std::vector<EmojiFont> g_emojiFonts;
NOTIFYICONDATAW g_tray{};
ID2D1Factory* g_d2dFactory{};
ID2D1DCRenderTarget* g_d2dTarget{};
IDWriteFactory3* g_dwriteFactory{};
IUIAutomation* g_uiAutomation{};
bool g_comInitialized{};
IDWriteTextFormat* g_emojiFormat{};
std::vector<SearchResult> g_visible;
std::vector<SearchResult> g_displayVisible;
void UpdateStatusLine();
void UpdateSortIndicator();
int PickerHeight();
void LayoutChildren(HWND window);

std::filesystem::path EmojiCatalogPath() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    return std::filesystem::path(path).parent_path() / L"emojis.txt";
}

bool LoadEmojis() {
    std::ifstream file(EmojiCatalogPath(), std::ios::binary);
    if (!g_catalog.Load(file)) return false;
    std::ifstream intents(EmojiCatalogPath().parent_path() / L"intent_phrases.tsv", std::ios::binary);
    return intents && g_catalog.LoadIntents(intents);
}

void LoadProfile() {
    wchar_t path[MAX_PATH]{};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        g_storage = ProfileStorage(std::filesystem::path(path) / L"SwashMoji",
                                   std::filesystem::path(path) / L"WinMoji");
    } else {
        g_storage = ProfileStorage(std::filesystem::current_path());
    }
    auto loaded = g_storage.Load();
    g_profile = std::move(loaded.profile);
    g_profileUnsaved = loaded.unsaved || g_storage.ReadOnly();
    g_storageDiagnostic = std::move(loaded.diagnostic);
}

void SaveProfile() {
    g_profileUnsaved = !g_storage.Save(g_profile, g_storageDiagnostic);
    UpdateStatusLine();
    if (g_tray.hWnd) UpdateSortIndicator();
}

void SaveSettings() { SaveProfile(); }

void Remember(const wchar_t* glyph) {
    SwashMoji::Remember(g_profile, glyph);
    SaveProfile();
}

void RefreshList() {
    wchar_t input[256]{};
    GetWindowTextW(g_edit, input, static_cast<int>(std::size(input)));
    g_session.query = input;
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    g_visible = Search(g_catalog, g_profile, g_session.query);
    ShowWindow(g_teachPhrase, g_visible.empty() && !NormalizePhrase(g_session.query).empty() ? SW_SHOW : SW_HIDE);
    g_displayVisible.clear();
    g_session.rankingSnapshot.clear();
    for (const auto& result : g_visible) g_session.rankingSnapshot.push_back(result.id);
    g_session.selected = g_visible.empty() ? ResultId{} : g_visible.front().id;
    // A multi-column Win32 listbox fills each column top-to-bottom. Reorder the
    // listbox items so it still reads left-to-right: 1–10 on the first row, then
    // 11–20 on the next row.
    const size_t columns = (g_visible.size() + g_emojiRows - 1) / g_emojiRows;
    for (size_t column = 0; column < columns; ++column) {
        const size_t page = column / kEmojiColumns;
        const size_t columnInPage = column % kEmojiColumns;
        for (int row = 0; row < g_emojiRows; ++row) {
            const size_t index = page * g_emojiRows * kEmojiColumns +
                                 static_cast<size_t>(row) * kEmojiColumns + columnInPage;
            if (index < g_visible.size()) {
                g_displayVisible.push_back(g_visible[index]);
            }
        }
    }
    for (const auto& result : g_displayVisible) {
        SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(result.payload.c_str()));
    }
    if (!g_displayVisible.empty()) SendMessageW(g_list, LB_SETCURSEL, 0, 0);
}

void CancelPendingReturn() {
    ++g_session.insertionAttempt;
    g_focusReturn.Cancel();
    if (g_returnTimer) KillTimer(g_window, g_returnTimer);
    g_returnTimer = 0;
}

void OpenVocabulary(bool prefill) {
    if (g_vocabularyOpen) return;
    CancelPendingReturn();
    const int index = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    const auto selected = index >= 0 && index < static_cast<int>(g_displayVisible.size())
        ? g_displayVisible[index].id : ResultId{};
    g_vocabularyOpen = true;
    const bool opened = ShowVocabulary(g_window, reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_window, GWLP_HINSTANCE)),
        g_catalog, g_profile, prefill ? g_session.query : L"", prefill ? selected : ResultId{},
        [] { SaveProfile(); return !g_profileUnsaved; });
    g_vocabularyOpen = false;
    if (!opened) MessageBoxW(g_window, L"Could not open My vocabulary.", L"SwashMoji", MB_ICONERROR);
    RefreshList();
    for (size_t i = 0; i < g_displayVisible.size(); ++i) {
        if (g_displayVisible[i].id == selected) {
            SendMessageW(g_list, LB_SETCURSEL, i, 0);
            g_session.selected = selected;
            break;
        }
    }
    if (IsWindowVisible(g_window)) SetFocus(g_edit);
}

void DismissPicker() {
    CancelPendingReturn();
    ShowWindow(g_window, SW_HIDE);
}

void SetRecoveryMessage(const std::wstring& message) {
    g_recoveryMessage = message;
    const bool visible = !message.empty();
    SetWindowTextW(g_recoveryLabel, message.c_str());
    ShowWindow(g_recoveryLabel, visible ? SW_SHOW : SW_HIDE);
    ShowWindow(g_copyInstead, visible ? SW_SHOW : SW_HIDE);
    RECT bounds{};
    GetWindowRect(g_window, &bounds);
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfoW(MonitorFromWindow(g_window, MONITOR_DEFAULTTONEAREST), &monitor);
    const int y = std::max(static_cast<int>(monitor.rcWork.top),
                          std::min(static_cast<int>(bounds.top), static_cast<int>(monitor.rcWork.bottom) - PickerHeight()));
    SetWindowPos(g_window, nullptr, bounds.left, y, kPickerWidth, PickerHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
    LayoutChildren(g_window);
}

void Recover(const std::wstring& message) {
    CancelPendingReturn();
    SetRecoveryMessage(message);
    // The picker was never hidden before submission. Do not steal focus back if
    // the user has already moved to another application during the attempt.
    const auto foreground = g_inputPlatform.Foreground();
    if (foreground == g_session.originalTarget || foreground == reinterpret_cast<WindowToken>(g_window)) {
        SetForegroundWindow(g_window);
        if (GetForegroundWindow() == g_window) SetFocus(g_edit);
    }
}

void CopySelection() {
    CancelPendingReturn();
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_displayVisible.size())) return;
    g_session.selected = g_displayVisible[selected].id;
    const std::wstring value = g_displayVisible[selected].payload;
    Win32ClipboardPlatform clipboard(g_window);
    const auto outcome = CopyText(clipboard, value);
    if (outcome.status == CopyStatus::Copied) {
        SetRecoveryMessage(L"");
        Remember(value.c_str());
        DismissPicker();
        return;
    }
    if (!outcome.clipboardClosed && !outcome.textPublished) {
        Recover(outcome.clipboardChanged
            ? L"Copy failed after clearing the clipboard.\nThe clipboard also could not be closed."
            : L"Copy failed; the clipboard was not replaced.\nThe clipboard also could not be closed.");
        return;
    }
    switch (outcome.status) {
    case CopyStatus::Busy: Recover(L"Clipboard is busy. Nothing was copied.\nTry Copy instead again."); break;
    case CopyStatus::PublishFailed: Recover(L"Copy failed after clearing the clipboard.\nYour selection is still available."); break;
    case CopyStatus::CloseFailed: Recover(L"Text was copied, but the clipboard did not close.\nYour selection is still available."); break;
    default: Recover(L"Could not copy. The clipboard was not replaced.\nYour selection is still available."); break;
    }
}

void InsertSelection(bool keepOpen = false) {
    CancelPendingReturn();
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_displayVisible.size())) return;
    g_session.selected = g_displayVisible[selected].id;
    const std::wstring value = g_displayVisible[selected].payload;
    const auto outcome = InsertText(g_inputPlatform, g_inputTarget, value);
    if (outcome.status != InsertionStatus::FullySubmitted) {
        switch (outcome.status) {
        case InsertionStatus::NoTarget: Recover(L"The original app is no longer available.\nCopy instead, then paste where you want."); break;
        case InsertionStatus::FocusFailed: Recover(L"Could not focus the original app.\nCopy instead, or reopen from the text field."); break;
        case InsertionStatus::ReleaseModifiers: Recover(L"Release Alt and Windows keys before inserting.\nYour selection is still available."); break;
        case InsertionStatus::PartiallySubmitted:
            Recover(outcome.cleanupComplete
                ? L"Input may be incomplete. Check the destination.\nRelease Ctrl/Shift before continuing."
                : L"Input may be incomplete; key cleanup also failed.\nCheck the destination and release modifier keys.");
            break;
        default: Recover(L"Could not submit the emoji to the original app.\nCopy instead, then paste where you want."); break;
        }
        return;
    }
    SetRecoveryMessage(L"");
    if (keepOpen && g_foregroundHook) {
        g_returnArmedAt = GetTickCount();
        g_focusReturn.Arm(g_session.insertionAttempt, g_inputTarget.window);
        // IDs are never reused in this process, including after cancellation.
        g_returnTimer = static_cast<UINT_PTR>(g_session.insertionAttempt) + kReturnTimerBase;
        if (!SetTimer(g_window, g_returnTimer, 75, nullptr)) {
            g_focusReturn.Cancel();
            g_returnTimer = 0;
        }
    }
    if (!keepOpen) DismissPicker();
    Remember(value.c_str());
}

void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND window, LONG, LONG, DWORD, DWORD eventTime) {
    if (!g_focusReturn.Pending() || static_cast<LONG>(eventTime - g_returnArmedAt) < 0) return;
    g_focusReturn.ForegroundChanged(reinterpret_cast<WindowToken>(window));
    if (!g_focusReturn.Pending()) CancelPendingReturn();
}

bool TryGetAutomationTextFieldAnchor(RECT& anchor) {
    if (!g_uiAutomation) return false;
    IUIAutomationElement* focused{};
    if (FAILED(g_uiAutomation->GetFocusedElement(&focused)) || !focused) return false;
    CONTROLTYPEID type{};
    RECT bounds{};
    const bool isTextField = SUCCEEDED(focused->get_CurrentControlType(&type)) &&
        (type == UIA_EditControlTypeId || type == UIA_DocumentControlTypeId ||
         type == UIA_ComboBoxControlTypeId);
    const bool hasBounds = SUCCEEDED(focused->get_CurrentBoundingRectangle(&bounds)) &&
        bounds.right > bounds.left && bounds.bottom > bounds.top;
    focused->Release();
    if (!isTextField || !hasBounds) return false;
    anchor = bounds;
    return true;
}

bool TryGetTextFieldAnchor(HWND active, RECT& anchor) {
    if (TryGetAutomationTextFieldAnchor(anchor)) return true;

    GUITHREADINFO info{sizeof(info)};
    const DWORD thread = GetWindowThreadProcessId(active, nullptr);
    if (!thread || !GetGUIThreadInfo(thread, &info)) return false;
    if (info.hwndCaret) {
        anchor = info.rcCaret;
        MapWindowPoints(info.hwndCaret, nullptr, reinterpret_cast<POINT*>(&anchor), 2);
        if (anchor.right <= anchor.left) anchor.right = anchor.left + 1;
        if (anchor.bottom <= anchor.top) anchor.bottom = anchor.top + 1;
        return true;
    }
    if (!info.hwndFocus || GetAncestor(info.hwndFocus, GA_ROOT) == info.hwndFocus) return false;
    return GetWindowRect(info.hwndFocus, &anchor);
}

int PickerHeight() {
    const int listHeight = g_emojiRows * kResultSize;
    return kPickerHeight + listHeight - kResultSize - (g_statusVisible ? 0 : kStatusHeight + 5)
        + (g_recoveryMessage.empty() ? 0 : kRecoveryHeight);
}

void CenterOnActiveMonitor() {
    CancelPendingReturn();
    HWND active = GetForegroundWindow();
    const auto target = CaptureExternalTarget(active);
    if (target.window) {
        g_inputTarget = target;
        g_session.originalTarget = target.window;
        SetRecoveryMessage(L"");
    }
    RECT anchor{};
    const bool hasAnchor = g_positionAboveTextField && TryGetTextFieldAnchor(active, anchor);
    HMONITOR monitor = hasAnchor
        ? MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST)
        : MonitorFromWindow(active, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    const RECT& area = info.rcWork;
    int x = area.left + ((area.right - area.left) - kPickerWidth) / 2;
    const int pickerHeight = PickerHeight();
    int y = area.top + ((area.bottom - area.top) - pickerHeight) / 2;
    if (hasAnchor) {
        x = anchor.left + (anchor.right - anchor.left) / 2 - kPickerWidth / 2;
        y = anchor.top - pickerHeight - 8;
        if (y < area.top) y = anchor.bottom + 8;
        const int minX = static_cast<int>(area.left);
        const int minY = static_cast<int>(area.top);
        const int maxX = std::max(minX, static_cast<int>(area.right) - kPickerWidth);
        const int maxY = std::max(minY, static_cast<int>(area.bottom) - pickerHeight);
        x = std::clamp(x, minX, maxX);
        y = std::clamp(y, minY, maxY);
    }
    SetWindowPos(g_window, HWND_TOPMOST, x, y, kPickerWidth, pickerHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetForegroundWindow(g_window);
    SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, -1);
}

void UpdateStatusLine() {
    if (!g_status || g_emojiFonts.empty()) return;
    if (!g_storageDiagnostic.empty()) {
        SetWindowTextW(g_status, g_storageDiagnostic.c_str());
        return;
    }
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    const std::wstring label = font.name + (font.color ? L" · Color" : L" · Monochrome");
    const std::wstring text = label + L"    Tab: font    Alt+I: skin tone    Alt+1-3: rows    F1: help";
    SetWindowTextW(g_status, text.c_str());
}

void ShowFontToast() {
    if (!g_status || g_emojiFonts.empty()) return;
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    const std::wstring text = std::wstring(L"Font: ") + font.name +
                              (font.color ? L" · Color" : L" · Monochrome");
    SetWindowTextW(g_status, text.c_str());
    KillTimer(g_window, kStatusTimerId);
    SetTimer(g_window, kStatusTimerId, 800, nullptr);
}

void ToggleStatusLine() {
    g_statusVisible = !g_statusVisible;
    KillTimer(g_window, kStatusTimerId);
    ShowWindow(g_status, g_statusVisible ? SW_SHOW : SW_HIDE);
    if (g_statusVisible) UpdateStatusLine();
    SetWindowPos(g_window, nullptr, 0, 0, kPickerWidth, PickerHeight(),
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void SetEmojiRows(int rows) {
    rows = std::clamp(rows, kMinEmojiRows, kMaxEmojiRows);
    if (rows == g_emojiRows) return;
    RECT bounds{};
    GetWindowRect(g_window, &bounds);
    g_emojiRows = rows;
    SaveSettings();
    RefreshList();
    const int height = PickerHeight();
    const HMONITOR monitor = MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    const int y = std::max(static_cast<int>(info.rcWork.top), static_cast<int>(bounds.bottom) - height);
    SetWindowPos(g_window, nullptr, bounds.left, y, kPickerWidth, height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void CycleSkinTone() {
    static const wchar_t* names[]{L"Default", L"Light", L"Medium-light", L"Medium",
                                  L"Medium-dark", L"Dark"};
    g_skinToneIndex = (g_skinToneIndex + 1) % 6;
    SaveSettings();
    RefreshList();
    if (g_statusVisible) {
        const std::wstring text = std::wstring(L"Skin tone: ") + names[g_skinToneIndex];
        SetWindowTextW(g_status, text.c_str());
        KillTimer(g_window, kStatusTimerId);
        SetTimer(g_window, kStatusTimerId, 800, nullptr);
    }
}

void SelectEmojiFont(size_t index) {
    if (g_emojiFonts.empty()) return;
    g_emojiFontIndex = index % g_emojiFonts.size();
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    if (g_emojiFont) DeleteObject(g_emojiFont);
    g_emojiFont = CreateFontW(-30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, font.name.c_str());
    if (g_emojiFormat) {
        g_emojiFormat->Release();
        g_emojiFormat = nullptr;
    }
    if (g_dwriteFactory) {
        g_dwriteFactory->CreateTextFormat(font.name.c_str(), nullptr,
                                          DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL, 30.0f, L"",
                                          &g_emojiFormat);
        if (g_emojiFormat) {
            g_emojiFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_emojiFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
    if (g_window) {
        const std::wstring title = std::wstring(L"SwashMoji — ") + font.name +
                                   (font.color ? L" · Color" : L" · Monochrome");
        SetWindowTextW(g_window, title.c_str());
    }
    if (g_list) InvalidateRect(g_list, nullptr, TRUE);
}

void CycleEmojiFont() {
    SelectEmojiFont(g_emojiFontIndex + 1);
    ShowFontToast();
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
    g_tray.hIcon = g_appIcon;
    lstrcpyW(g_tray.szTip, L"SwashMoji — Alt+E");
    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

void UpdateSortIndicator() {
    const wchar_t* cue = g_sortByUsage
                             ? L"Search — most used first (Alt+T)"
                             : L"Search — most recent first (Alt+T)";
    SendMessageW(g_edit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(cue));
    std::wstring tip = g_sortByUsage ? L"SwashMoji — most used — Alt+E" : L"SwashMoji — most recent — Alt+E";
    if (g_profileUnsaved) tip += L" — Changes not saved";
    else if (!g_storageDiagnostic.empty()) tip += L" — " + g_storageDiagnostic;
    lstrcpynW(g_tray.szTip, tip.c_str(), static_cast<int>(std::size(g_tray.szTip)));
    Shell_NotifyIconW(NIM_MODIFY, &g_tray);
}

void SetSortMode(bool sortByUsage) {
    g_sortByUsage = sortByUsage;
    SaveSettings();
    UpdateSortIndicator();
    RefreshList();
}

void ToggleSortMode() {
    SetSortMode(!g_sortByUsage);
}

void ConfirmAndClearUsageHistory() {
    const int result = MessageBoxW(
        g_window,
        L"Forget all recently used emoji and reset how often each emoji was chosen?\n\nThis cannot be undone.",
        L"Clear remembered emoji",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (result != IDYES) return;

    ClearHistory(g_profile);
    SaveProfile();
    RefreshList();
}

void DrawHelpText(HDC dc, const wchar_t* text, RECT area, HFONT font, COLORREF color,
                  UINT format = DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX) {
    const HFONT previous = static_cast<HFONT>(SelectObject(dc, font));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &area, format);
    SelectObject(dc, previous);
}

void DrawHelpRow(HDC dc, int x, int y, int width, int height,
                 const wchar_t* key, const wchar_t* description) {
    constexpr int keyWidth = 94;
    RECT keyArea{x, y, x + keyWidth, y + 28};
    HBRUSH keyBrush = CreateSolidBrush(RGB(42, 42, 42));
    HPEN keyPen = CreatePen(PS_SOLID, 1, RGB(67, 67, 67));
    const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dc, keyBrush));
    const HPEN oldPen = static_cast<HPEN>(SelectObject(dc, keyPen));
    RoundRect(dc, keyArea.left, keyArea.top, keyArea.right, keyArea.bottom, 7, 7);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(keyPen);
    DeleteObject(keyBrush);

    DrawHelpText(dc, key, keyArea, g_helpHeadingFont, kText,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    RECT descriptionArea{x + keyWidth + 14, y + 3, x + width, y + height};
    DrawHelpText(dc, description, descriptionArea, g_helpBodyFont, kText);
}

LRESULT CALLBACK HelpWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        BOOL dark = TRUE;
        DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
        SetWindowTheme(window, L"DarkMode_Explorer", nullptr);
        SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIcon));
        SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIcon));
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        FillRect(dc, &client, g_backgroundBrush);

        DrawHelpText(dc, L"SwashMoji", RECT{28, 22, 350, 58}, g_helpTitleFont, kText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        DrawHelpText(dc, L"Keyboard-first emoji picker", RECT{29, 58, 350, 81},
                     g_helpBodyFont, kMutedText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        HBRUSH accentBrush = CreateSolidBrush(RGB(75, 151, 222));
        RECT accent{28, 91, client.right - 28, 93};
        FillRect(dc, &accent, accentBrush);
        DeleteObject(accentBrush);

        constexpr COLORREF heading = RGB(112, 184, 246);
        DrawHelpText(dc, L"ESSENTIALS", RECT{28, 112, 330, 136},
                     g_helpHeadingFont, heading);
        int leftY = 143;
        DrawHelpRow(dc, 28, leftY, 315, 42, L"Alt+E", L"Open from any app.");
        leftY += 44;
        DrawHelpRow(dc, 28, leftY, 315, 70, L"Type", L"Search in English or Norwegian: names, intent phrases, and your own aliases. Close spelling is a fallback.");
        leftY += 72;
        DrawHelpRow(dc, 28, leftY, 315, 46, L"Arrow keys", L"Move through matching emoji.");
        leftY += 48;
        DrawHelpRow(dc, 28, leftY, 315, 46, L"Click", L"Insert and return to SwashMoji.");
        leftY += 48;
        DrawHelpRow(dc, 28, leftY, 315, 46, L"Enter", L"Insert into the active text field.");
        leftY += 48;
        DrawHelpRow(dc, 28, leftY, 315, 54, L"Shift+Enter", L"Copy; stay open if copying fails.");
        leftY += 56;
        DrawHelpRow(dc, 28, leftY, 315, 54, L"Ctrl+Enter", L"Insert and keep SwashMoji open.");
        leftY += 56;
        DrawHelpRow(dc, 28, leftY, 315, 42, L"Esc", L"Close the picker.");
        leftY += 44;
        DrawHelpRow(dc, 28, leftY, 315, 72, L"Alt+C", L"After an insertion error: Copy instead. Check the destination if input was partial.");
        leftY += 74;
        DrawHelpRow(dc, 28, leftY, 315, 66, L"Alt+A", L"Add an alias for your selection, or teach a phrase with no matches.");

        constexpr int rightX = 374;
        DrawHelpText(dc, L"CUSTOMIZE", RECT{rightX, 112, 690, 136},
                     g_helpHeadingFont, heading);
        int rightY = 143;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Tab", L"Cycle color and monochrome fonts.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+T", L"Toggle recent / most-used sorting.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+S", L"Hide or show the status line.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+I", L"Cycle skin tone for all compatible emoji.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+1 / 2 / 3", L"Show one, two, or three emoji rows.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 42, L"F1", L"Open this guide.");
        rightY += 57;

        DrawHelpText(dc, L"RECENTS & TRAY", RECT{rightX, rightY, 690, rightY + 24},
                     g_helpHeadingFont, heading);
        rightY += 31;
        DrawHelpText(dc,
                     L"SwashMoji remembers which emoji you choose. Recent mode puts your latest choices first; most-used mode puts your frequent choices first. This stays on your PC.",
                     RECT{rightX, rightY, 692, rightY + 90}, g_helpBodyFont, kText);
        rightY += 96;
        DrawHelpText(dc,
                     L"Right-click a result to add an alias. In the tray menu, My vocabulary edits or removes your saved aliases. Clearing history keeps them.",
                     RECT{rightX, rightY, 692, rightY + 90}, g_helpBodyFont, kText);

        DrawHelpText(dc, L"All usage data is stored locally in %LOCALAPPDATA%\\SwashMoji",
                     RECT{28, client.bottom - 37, client.right - 28, client.bottom - 17},
                     g_statusFont, kMutedText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        g_helpWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowHelp() {
    CancelPendingReturn();
    if (g_helpWindow) {
        ShowWindow(g_helpWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_helpWindow);
        return;
    }

    RECT windowRect{0, 0, kHelpWidth, kHelpHeight};
    AdjustWindowRectEx(&windowRect, WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    HMONITOR monitor = MonitorFromWindow(g_window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(monitor, &monitorInfo);
    const RECT& area = monitorInfo.rcWork;
    const int x = area.left + ((area.right - area.left) - width) / 2;
    const int y = area.top + ((area.bottom - area.top) - height) / 2;

    g_helpWindow = CreateWindowExW(
        WS_EX_TOOLWINDOW, kHelpClassName, L"SwashMoji Help", WS_CAPTION | WS_SYSMENU,
        x, y, width, height, g_window, nullptr,
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_window, GWLP_HINSTANCE)), nullptr);
    if (!g_helpWindow) return;
    ShowWindow(g_helpWindow, SW_SHOW);
    UpdateWindow(g_helpWindow);
    SetForegroundWindow(g_helpWindow);
}

void ShowTrayMenu() {
    CancelPendingReturn();
    const auto target = CaptureExternalTarget(GetForegroundWindow());
    if (target.window) { g_inputTarget = target; g_session.originalTarget = target.window; }
    HMENU menu = CreatePopupMenu();
    const UINT positionFlags = MF_STRING | (g_positionAboveTextField ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(menu, positionFlags, kPositionAboveTextFieldId, L"Try positioning above active text field");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kSortRecentId, L"Sort: Most recent");
    AppendMenuW(menu, MF_STRING, kSortMostUsedId, L"Sort: Most used");
    CheckMenuRadioItem(menu, kSortRecentId, kSortMostUsedId,
                       g_sortByUsage ? kSortMostUsedId : kSortRecentId, MF_BYCOMMAND);
    AppendMenuW(menu, MF_STRING, kClearUsageHistoryId, L"Clear remembered emoji...");
    AppendMenuW(menu, MF_STRING, kVocabularyId, L"My vocabulary...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kExitId, L"Exit");
    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(g_window);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        point.x, point.y, 0, g_window, nullptr);
    DestroyMenu(menu);
    // Lets the taskbar dismiss the menu correctly after a tray interaction.
    PostMessageW(g_window, WM_NULL, 0, 0);
    if (command == kPositionAboveTextFieldId) {
        g_positionAboveTextField = !g_positionAboveTextField;
        SaveSettings();
    } else if (command == kSortRecentId) {
        SetSortMode(false);
    } else if (command == kSortMostUsedId) {
        SetSortMode(true);
    } else if (command == kClearUsageHistoryId) {
        ConfirmAndClearUsageHistory();
    } else if (command == kVocabularyId) {
        OpenVocabulary(false);
    } else if (command == kExitId) {
        DestroyWindow(g_window);
    }
}

void LayoutChildren(HWND window) {
    RECT area{};
    GetClientRect(window, &area);
    constexpr int margin = 12;
    MoveWindow(g_edit, margin, margin, area.right - margin * 2, kInputHeight, TRUE);
    const int listY = margin + kInputHeight + 8;
    const int listHeight = g_emojiRows * kResultSize;
    MoveWindow(g_list, margin, listY, area.right - margin * 2, listHeight, TRUE);
    MoveWindow(g_teachPhrase, margin + 24, listY + 5, area.right - margin * 2 - 48, 36, TRUE);
    MoveWindow(g_status, margin, listY + listHeight + 5, area.right - margin * 2,
               kStatusHeight, TRUE);
    const int recoveryY = listY + listHeight + 8 + (g_statusVisible ? kStatusHeight + 5 : 0);
    MoveWindow(g_recoveryLabel, margin, recoveryY, area.right - margin * 2 - 132, kRecoveryHeight - 12, TRUE);
    MoveWindow(g_copyInstead, area.right - margin - 126, recoveryY + 8, 126, 30, TRUE);
}

void MoveSelection(int direction) {
    if (g_displayVisible.empty()) return;
    int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected == LB_ERR) selected = 0;
    selected = std::clamp(selected + direction, 0, static_cast<int>(g_displayVisible.size()) - 1);
    SendMessageW(g_list, LB_SETCURSEL, selected, 0);
    g_session.selected = g_displayVisible[selected].id;
}

LRESULT CALLBACK InputProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam) {
    const WNDPROC original = control == g_edit ? g_editProc : g_listProc;
    if (control == g_list && message == WM_CONTEXTMENU) {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (point.x == -1 && point.y == -1) {
            RECT area{};
            GetWindowRect(control, &area);
            point = {area.left + 12, area.bottom};
        } else {
            POINT local = point;
            ScreenToClient(control, &local);
            const auto item = SendMessageW(control, LB_ITEMFROMPOINT, 0, MAKELPARAM(local.x, local.y));
            if (HIWORD(item)) return 0;
            SendMessageW(control, LB_SETCURSEL, LOWORD(item), 0);
        }
        if (g_displayVisible.empty()) return 0;
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, kVocabularyId, L"Add alias...\tAlt+A");
        const auto command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
            point.x, point.y, 0, g_window, nullptr);
        DestroyMenu(menu);
        if (command == kVocabularyId) OpenVocabulary(true);
        return 0;
    }
    if (control == g_list && message == WM_LBUTTONUP) {
        const LRESULT result = CallWindowProcW(original, control, message, wParam, lParam);
        InsertSelection(true);
        return result;
    }
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        if (wParam == 'I' && (GetKeyState(VK_MENU) & 0x8000)) {
            CycleSkinTone();
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            DismissPicker();
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (!g_recoveryMessage.empty() && (lParam & (1u << 30))) return 0;
            if (GetKeyState(VK_SHIFT) & 0x8000) CopySelection();
            else if (GetKeyState(VK_CONTROL) & 0x8000) InsertSelection(true);
            else InsertSelection();
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
        if (wParam == VK_LEFT) {
            MoveSelection(-g_emojiRows);
            return 0;
        }
        if (wParam == VK_RIGHT) {
            MoveSelection(g_emojiRows);
            return 0;
        }
    }
    return CallWindowProcW(original, control, message, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        g_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)), nullptr, nullptr);
        g_list = CreateWindowExW(0, L"LISTBOX", L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                 LBS_OWNERDRAWFIXED | LBS_MULTICOLUMN | LBS_HASSTRINGS,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);
        g_status = CreateWindowExW(0, L"STATIC", L"",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                   0, 0, 0, 0, window,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)), nullptr, nullptr);
        g_recoveryLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | SS_LEFT,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kRecoveryId)), nullptr, nullptr);
        g_copyInstead = CreateWindowExW(0, L"BUTTON", L"&Copy instead", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kCopyInsteadId)), nullptr, nullptr);
        g_teachPhrase = CreateWindowExW(0, L"BUTTON", L"No matches. Teach this phrase (Alt+A)", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kTeachPhraseId)), nullptr, nullptr);
        SendMessageW(g_teachPhrase, WM_SETFONT, reinterpret_cast<WPARAM>(g_statusFont), TRUE);
        SendMessageW(g_recoveryLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_statusFont), TRUE);
        SendMessageW(g_copyInstead, WM_SETFONT, reinterpret_cast<WPARAM>(g_statusFont), TRUE);
        SendMessageW(g_edit, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        SendMessageW(g_edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
        SendMessageW(g_list, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_statusFont), TRUE);
        SendMessageW(g_list, LB_SETCOLUMNWIDTH, kResultSize, 0);
        SetWindowTheme(g_edit, L"DarkMode_Explorer", nullptr);
        SetWindowTheme(g_list, L"DarkMode_Explorer", nullptr);
        g_editProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_edit, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        g_listProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_list, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        LayoutChildren(window);
        RefreshList();
        AddTrayIcon(window);
        UpdateSortIndicator();
        UpdateStatusLine();
        return 0;
    }
    case WM_SIZE: LayoutChildren(window); return 0;
    case WM_MEASUREITEM:
        if (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlID == kListId) {
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = kResultSize;
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemWidth = kResultSize;
            return TRUE;
        }
        break;
    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID != kListId || item->itemID >= g_displayVisible.size()) break;
        const bool selected = (item->itemState & ODS_SELECTED) != 0;
        HBRUSH fill = CreateSolidBrush(selected ? kSelected : kBackground);
        FillRect(item->hDC, &item->rcItem, fill);
        DeleteObject(fill);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, kText);
        const HFONT previousFont = static_cast<HFONT>(SelectObject(item->hDC, g_uiFont));
        const auto& result = g_displayVisible[item->itemID];
        RECT glyph = item->rcItem;
        InflateRect(&glyph, -3, -3);
        DrawColorEmoji(item->hDC, glyph, result.payload);
        if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
        SelectObject(item->hDC, previousFont);
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
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wParam), kMutedText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kBackground);
        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    case WM_COMMAND:
        if (LOWORD(wParam) == kTeachPhraseId && HIWORD(wParam) == BN_CLICKED) OpenVocabulary(true);
        if (LOWORD(wParam) == kCopyInsteadId && HIWORD(wParam) == BN_CLICKED) CopySelection();
        if (LOWORD(wParam) == kEditId && HIWORD(wParam) == EN_CHANGE) RefreshList();
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam == 'I' && (GetKeyState(VK_MENU) & 0x8000)) {
            CycleSkinTone();
            return 0;
        }
        if (wParam == VK_ESCAPE) { DismissPicker(); return 0; }
        if (wParam == VK_RETURN) {
            if (!g_recoveryMessage.empty() && (lParam & (1u << 30))) return 0;
            if (GetKeyState(VK_SHIFT) & 0x8000) CopySelection();
            else if (GetKeyState(VK_CONTROL) & 0x8000) InsertSelection(true);
            else InsertSelection();
            return 0;
        }
        break;
    case WM_HOTKEY:
        if (g_vocabularyOpen) return 0;
        if (wParam == kHotkeyId) {
            SetWindowTextW(g_edit, L"");
            CenterOnActiveMonitor();
        }
        return 0;
    case WM_TIMER:
        if (wParam == kStatusTimerId) {
            KillTimer(window, kStatusTimerId);
            UpdateStatusLine();
            return 0;
        }
        if (g_returnTimer && wParam == g_returnTimer) {
            KillTimer(window, g_returnTimer);
            g_returnTimer = 0;
            if (g_focusReturn.Take(g_session.insertionAttempt, g_inputPlatform.Foreground(),
                                  IsWindowVisible(window) != FALSE, g_inputPlatform.ValidTarget(g_inputTarget))) {
                SetForegroundWindow(g_window);
                if (GetForegroundWindow() == g_window) SetFocus(g_edit);
            }
            return 0;
        }
        break;
    case kShowPickerMessage:
        if (g_vocabularyOpen) return 0;
        SetWindowTextW(g_edit, L"");
        CenterOnActiveMonitor();
        return 0;
    case kTrayMessage:
        if (g_vocabularyOpen) return 0;
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
        if ((wParam & 0xFFF0) == SC_CLOSE) { DismissPicker(); return 0; }
        break;
    case WM_DESTROY:
        CancelPendingReturn();
        if (g_foregroundHook) { UnhookWinEvent(g_foregroundHook); g_foregroundHook = nullptr; }
        UnregisterHotKey(window, kHotkeyId);
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    // A repeat launch should surface the existing picker, not create a second
    // process that cannot own the global Alt+E shortcut.
    if (HWND existing = FindWindowW(kClassName, nullptr)) {
        PostMessageW(existing, kShowPickerMessage, 0, 0);
        return 0;
    }
    if (!LoadEmojis()) {
        MessageBoxW(nullptr, L"Could not read emojis.txt or intent_phrases.tsv beside SwashMoji.exe.", L"SwashMoji", MB_ICONERROR);
        return 1;
    }
    LoadProfile();
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    g_comInitialized = SUCCEEDED(comResult);
    CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(&g_uiAutomation));
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.lpfnWndProc = WindowProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    g_appIcon = LoadIconW(instance, MAKEINTRESOURCEW(kAppIconId));
    wc.hIcon = g_appIcon;
    g_uiFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                           L"Segoe UI Variable Text");
    g_statusFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                               L"Segoe UI Variable Text");
    g_helpTitleFont = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                  L"Segoe UI Variable Display");
    g_helpHeadingFont = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                    L"Segoe UI Variable Text");
    g_helpBodyFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                 L"Segoe UI Variable Text");
    g_backgroundBrush = CreateSolidBrush(kBackground);
    g_inputBrush = CreateSolidBrush(kInputBackground);
    InitializeColorEmojiDrawing();
    LoadInstalledColorEmojiFonts();
    // Only the glyph column uses this font; labels retain the regular UI font.
    SelectEmojiFont(0);
    wc.hbrBackground = g_backgroundBrush;
    if (!RegisterClassW(&wc)) return 1;
    WNDCLASSW helpClass{};
    helpClass.hInstance = instance;
    helpClass.lpszClassName = kHelpClassName;
    helpClass.lpfnWndProc = HelpWindowProc;
    helpClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    helpClass.hIcon = g_appIcon;
    helpClass.hbrBackground = g_backgroundBrush;
    if (!RegisterClassW(&helpClass)) return 1;

    g_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kClassName, L"SwashMoji",
                               WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                               kPickerWidth, PickerHeight(), nullptr, nullptr, instance, nullptr);
    if (!g_window) return 1;
    g_foregroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        nullptr, ForegroundChanged, 0, 0, WINEVENT_OUTOFCONTEXT);
    SendMessageW(g_window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIcon));
    SendMessageW(g_window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIcon));
    if (!RegisterHotKey(g_window, kHotkeyId, MOD_ALT | MOD_NOREPEAT, 'E')) {
        MessageBoxW(nullptr, L"Alt+E er allerede i bruk av et annet program.", L"SwashMoji", MB_ICONWARNING);
    }

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        // Handle app-local Alt shortcuts before dispatch. Depending on focus and
        // popup state, Windows can address system-key messages to either the
        // child control or the top-level picker.
        const bool altPressed = (GetKeyState(VK_MENU) & 0x8000) ||
                                (message.message == WM_SYSKEYDOWN &&
                                 (message.lParam & (1u << 29)));
        const bool firstKeyPress = !(message.lParam & (1u << 30));
        const bool pickerKey = (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            (message.hwnd == g_window || IsChild(g_window, message.hwnd));
        if (pickerKey && altPressed && firstKeyPress && message.wParam == 'A') {
            OpenVocabulary(true);
            continue;
        }
        if (pickerKey && message.hwnd == g_teachPhrase) {
            if (message.wParam == VK_RETURN) { OpenVocabulary(true); continue; }
            if (message.wParam == VK_ESCAPE) { DismissPicker(); continue; }
        }
        if (pickerKey && message.hwnd == g_copyInstead && message.wParam == VK_ESCAPE) {
            DismissPicker();
            continue;
        }
        if (pickerKey && !g_recoveryMessage.empty() && firstKeyPress &&
            ((altPressed && message.wParam == 'C') ||
             (message.hwnd == g_copyInstead && message.wParam == VK_RETURN))) {
            CopySelection();
            continue;
        }
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            firstKeyPress && message.wParam == VK_F1) {
            ShowHelp();
            continue;
        }
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            firstKeyPress && message.wParam == VK_TAB) {
            CycleEmojiFont();
            continue;
        }
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            altPressed && firstKeyPress) {
            if (message.wParam == 'T') {
                ToggleSortMode();
                continue;
            }
            if (message.wParam == 'S') {
                ToggleStatusLine();
                continue;
            }
            if (message.wParam == 'I') {
                CycleSkinTone();
                continue;
            }
            if (message.wParam >= '1' && message.wParam <= '3') {
                SetEmojiRows(static_cast<int>(message.wParam - '0'));
                continue;
            }
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (g_uiAutomation) g_uiAutomation->Release();
    if (g_emojiFormat) g_emojiFormat->Release();
    if (g_dwriteFactory) g_dwriteFactory->Release();
    if (g_d2dTarget) g_d2dTarget->Release();
    if (g_d2dFactory) g_d2dFactory->Release();
    DeleteObject(g_emojiFont);
    DeleteObject(g_helpBodyFont);
    DeleteObject(g_helpHeadingFont);
    DeleteObject(g_helpTitleFont);
    DeleteObject(g_statusFont);
    DeleteObject(g_uiFont);
    DeleteObject(g_inputBrush);
    DeleteObject(g_backgroundBrush);
    if (g_comInitialized) CoUninitialize();
    return static_cast<int>(message.wParam);
}
