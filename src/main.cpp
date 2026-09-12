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
#include "edit_controls.h"
#include "native_emoji.h"
#include "native_theme.h"
#include "language_preferences.h"
#include "picker.h"
#include <oleacc.h>
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
constexpr int kPickerHeight = 132;
constexpr int kInputHeight = 34;
constexpr int kResultSize = 48;
constexpr int kEmojiColumns = 10;
constexpr int kMinEmojiRows = 1;
constexpr int kMaxEmojiRows = 3;
constexpr int kHelpWidth = 720;
constexpr int kHelpHeight = 840;
constexpr int kEditId = 100;
constexpr int kListId = 101;
constexpr int kStatusId = 102;
constexpr int kRecoveryId = 103;
constexpr int kCopyInsteadId = 104;
constexpr int kTeachPhraseId = 105;
constexpr int kDetailsId = 106;
constexpr UINT_PTR kHoverTimerId = 2;
constexpr int kVocabularyId = 205;
constexpr int kPinId = 206;
constexpr int kLearnQueriesId = 207;
constexpr int kDisplayLanguagesId = 208;
constexpr int kRecoveryHeight = 68;
constexpr int kExitId = 200;
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
constexpr COLORREF kSelected = RGB(42, 86, 128);

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
HWND g_hoverWindow{};
bool g_detailsOpen{};
int g_hoverIndex{-1}, g_pressedIndex{-1};
POINT g_hoverPoint{};
UINT g_dpi{96};
ResultId g_variantTarget;
std::wstring g_variantPayload;
int Px(int value) { return MulDiv(value, g_dpi, 96); }
bool HighContrast() {
    HIGHCONTRASTW value{sizeof(value)};
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(value), &value, 0) && (value.dwFlags & HCF_HIGHCONTRASTON);
}
COLORREF Background() { return HighContrast() ? GetSysColor(COLOR_WINDOW) : kBackground; }
COLORREF Foreground() { return HighContrast() ? GetSysColor(COLOR_WINDOWTEXT) : kText; }
void CloseHover();
void OpenDetails();
void UpdateDpi(UINT dpi);
void ClampWindow(HWND window);
void SelectionChanged();

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
RankingPreferences g_rankingPreferences;
ProfileStorage g_storage;
bool g_profileUnsaved{};
std::wstring g_storageDiagnostic;
bool& g_sortByUsage = g_profile.settings.sortByUsage;
bool g_statusVisible{true};
bool g_pickerAboveAnchor{};
int& g_emojiRows = g_profile.settings.emojiRows;
int& g_skinToneIndex = g_profile.settings.skinTone;
WNDPROC g_editProc{};
WNDPROC g_listProc{};
HBRUSH g_backgroundBrush{};
HBRUSH g_inputBrush{};
HICON g_appIcon{};
HFONT g_uiFont{};
HFONT g_statusFont{};
HFONT g_resultLabelFont{};
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
void SetRecoveryMessage(const std::wstring& message);
void ResizePicker(int height);

void CaptureInputTarget(HWND active) {
    const auto target = CaptureExternalTarget(active);
    g_inputTarget = target;
    g_session.originalTarget = target.window;
    if (target.window) SetRecoveryMessage(L"");
}

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
    auto loaded = g_storage.Load(&g_catalog);
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

void RememberSelection(const ResultId& target) {
    RecordChoice(g_profile, target, g_session.query);
    SaveProfile();
}

void RefreshList() {
    wchar_t input[256]{};
    GetWindowTextW(g_edit, input, static_cast<int>(std::size(input)));
    const auto oldIndex = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
    const auto previous = oldIndex >= 0 && static_cast<size_t>(oldIndex) < g_displayVisible.size()
        ? g_displayVisible[oldIndex].id : g_session.selected;
    CloseHover();
    const bool preserveSelection = g_session.query == input;
    if (!preserveSelection) { g_variantTarget = {}; g_variantPayload.clear(); }
    g_session.query = input;
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    g_visible = Search(g_catalog, g_profile, g_session.query, &g_rankingPreferences);
    // Fit the current results without changing the user's preferred maximum.
    // Resize before filling the multicolumn list so its native row count agrees
    // with the item mapping, hit tests and keyboard navigation.
    const auto window = GetParent(g_list);
    RECT bounds{};
    GetWindowRect(window, &bounds);
    if (bounds.bottom - bounds.top != PickerHeight()) {
        ResizePicker(PickerHeight());
    }
    LayoutChildren(window);
    ShowWindow(g_teachPhrase, g_visible.empty() && !NormalizePhrase(g_session.query).empty() ? SW_SHOW : SW_HIDE);
    g_displayVisible.clear();
    g_session.rankingSnapshot.clear();
    for (const auto& result : g_visible) g_session.rankingSnapshot.push_back(result.id);
    g_session.selected = g_visible.empty() ? ResultId{} : g_visible.front().id;
    for (const auto index : GridOrder(g_visible.size(), GridRows(g_visible.size(), g_emojiRows), kEmojiColumns))
        g_displayVisible.push_back(g_visible[index]);
    for (const auto& result : g_displayVisible) {
        const auto* exact = g_catalog.Find(result.id == g_variantTarget && !g_variantPayload.empty() ? g_variantPayload : result.payload);
        const auto label = exact ? FormatEmojiDisplayName(*exact, g_profile.settings.displayLanguages) : result.label;
        SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    if (!g_displayVisible.empty()) {
        size_t selection = 0;
        if (preserveSelection) for (size_t i = 0; i < g_displayVisible.size(); ++i)
            if (g_displayVisible[i].id == previous) { selection = i; break; }
        SendMessageW(g_list, LB_SETCURSEL, selection, 0);
        g_session.selected = g_displayVisible[selection].id;
    }
    SelectionChanged();
}

void ToggleSelectedPin() {
    const auto index = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
    if (index < 0 || static_cast<size_t>(index) >= g_displayVisible.size()) return;
    const auto id = g_displayVisible[index].id;
    if (IsPinned(g_profile, id)) Unpin(g_profile, id);
    else {
        const auto result = Pin(g_profile, g_catalog, id);
        if (result == PinResult::LimitReached) {
            MessageBoxW(g_window, L"You can pin up to ten favorites. Unpin one in My vocabulary to make room.", L"Favorites", MB_ICONINFORMATION);
            return;
        }
        if (result != PinResult::Pinned) return;
    }
    SaveProfile();
    RefreshList();
}

void BeginPickerSession() {
    g_variantTarget = {}; g_variantPayload.clear();
    g_rankingPreferences = g_profile;
    RefreshList();
}

void CancelPendingReturn() {
    ++g_session.insertionAttempt;
    g_focusReturn.Cancel();
    if (g_returnTimer) KillTimer(g_window, g_returnTimer);
    g_returnTimer = 0;
}

void OpenVocabulary(bool prefill) {
    if (g_vocabularyOpen || g_detailsOpen) return;
    CloseHover();
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
    CloseHover();
    g_variantTarget = {}; g_variantPayload.clear();
    CancelPendingReturn();
    ShowWindow(g_window, SW_HIDE);
}

void SetRecoveryMessage(const std::wstring& message) {
    g_recoveryMessage = message;
    const bool visible = !message.empty();
    SetWindowTextW(g_recoveryLabel, message.c_str());
    if (visible) NotifyWinEvent(EVENT_SYSTEM_ALERT, g_recoveryLabel, OBJID_CLIENT, CHILDID_SELF);
    ShowWindow(g_recoveryLabel, visible ? SW_SHOW : SW_HIDE);
    ShowWindow(g_copyInstead, visible ? SW_SHOW : SW_HIDE);
    RECT bounds{};
    GetWindowRect(g_window, &bounds);
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfoW(MonitorFromWindow(g_window, MONITOR_DEFAULTTONEAREST), &monitor);
    const int y = std::max(static_cast<int>(monitor.rcWork.top),
                          std::min(static_cast<int>(bounds.top), static_cast<int>(monitor.rcWork.bottom) - PickerHeight()));
    SetWindowPos(g_window, nullptr, bounds.left, y, Px(kPickerWidth), PickerHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
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

void CopySelection(ClipboardPlatform* platformOverride = nullptr) {
    CancelPendingReturn();
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_displayVisible.size())) return;
    g_session.selected = g_displayVisible[selected].id;
    const std::wstring value = g_displayVisible[selected].id == g_variantTarget && !g_variantPayload.empty()
        ? g_variantPayload : g_displayVisible[selected].payload;
    Win32ClipboardPlatform clipboard(g_window);
    const auto outcome = CopyText(platformOverride ? *platformOverride : clipboard, value);
    if (outcome.status == CopyStatus::Copied) {
        SetRecoveryMessage(L"");
        RememberSelection(g_session.selected);
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

void InsertSelection(bool keepOpen = false, InputPlatform* platformOverride = nullptr) {
    CloseHover();
    CancelPendingReturn();
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_displayVisible.size())) return;
    g_session.selected = g_displayVisible[selected].id;
    const std::wstring value = g_displayVisible[selected].id == g_variantTarget && !g_variantPayload.empty()
        ? g_variantPayload : g_displayVisible[selected].payload;
    const auto outcome = InsertText(platformOverride ? *platformOverride : g_inputPlatform, g_inputTarget, value);
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
    g_variantTarget = {}; g_variantPayload.clear();
    RefreshList();
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
    RememberSelection(g_session.selected);
}

void CALLBACK ForegroundChanged(HWINEVENTHOOK, DWORD, HWND window, LONG, LONG, DWORD, DWORD eventTime) {
    if (!g_focusReturn.Pending() || static_cast<LONG>(eventTime - g_returnArmedAt) < 0) return;
    g_focusReturn.ForegroundChanged(reinterpret_cast<WindowToken>(window));
    if (!g_focusReturn.Pending()) CancelPendingReturn();
}

bool TextRangeBounds(IUIAutomationTextRange* range, RECT& anchor, bool useRightEdge) {
    SAFEARRAY* rectangles{};
    if (!range || FAILED(range->GetBoundingRectangles(&rectangles)) || !rectangles) return false;
    LONG lower{}, upper{};
    double* values{};
    const bool valid = SUCCEEDED(SafeArrayGetLBound(rectangles, 1, &lower)) &&
        SUCCEEDED(SafeArrayGetUBound(rectangles, 1, &upper)) && upper - lower + 1 >= 4 &&
        SUCCEEDED(SafeArrayAccessData(rectangles, reinterpret_cast<void**>(&values)));
    if (!valid) { SafeArrayDestroy(rectangles); return false; }
    const LONG offset = useRightEdge ? upper - lower - 3 : 0;
    const double x = values[offset] + (useRightEdge ? values[offset + 2] : 0.0);
    const double y = values[offset + 1];
    const double height = values[offset + 3];
    anchor.left = static_cast<LONG>(x);
    anchor.right = anchor.left + 1;
    anchor.top = static_cast<LONG>(y);
    anchor.bottom = std::max(anchor.top + 1, static_cast<LONG>(y + height));
    SafeArrayUnaccessData(rectangles);
    SafeArrayDestroy(rectangles);
    return true;
}

bool TextRangeCaretBounds(IUIAutomationTextRange* range, RECT& anchor) {
    if (TextRangeBounds(range, anchor, true)) return true;
    int moved{};
    if (SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_End, TextUnit_Character, 1, &moved)) && moved &&
        TextRangeBounds(range, anchor, false)) return true;
    if (SUCCEEDED(range->MoveEndpointByUnit(TextPatternRangeEndpoint_Start, TextUnit_Character, -1, &moved)) && moved)
        return TextRangeBounds(range, anchor, true);
    return false;
}

bool TryGetAutomationCaretAnchor(RECT& anchor) {
    if (!g_uiAutomation) return false;
    IUIAutomationElement* focused{};
    if (FAILED(g_uiAutomation->GetFocusedElement(&focused)) || !focused) return false;
    IUIAutomationTextPattern2* pattern{};
    const HRESULT patternResult = focused->GetCurrentPatternAs(UIA_TextPattern2Id, IID_PPV_ARGS(&pattern));
    if (SUCCEEDED(patternResult) && pattern) {
        BOOL active{};
        IUIAutomationTextRange* range{};
        const HRESULT caretResult = pattern->GetCaretRange(&active, &range);
        pattern->Release();
        if (SUCCEEDED(caretResult) && active && range) {
            const bool found = TextRangeCaretBounds(range, anchor);
            range->Release();
            if (found) { focused->Release(); return true; }
        } else if (range) {
            range->Release();
        }
    }

    IUIAutomationTextPattern* textPattern{};
    const HRESULT textResult = focused->GetCurrentPatternAs(UIA_TextPatternId, IID_PPV_ARGS(&textPattern));
    focused->Release();
    if (FAILED(textResult) || !textPattern) return false;
    IUIAutomationTextRangeArray* selections{};
    const HRESULT selectionResult = textPattern->GetSelection(&selections);
    textPattern->Release();
    if (FAILED(selectionResult) || !selections) return false;
    int length{};
    selections->get_Length(&length);
    IUIAutomationTextRange* range{};
    if (length > 0) selections->GetElement(length - 1, &range);
    selections->Release();
    if (!range) return false;
    const bool found = TextRangeCaretBounds(range, anchor);
    range->Release();
    return found;
}

bool TryGetTextFieldAnchor(HWND active, RECT& anchor) {
    GUITHREADINFO info{sizeof(info)};
    const DWORD thread = GetWindowThreadProcessId(active, nullptr);
    const bool hasThreadInfo = thread && GetGUIThreadInfo(thread, &info);
    if (hasThreadInfo && info.hwndCaret) {
        anchor = info.rcCaret;
        MapWindowPoints(info.hwndCaret, nullptr, reinterpret_cast<POINT*>(&anchor), 2);
        if (anchor.right <= anchor.left) anchor.right = anchor.left + 1;
        if (anchor.bottom <= anchor.top) anchor.bottom = anchor.top + 1;
        return true;
    }
    if (TryGetAutomationCaretAnchor(anchor)) return true;
    if (!hasThreadInfo || !info.hwndFocus || GetAncestor(info.hwndFocus, GA_ROOT) == info.hwndFocus) return false;
    return GetWindowRect(info.hwndFocus, &anchor);
}

int PickerHeight() {
    const int listHeight = GridRows(g_visible.size(), g_emojiRows) * kResultSize;
    return Px(kPickerHeight + listHeight - kResultSize - (g_statusVisible ? 0 : 26)
        + (g_recoveryMessage.empty() ? 0 : kRecoveryHeight));
}

void ResizePicker(int height) {
    RECT bounds{};
    GetWindowRect(g_window, &bounds);
    int y = bounds.top;
    if (IsWindowVisible(g_window) && g_pickerAboveAnchor) y = bounds.bottom - height;
    SetWindowPos(g_window, nullptr, bounds.left, y, Px(kPickerWidth), height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    ClampWindow(g_window);
}

void CenterOnActiveMonitor() {
    CancelPendingReturn();
    BeginPickerSession();
    HWND active = GetForegroundWindow();
    CaptureInputTarget(active);
    RECT anchor{};
    bool hasAnchor = TryGetTextFieldAnchor(active, anchor);
    if (!hasAnchor) {
        POINT cursor{};
        if (GetCursorPos(&cursor)) {
            anchor = {cursor.x, cursor.y, cursor.x + 1, cursor.y + 1};
            hasAnchor = true;
        }
    }
    HMONITOR monitor = hasAnchor ? MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST)
                                 : MonitorFromWindow(active, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    const RECT& area = info.rcWork;
    // Move onto the destination monitor first so Windows supplies its actual DPI.
    SetWindowPos(g_window, nullptr, area.left, area.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    UpdateDpi(GetDpiForWindow(g_window));
    int x = area.left + ((area.right - area.left) - Px(kPickerWidth)) / 2;
    const int pickerHeight = PickerHeight();
    int y = area.top + ((area.bottom - area.top) - pickerHeight) / 2;
    if (hasAnchor) {
        const auto placement = PlacePickerNearAnchor(anchor.left, anchor.top, anchor.right, anchor.bottom,
            area.left, area.top, area.right, area.bottom, Px(kPickerWidth), pickerHeight, Px(8));
        x = placement.x;
        y = placement.y;
        g_pickerAboveAnchor = placement.aboveAnchor;
    } else {
        g_pickerAboveAnchor = false;
    }
    SetWindowPos(g_window, HWND_TOPMOST, x, y, Px(kPickerWidth), pickerHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    ClampWindow(g_window);
    SetForegroundWindow(g_window);
    SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, -1);
}


void UpdateStatusLine() {
    if (!g_status) return;
    if (!g_storageDiagnostic.empty()) { SetWindowTextW(g_status, g_storageDiagnostic.c_str()); return; }
    const auto index = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
    std::wstring label = L"No matches. Teach this phrase with Alt+A.";
    if (index >= 0 && static_cast<size_t>(index) < g_displayVisible.size()) {
        const auto& result = g_displayVisible[index];
        const auto* exact = g_catalog.Find(result.id == g_variantTarget && !g_variantPayload.empty() ? g_variantPayload : result.payload);
        label = exact && result.id.kind == ResultKind::Emoji
            ? FormatEmojiDisplayName(*exact, g_profile.settings.displayLanguages) : result.label;
        if (result.id == g_variantTarget && !g_variantPayload.empty()) label += L" (once)";
    }
    SetWindowTextW(g_status, label.c_str());
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
    ResizePicker(PickerHeight());
}

void SetEmojiRows(int rows) {
    rows = std::clamp(rows, kMinEmojiRows, kMaxEmojiRows);
    if (rows == g_emojiRows) return;
    g_emojiRows = rows;
    SaveSettings();
    RefreshList();
    ResizePicker(PickerHeight());
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
    g_emojiFont = CreateFontW(-Px(30), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, font.name.c_str());
    if (g_emojiFormat) {
        g_emojiFormat->Release();
        g_emojiFormat = nullptr;
    }
    if (g_dwriteFactory) {
        g_dwriteFactory->CreateTextFormat(font.name.c_str(), nullptr,
                                          DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL, static_cast<float>(Px(30)), L"",
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
    // BindDC bounds and font sizes are physical pixels; keep D2D at 96 DPI.
    properties.dpiX = properties.dpiY = 96.0f;
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

void DrawColorEmoji(HDC dc, const RECT& bounds, const std::wstring& glyph, COLORREF color = CLR_INVALID) {
    if (color == CLR_INVALID) color = Foreground();
    SetTextColor(dc, color);
    if (!g_d2dTarget || !g_emojiFormat || FAILED(g_d2dTarget->BindDC(dc, &bounds))) {
        const HFONT previous = static_cast<HFONT>(SelectObject(dc, g_emojiFont));
        DrawTextW(dc, glyph.c_str(), -1, const_cast<RECT*>(&bounds),
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        SelectObject(dc, previous);
        return;
    }
    g_d2dTarget->BeginDraw();
    ID2D1SolidColorBrush* brush{};
    const D2D1_COLOR_F textColor{GetRValue(color)/255.0f, GetGValue(color)/255.0f, GetBValue(color)/255.0f, 1.0f};
    if (SUCCEEDED(g_d2dTarget->CreateSolidColorBrush(textColor, &brush))) {
        // A DCRenderTarget's coordinate system starts at its current BindDC clip rectangle.
        const D2D1_RECT_F textBounds{0.0f, 0.0f,
                                     static_cast<float>(bounds.right - bounds.left),
                                     static_cast<float>(bounds.bottom - bounds.top)};
        const auto options = !HighContrast() && !g_emojiFonts.empty() && g_emojiFonts[g_emojiFontIndex].color
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
    const wchar_t* cue = L"";
    SendMessageW(g_edit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(cue));
    std::wstring tip = g_sortByUsage ? L"SwashMoji — Most used (Alt+T) — Alt+E" : L"SwashMoji — Most recent (Alt+T) — Alt+E";
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
        L"Clear recent choices, usage counts, and learned search preferences?\n\nYour aliases, favorites, and appearance settings will stay. This cannot be undone.",
        L"Clear learned history",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (result != IDYES) return;

    ClearHistory(g_profile);
    g_rankingPreferences = g_profile;
    SaveProfile();
    RefreshList();
}

const wchar_t* HelpText() {
    return L"SwashMoji — keyboard guide\r\n\r\n"
        L"Alt+E: Open from any app. Search English, Norwegian, German, Italian, French or Spanish names, phrases and aliases.\r\n\r\n"
        L"Click or Enter: Insert and close.\r\nCtrl+click or Ctrl+Enter: Insert and keep open.\r\n"
        L"Shift+Enter: Copy and close after success.\r\n"
        L"Tab / Shift+Tab: Move between search, results and available recovery actions.\r\n"
        L"Arrows and Ctrl+arrows navigate results, including while searching. Keep typing to refine the query. Shift+arrows and Home/End edit the query.\r\n"
        L"Arrows in results: Move spatially. Page Up/Down: Previous/next page. Home/End: First/last result.\r\n"
        L"Ctrl+Backspace: Delete the previous word or selection. Ctrl+Z: Undo.\r\n"
        L"Esc: Close Details, vocabulary or help first; otherwise dismiss and return to the original app.\r\n\r\n"
        L"My vocabulary: Create named combinations of 2–8 emoji, with explicit variants and insertion order. Global tone does not change a saved combination.\r\n"
        L"Details (Alt+D or right-click): Larger preview and valid catalog variants. Use once applies to the next successful insertion or copy; Cancel discards the draft. Your global tone stays unchanged.\r\n"
        L"Hover briefly over a result for a preview without changing keyboard selection.\r\n\r\n"
        L"Languages in the tray selects one or two languages for emoji names. Search still uses all available languages. Long names are shortened visually. Combination sequence tiles show the authored order. Save combination saves; Close discards the draft.\r\n\r\n"
        L"Alt+F: Cycle emoji fonts (formerly Tab).\r\nAlt+I: Cycle global skin tone.\r\n"
        L"Alt+1 / 2 / 3: One, two or three rows.\r\nAlt+T: Recent / most-used sorting.\r\n"
        L"Alt+S: Show/hide selected-result text. F1: This guide.\r\n\r\n"
        L"Alt+A: Add alias, or teach an unmatched phrase.\r\nAlt+P: Pin/unpin favorite.\r\n"
        L"My vocabulary in the tray edits aliases and orders up to ten favorites. Save alias saves; Close discards drafts.\r\n\r\n"
        L"Failures preserve your query and choice. Alt+C: Copy instead. Partial input is never automatically retried; check the destination. Direct insertion leaves the clipboard untouched.\r\n\r\n"
        L"Learning changes future sessions; repeated insertion keeps results stable. Learn from searches toggles query learning. Clear learned history retains aliases, combinations, favorites and appearance.\r\n\r\n"
        L"Preferences stay locally in %LOCALAPPDATA%\\SwashMoji. No runtime network access.";
}

void LayoutHelp(HWND window) {
    RECT area{}; GetClientRect(window, &area);
    const int margin = MulDiv(12, GetDpiForWindow(window), 96);
    MoveWindow(GetDlgItem(window, 1), margin, margin, std::max(1L, area.right - margin * 2),
        std::max(1L, area.bottom - margin * 2), TRUE);
}
void HelpFont(HWND window) {
    auto old = reinterpret_cast<HFONT>(GetWindowLongPtrW(window, GWLP_USERDATA));
    auto font = CreateFontW(-MulDiv(16, GetDpiForWindow(window), 96), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(font));
    SendDlgItemMessageW(window, 1, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    if (old) DeleteObject(old);
}
LRESULT CALLBACK HelpWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateWindowExW(0, L"EDIT", HelpText(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL, 0, 0, 0, 0, window, reinterpret_cast<HMENU>(1), nullptr, nullptr);
        HelpFont(window); LayoutHelp(window); return 0;
    case WM_SIZE: LayoutHelp(window); return 0;
    case WM_DPICHANGED: {
        const auto& bounds = *reinterpret_cast<RECT*>(lParam);
        SetWindowPos(window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        HelpFont(window); ClampWindow(window); LayoutHelp(window); return 0;
    }
    case WM_SETFOCUS: SetFocus(GetDlgItem(window, 1)); return 0;
    case WM_CLOSE: DestroyWindow(window); return 0;
    case WM_DESTROY:
        DeleteObject(reinterpret_cast<HFONT>(GetWindowLongPtrW(window, GWLP_USERDATA)));
        g_helpWindow = nullptr;
        if (IsWindowVisible(g_window)) { SetForegroundWindow(g_window); SetFocus(g_edit); }
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

    RECT windowRect{0, 0, Px(kHelpWidth), Px(kHelpHeight)};
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
    ClampWindow(g_helpWindow);
    ShowWindow(g_helpWindow, SW_SHOW);
    UpdateWindow(g_helpWindow);
    SetForegroundWindow(g_helpWindow);
}

void ShowTrayMenu() {
    CancelPendingReturn();
    const auto target = CaptureExternalTarget(GetForegroundWindow());
    if (target.window) { g_inputTarget = target; g_session.originalTarget = target.window; }
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, kSortRecentId, L"Sort: Most recent");
    AppendMenuW(menu, MF_STRING, kSortMostUsedId, L"Sort: Most used");
    CheckMenuRadioItem(menu, kSortRecentId, kSortMostUsedId,
                       g_sortByUsage ? kSortMostUsedId : kSortRecentId, MF_BYCOMMAND);
    AppendMenuW(menu, MF_STRING, kClearUsageHistoryId, L"Clear learned history...");
    AppendMenuW(menu, MF_STRING, kVocabularyId, L"My vocabulary...");
    AppendMenuW(menu, MF_STRING, kDisplayLanguagesId, L"Languages...");
    AppendMenuW(menu, MF_STRING | (g_profile.settings.learnQueries ? MF_CHECKED : MF_UNCHECKED),
        kLearnQueriesId, L"Learn from searches");
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
    if (command == kSortRecentId) {
        SetSortMode(false);
    } else if (command == kSortMostUsedId) {
        SetSortMode(true);
    } else if (command == kClearUsageHistoryId) {
        ConfirmAndClearUsageHistory();
    } else if (command == kVocabularyId) {
        OpenVocabulary(false);
    } else if (command == kDisplayLanguagesId) {
        if (g_vocabularyOpen || g_detailsOpen) return;
        CloseHover();
        const auto selected = g_session.selected;
        g_vocabularyOpen = true;
        const bool opened = ShowLanguagePreferences(g_window, reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_window, GWLP_HINSTANCE)),
            g_profile, [] { SaveProfile(); return !g_profileUnsaved; });
        g_vocabularyOpen = false;
        if (!opened) MessageBoxW(g_window, L"Could not open language preferences.", L"SwashMoji", MB_ICONERROR);
        RefreshList();
        for (size_t i = 0; i < g_displayVisible.size(); ++i) if (g_displayVisible[i].id == selected) {
            SendMessageW(g_list, LB_SETCURSEL, i, 0);
            g_session.selected = selected;
            break;
        }
        UpdateStatusLine();
        if (IsWindowVisible(g_window)) SetFocus(g_edit);
    } else if (command == kLearnQueriesId) {
        g_profile.settings.learnQueries = !g_profile.settings.learnQueries;
        SaveProfile();
        RefreshList();
    } else if (command == kExitId) {
        DestroyWindow(g_window);
    }
}

void LayoutChildren(HWND window) {
    if (!g_list) return;
    RECT area{}; GetClientRect(window, &area);
    const int margin = Px(10);
    MoveWindow(g_edit, margin, Px(10), area.right - margin * 2, Px(kInputHeight), TRUE);
    const int listY = Px(52), listHeight = GridRows(g_visible.size(), g_emojiRows) * Px(kResultSize);
    MoveWindow(g_list, margin, listY, area.right - margin * 2, listHeight, TRUE);
    MoveWindow(g_teachPhrase, margin + Px(12), listY + Px(5), area.right - margin * 2 - Px(24), Px(36), TRUE);
    const int footerY = listY + listHeight;
    MoveWindow(g_status, margin + Px(2), footerY + Px(6), area.right - margin * 2 - Px(4), Px(20), TRUE);
    const int recoveryY = footerY + (g_statusVisible ? Px(34) : Px(8));
    MoveWindow(g_recoveryLabel, margin, recoveryY, area.right - margin * 2 - Px(132), Px(kRecoveryHeight - 12), TRUE);
    MoveWindow(g_copyInstead, area.right - margin - Px(126), recoveryY + Px(8), Px(126), Px(30), TRUE);
}

void CloseHover() {
    if (g_window) KillTimer(g_window, kHoverTimerId);
    g_hoverIndex = -1;
    if (g_hoverWindow) { DestroyWindow(g_hoverWindow); g_hoverWindow = nullptr; }
}

void SelectionChanged() {
    const auto index = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
    if (index >= 0 && static_cast<size_t>(index) < g_displayVisible.size()) {
        g_session.selected = g_displayVisible[index].id;
        NotifyWinEvent(EVENT_OBJECT_SELECTION, g_list, OBJID_CLIENT, static_cast<LONG>(index + 1));
    }
    UpdateStatusLine();
}

int HitResult(POINT point) {
    RECT client{};
    GetClientRect(g_list, &client);
    if (!PtInRect(&client, point)) return -1;
    const auto hit = SendMessageW(g_list, LB_ITEMFROMPOINT, 0, MAKELPARAM(point.x, point.y));
    const int index = LOWORD(hit);
    RECT item{};
    if (HIWORD(hit) || index >= static_cast<int>(g_displayVisible.size()) ||
        SendMessageW(g_list, LB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&item)) == LB_ERR ||
        !PtInRect(&item, point)) return -1;
    return index;
}

void RestoreTargetOnEscape() {
    DismissPicker();
    if (g_inputPlatform.ValidTarget(g_inputTarget)) SetForegroundWindow(reinterpret_cast<HWND>(g_inputTarget.window));
}

void FocusNext(bool reverse) {
    const HWND order[]{g_edit, g_list, g_teachPhrase, g_copyInstead};
    std::vector<HWND> available;
    for (auto window : order) if (IsWindowVisible(window) && IsWindowEnabled(window) &&
        (window != g_list || !g_displayVisible.empty())) available.push_back(window);
    if (available.empty()) return;
    auto found = std::find(available.begin(), available.end(), GetFocus());
    int index = found == available.end() ? (reverse ? 0 : -1) : static_cast<int>(found - available.begin());
    index = (index + (reverse ? -1 : 1) + static_cast<int>(available.size())) % static_cast<int>(available.size());
    SetFocus(available[index]);
}

void ClampWindow(HWND window) {
    RECT rect{};
    GetWindowRect(window, &rect);
    MONITORINFO monitor{sizeof(monitor)};
    if (!GetMonitorInfoW(MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST), &monitor)) return;
    const auto& area = monitor.rcWork;
    const int width = std::min(rect.right - rect.left, area.right - area.left);
    const int height = std::min(rect.bottom - rect.top, area.bottom - area.top);
    SetWindowPos(window, nullptr, std::clamp(rect.left, area.left, area.right - width),
        std::clamp(rect.top, area.top, area.bottom - height), width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void UpdateDpi(UINT dpi) {
    g_dpi = dpi ? dpi : 96;
    auto oldUi = g_uiFont, oldStatus = g_statusFont;
    auto oldLabel = g_resultLabelFont;
    g_uiFont = CreateFontW(-Px(20), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_statusFont = CreateFontW(-Px(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    g_resultLabelFont = CreateFontW(-Px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    for (auto control : {g_edit, g_list}) if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    for (auto control : {g_teachPhrase, g_copyInstead, g_recoveryLabel})
        if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_statusFont), TRUE);
    if (g_status) SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_resultLabelFont), TRUE);
    if (oldUi) DeleteObject(oldUi);
    if (oldStatus && oldStatus != oldUi) DeleteObject(oldStatus);
    if (oldLabel) DeleteObject(oldLabel);
    SelectEmojiFont(g_emojiFontIndex);
    if (g_list) {
        SendMessageW(g_list, LB_SETITEMHEIGHT, 0, Px(kResultSize));
        SendMessageW(g_list, LB_SETCOLUMNWIDTH, Px(kResultSize), 0);
    }
    CloseHover();
}

void DrawLargePreview(HDC dc, RECT area, const std::wstring& payload, UINT dpi, int fontSize = 56, COLORREF color = CLR_INVALID) {
    const auto oldFont = g_emojiFont;
    auto* oldFormat = g_emojiFormat;
    g_emojiFormat = nullptr;
    const wchar_t* name = g_emojiFonts.empty() ? L"Segoe UI Emoji" : g_emojiFonts[g_emojiFontIndex].name.c_str();
    const int size = MulDiv(fontSize, dpi, 96);
    g_emojiFont = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, name);
    if (g_dwriteFactory) {
        g_dwriteFactory->CreateTextFormat(name, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, static_cast<float>(size), L"", &g_emojiFormat);
        if (g_emojiFormat) {
            g_emojiFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_emojiFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, Foreground());
    DrawColorEmoji(dc, area, payload, color == CLR_INVALID ? GetSysColor(COLOR_WINDOWTEXT) : color);
    if (g_emojiFormat) g_emojiFormat->Release();
    DeleteObject(g_emojiFont);
    g_emojiFont = oldFont; g_emojiFormat = oldFormat;
}

struct DetailsState {
    std::vector<const Emoji*> variants;
    size_t selected{};
};
INT_PTR CALLBACK DetailsProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    INT_PTR themeResult{};
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themeResult)) return themeResult;
    if (message == WM_MEASUREITEM && reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlID == 403) {
        reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = MulDiv(20, GetDpiForWindow(dialog), 96);
        return TRUE;
    }
    if (message == WM_DRAWITEM && reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->CtlID == 403) {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->itemID == static_cast<UINT>(-1)) return TRUE;
        const auto length = SendMessageW(item->hwndItem, LB_GETTEXTLEN, item->itemID, 0);
        std::wstring text(static_cast<size_t>(std::max<LRESULT>(0, length)) + 1, L'\0');
        SendMessageW(item->hwndItem, LB_GETTEXT, item->itemID, reinterpret_cast<LPARAM>(text.data()));
        text.resize(static_cast<size_t>(std::max<LRESULT>(0, length)));
        const bool selected = (item->itemState & ODS_SELECTED) != 0;
        HBRUSH fill = selected
            ? CreateSolidBrush(HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : kSelected)
            : NativeTheme::SurfaceBrush();
        FillRect(item->hDC, &item->rcItem, fill);
        if (selected) DeleteObject(fill);
        const auto color = selected && HighContrast() ? GetSysColor(COLOR_HIGHLIGHTTEXT) : Foreground();
        NativeEmoji::DrawLine(item->hDC, item->rcItem, text,
            static_cast<float>(MulDiv(14, GetDpiForWindow(dialog), 96)), color, false, !HighContrast());
        if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
        return TRUE;
    }
    auto* state = reinterpret_cast<DetailsState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        NativeTheme::Apply(dialog);
        state = reinterpret_cast<DetailsState*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        NativeTheme::ApplyEmojiFont(dialog, {402, 403});
        int width = 0;
        HDC dc = GetDC(dialog);
        auto font = SelectObject(dc, reinterpret_cast<HFONT>(SendDlgItemMessageW(dialog, 403, WM_GETFONT, 0, 0)));
        for (const auto* emoji : state->variants) {
            const auto label = emoji->glyph + L"  " + FormatEmojiDisplayName(*emoji, g_profile.settings.displayLanguages);
            SendDlgItemMessageW(dialog, 403, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            SIZE extent{};
            GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.size()), &extent);
            width = std::max(width, static_cast<int>(extent.cx));
        }
        SelectObject(dc, font); ReleaseDC(dialog, dc);
        SendDlgItemMessageW(dialog, 403, LB_SETHORIZONTALEXTENT, width + 16, 0);
        SendDlgItemMessageW(dialog, 403, LB_SETCURSEL, state->selected, 0);
        SetDlgItemTextW(dialog, 402, FormatEmojiDisplayName(*state->variants[state->selected], g_profile.settings.displayLanguages).c_str());
        ClampWindow(dialog);
        SetFocus(GetDlgItem(dialog, 403));
        return FALSE;
    }
    if (!state) return FALSE;
    if (message == WM_DESTROY) { NativeTheme::ReleaseEmojiFont(dialog); return FALSE; }
    if (message == WM_COMMAND) {
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
        if (LOWORD(wParam) == IDOK) { EndDialog(dialog, IDOK); return TRUE; }
        if (LOWORD(wParam) == 403 && HIWORD(wParam) == LBN_SELCHANGE) {
            const auto selection = SendDlgItemMessageW(dialog, 403, LB_GETCURSEL, 0, 0);
            if (selection >= 0 && static_cast<size_t>(selection) < state->variants.size()) state->selected = selection;
            SetDlgItemTextW(dialog, 402, FormatEmojiDisplayName(*state->variants[state->selected], g_profile.settings.displayLanguages).c_str());
            InvalidateRect(GetDlgItem(dialog, 401), nullptr, TRUE);
            return TRUE;
        }
    }
    if (message == WM_DRAWITEM && reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->CtlID == 401) {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        FillRect(item->hDC, &item->rcItem, NativeTheme::SurfaceBrush());
        DrawLargePreview(item->hDC, item->rcItem, state->variants[state->selected]->glyph, GetDpiForWindow(dialog));
        return TRUE;
    }
    // Per-Monitor V2 dialog manager scales controls, fonts and dialog bounds.
    return FALSE;
}

void OpenDetails() {
    if (g_detailsOpen || g_vocabularyOpen) return;
    const auto index = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
    if (index < 0 || static_cast<size_t>(index) >= g_displayVisible.size()) return;
    const auto result = g_displayVisible[index];
    if (result.id.kind == ResultKind::Combination) {
        const auto found = g_profile.combinations.find(result.id.value);
        if (found == g_profile.combinations.end()) return;
        CloseHover(); CancelPendingReturn();
        HWND focus = GetFocus(); g_detailsOpen = true;
        ShowCombinationDetails(g_window, reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_window, GWLP_HINSTANCE)), g_catalog, found->second, g_profile.settings.displayLanguages);
        g_detailsOpen = false;
        if (IsWindowVisible(g_window)) SetFocus(IsWindow(focus) ? focus : g_list);
        return;
    }
    DetailsState state{CatalogVariants(g_catalog, result.id)};
    if (state.variants.empty()) return;
    const auto payload = result.id == g_variantTarget && !g_variantPayload.empty() ? g_variantPayload : result.payload;
    for (size_t i = 0; i < state.variants.size(); ++i) if (state.variants[i]->glyph == payload) state.selected = i;
    CloseHover(); CancelPendingReturn();
    HWND focus = GetFocus();
    g_detailsOpen = true;
    const auto answer = DialogBoxParamW(reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_window, GWLP_HINSTANCE)),
        MAKEINTRESOURCEW(400), g_window, DetailsProc, reinterpret_cast<LPARAM>(&state));
    g_detailsOpen = false;
    if (answer == IDOK) {
        g_variantTarget = result.id;
        g_variantPayload = state.variants[state.selected]->glyph;
        RefreshList();
    }
    if (IsWindowVisible(g_window)) SetFocus(IsWindow(focus) ? focus : g_list);
}

LRESULT CALLBACK HoverProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCHITTEST) return HTTRANSPARENT;
    if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT area{}; GetClientRect(window, &area);
        FillRect(dc, &area, GetSysColorBrush(COLOR_INFOBK));
        if (g_hoverIndex >= 0 && static_cast<size_t>(g_hoverIndex) < g_displayVisible.size()) {
            const auto& result = g_displayVisible[g_hoverIndex];
            const auto payload = result.id == g_variantTarget && !g_variantPayload.empty() ? g_variantPayload : result.payload;
            RECT glyph = area; glyph.bottom = Px(80);
            const auto combo = result.id.kind == ResultKind::Combination ? g_profile.combinations.find(result.id.value) : g_profile.combinations.end();
            const int size = combo == g_profile.combinations.end() ? 56 : std::min<int>(56, (glyph.right - glyph.left) * 96 / static_cast<int>(g_dpi * combo->second.entries.size()));
            DrawLargePreview(dc, glyph, payload, g_dpi, size);
            RECT label{Px(10), Px(82), area.right - Px(10), area.bottom - Px(8)};
            const auto* exact = g_catalog.Find(payload);
            auto old = SelectObject(dc, g_statusFont);
            SetTextColor(dc, GetSysColor(COLOR_INFOTEXT));
            DrawTextW(dc, exact ? FormatEmojiDisplayName(*exact, g_profile.settings.displayLanguages).c_str() : result.label.c_str(), -1, &label, DT_CENTER | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(dc, old);
        }
        EndPaint(window, &paint); return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowHover() {
    KillTimer(g_window, kHoverTimerId);
    POINT cursor{}; GetCursorPos(&cursor);
    POINT local = cursor; ScreenToClient(g_list, &local);
    if (g_hoverIndex < 0 || HitResult(local) != g_hoverIndex || !IsWindowVisible(g_window) ||
        g_detailsOpen || g_vocabularyOpen || GetForegroundWindow() != g_window) { CloseHover(); return; }
    WNDCLASSW cls{}; cls.hInstance = GetModuleHandleW(nullptr); cls.lpszClassName = L"SwashMojiHover";
    cls.lpfnWndProc = HoverProc; cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&cls);
    g_hoverWindow = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        cls.lpszClassName, L"Emoji preview", WS_POPUP | WS_BORDER, cursor.x + Px(14), cursor.y + Px(20),
        Px(300), Px(150), g_window, nullptr, cls.hInstance, nullptr);
    ClampWindow(g_hoverWindow);
    ShowWindow(g_hoverWindow, SW_SHOWNOACTIVATE);
}

[[maybe_unused]] void MoveSelection(int direction) {
    if (g_displayVisible.empty()) return;
    int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected == LB_ERR) selected = 0;
    selected = static_cast<int>(GridMove(selected, g_displayVisible.size(), g_emojiRows,
        direction == -g_emojiRows ? -1 : direction == g_emojiRows ? 1 : 0,
        direction == -g_emojiRows || direction == g_emojiRows ? 0 : direction));
    SendMessageW(g_list, LB_SETCURSEL, selected, 0);
    SelectionChanged();
}

LRESULT CALLBACK InputProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam) {
    const WNDPROC original = control == g_edit ? g_editProc : g_listProc;
    if (message == WM_GETDLGCODE) return CallWindowProcW(original, control, message, wParam, lParam) | DLGC_WANTARROWS;
    if (control == g_list && message == WM_MOUSEMOVE) {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        const int index = HitResult(point);
        if (index != g_hoverIndex || abs(point.x - g_hoverPoint.x) > Px(3) || abs(point.y - g_hoverPoint.y) > Px(3)) {
            CloseHover();
            g_hoverIndex = index; g_hoverPoint = point;
            if (index >= 0) SetTimer(g_window, kHoverTimerId, 350, nullptr);
        }
        TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, control, 0}; TrackMouseEvent(&track);
    }
    if (control == g_list && (message == WM_MOUSELEAVE || message == WM_MOUSEWHEEL || message == WM_HSCROLL)) CloseHover();
    if (control == g_list && message == WM_LBUTTONDOWN) {
        CloseHover();
        g_pressedIndex = HitResult({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        if (g_pressedIndex < 0) return 0;
    }
    if (control == g_list && message == WM_CONTEXTMENU) {
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        if (point.x == -1 && point.y == -1) {
            RECT area{};
            GetWindowRect(control, &area);
            point = {area.left + 12, area.bottom};
        } else {
            POINT local = point;
            ScreenToClient(control, &local);
            const int item = HitResult(local);
            if (item < 0) return 0;
            SendMessageW(control, LB_SETCURSEL, item, 0);
            SelectionChanged();
        }
        if (g_displayVisible.empty()) return 0;
        CloseHover();
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, kDetailsId, L"Details...");
        AppendMenuW(menu, MF_STRING, kVocabularyId, L"Add alias...\tAlt+A");
        const auto index = SendMessageW(control, LB_GETCURSEL, 0, 0);
        if (index >= 0 && static_cast<size_t>(index) < g_displayVisible.size())
            AppendMenuW(menu, MF_STRING, kPinId, IsPinned(g_profile, g_displayVisible[index].id)
                ? L"Unpin favorite\tAlt+P" : L"Pin favorite\tAlt+P");
        const auto command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
            point.x, point.y, 0, g_window, nullptr);
        DestroyMenu(menu);
        if (command == kDetailsId) OpenDetails();
        if (command == kVocabularyId) OpenVocabulary(true);
        if (command == kPinId) ToggleSelectedPin();
        return 0;
    }
    if (control == g_list && message == WM_LBUTTONUP) {
        const LRESULT result = CallWindowProcW(original, control, message, wParam, lParam);
        const int hit = HitResult({GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        if (hit >= 0 && hit == g_pressedIndex) {
            SendMessageW(control, LB_SETCURSEL, hit, 0);
            SelectionChanged();
            InsertSelection((GetKeyState(VK_CONTROL) & 0x8000) != 0);
        }
        g_pressedIndex = -1;
        return result;
    }
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        if (wParam == VK_TAB) { CloseHover(); FocusNext((GetKeyState(VK_SHIFT) & 0x8000) != 0); return 0; }
        if (wParam == 'I' && (GetKeyState(VK_MENU) & 0x8000)) {
            CycleSkinTone();
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            RestoreTargetOnEscape();
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (!g_recoveryMessage.empty() && (lParam & (1u << 30))) return 0;
            if (GetKeyState(VK_SHIFT) & 0x8000) CopySelection();
            else if (GetKeyState(VK_CONTROL) & 0x8000) InsertSelection(true);
            else InsertSelection();
            return 0;
        }
        // Search stays focused throughout keyboard navigation, so typing always
        // refines the query. Ctrl+arrows also navigate; Shift retains text selection.
        const bool searchArrow = control == g_edit &&
            !(GetKeyState(VK_SHIFT) & 0x8000) &&
            !(GetKeyState(VK_MENU) & 0x8000) &&
            (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN);
        if (control == g_list || searchArrow) {
            int dx = 0, dy = 0;
            if (wParam == VK_LEFT) dx = -1;
            if (wParam == VK_RIGHT) dx = 1;
            if (wParam == VK_UP) dy = -1;
            if (wParam == VK_DOWN) dy = 1;
            // A single row has no vertical neighbor: Up/Down still select the
            // previous/next result, both in search and in the results control.
            const int rows = GridRows(g_visible.size(), g_emojiRows);
            if (rows == 1 && dy) { dx = dy; dy = 0; }
            if (wParam == VK_PRIOR) dx = -kEmojiColumns;
            if (wParam == VK_NEXT) dx = kEmojiColumns;
            if (dx || dy || wParam == VK_HOME || wParam == VK_END) {
                CloseHover();
                const auto old = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
                auto next = GridMove(old < 0 ? 0 : old, g_displayVisible.size(), rows, dx, dy);
                if (wParam == VK_HOME) next = 0;
                if (wParam == VK_END && !g_displayVisible.empty()) next = g_displayVisible.size() - 1;
                SendMessageW(g_list, LB_SETCURSEL, next, 0);
                SelectionChanged(); return 0;
            }
        }
    }
    return CallWindowProcW(original, control, message, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        g_dpi = GetDpiForWindow(window);
        g_rankingPreferences = g_profile;
        g_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)), nullptr, nullptr);
        g_list = CreateWindowExW(0, L"LISTBOX", L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                 LBS_OWNERDRAWFIXED | LBS_MULTICOLUMN | LBS_HASSTRINGS,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);
        g_status = CreateWindowExW(0, L"STATIC", L"",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS | SS_NOPREFIX,
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
        SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_resultLabelFont ? g_resultLabelFont : g_statusFont), TRUE);
        SendMessageW(g_list, LB_SETCOLUMNWIDTH, Px(kResultSize), 0);
        SetWindowTheme(g_edit, HighContrast() ? L"" : L"DarkMode_Explorer", nullptr);
        SetWindowTheme(g_list, HighContrast() ? L"" : L"DarkMode_Explorer", nullptr);
        g_editProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_edit, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        g_listProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_list, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        EnableWordDeletion(g_edit);
        SetWindowTextW(g_list, L"Matching emoji");
        LayoutChildren(window);
        RefreshList();
        AddTrayIcon(window);
        UpdateSortIndicator();
        UpdateStatusLine();
        return 0;
    }
    case WM_DPICHANGED: {
        UpdateDpi(HIWORD(wParam));
        const auto& bounds = *reinterpret_cast<RECT*>(lParam);
        SetWindowPos(window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        ClampWindow(window);
        LayoutChildren(window);
        return 0;
    }
    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
        CloseHover();
        SetWindowTheme(g_edit, HighContrast() ? L"" : L"DarkMode_Explorer", nullptr);
        SetWindowTheme(g_list, HighContrast() ? L"" : L"DarkMode_Explorer", nullptr);
        RedrawWindow(window, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        return 0;
    case WM_ERASEBKGND: {
        RECT area{}; GetClientRect(window, &area);
        FillRect(reinterpret_cast<HDC>(wParam), &area, HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : g_backgroundBrush);
        return 1;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) CloseHover();
        break;
    case WM_SIZE: LayoutChildren(window); return 0;
    case WM_MEASUREITEM:
        if (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlID == kListId) {
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = Px(kResultSize);
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemWidth = Px(kResultSize);
            return TRUE;
        }
        break;
    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID != kListId || item->itemID >= g_displayVisible.size()) break;
        const bool selected = (item->itemState & ODS_SELECTED) != 0;
        HBRUSH fill = CreateSolidBrush(selected ? (HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : kSelected) : Background());
        FillRect(item->hDC, &item->rcItem, fill);
        DeleteObject(fill);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, Foreground());
        const HFONT previousFont = static_cast<HFONT>(SelectObject(item->hDC, g_uiFont));
        const auto& result = g_displayVisible[item->itemID];
        RECT glyph = item->rcItem;
        InflateRect(&glyph, -3, -3);
        const auto combination = result.id.kind == ResultKind::Combination ? g_profile.combinations.find(result.id.value) : g_profile.combinations.end();
        if (combination != g_profile.combinations.end()) {
            const auto& entries = combination->second.entries;
            for (size_t n = 0; n < std::min(size_t{2}, entries.size()); ++n) {
                RECT part = glyph;
                const auto middle = (glyph.left + glyph.right) / 2;
                if (n == 0) part.right = middle; else part.left = middle;
                if (entries.size() > 2) part.bottom -= Px(8);
                DrawLargePreview(item->hDC, part, entries[n].payload, g_dpi, 20,
                    selected && HighContrast() ? GetSysColor(COLOR_HIGHLIGHTTEXT) : Foreground());
            }
            if (entries.size() > 2) {
                RECT more = glyph; more.top = more.bottom - Px(15);
                DrawTextW(item->hDC, L"…", -1, &more, DT_CENTER | DT_SINGLELINE);
            }
        } else {
            DrawColorEmoji(item->hDC, glyph, result.id == g_variantTarget && !g_variantPayload.empty() ? g_variantPayload : result.payload,
                selected && HighContrast() ? GetSysColor(COLOR_HIGHLIGHTTEXT) : Foreground());
        }
        if (selected && !HighContrast()) {
            RECT outline = item->rcItem;
            InflateRect(&outline, -Px(1), -Px(1));
            const auto edge = CreateSolidBrush(RGB(104, 157, 201));
            FrameRect(item->hDC, &outline, edge);
            DeleteObject(edge);
        }
        if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
        SelectObject(item->hDC, previousFont);
        return TRUE;
    }
    case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wParam), Foreground());
        SetBkColor(reinterpret_cast<HDC>(wParam), HighContrast() ? Background() : kInputBackground);
        return reinterpret_cast<LRESULT>(HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : g_inputBrush);
    case WM_CTLCOLORLISTBOX:
        SetTextColor(reinterpret_cast<HDC>(wParam), Foreground());
        SetBkColor(reinterpret_cast<HDC>(wParam), Background());
        return reinterpret_cast<LRESULT>(HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : g_backgroundBrush);
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wParam), HighContrast() || reinterpret_cast<HWND>(lParam) == g_status
            ? Foreground() : kMutedText);
        SetBkColor(reinterpret_cast<HDC>(wParam), Background());
        return reinterpret_cast<LRESULT>(HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : g_backgroundBrush);
    case WM_COMMAND:
        if (LOWORD(wParam) == kListId && HIWORD(wParam) == LBN_SELCHANGE) SelectionChanged();
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
        if (g_vocabularyOpen || g_detailsOpen) return 0;
        if (wParam == kHotkeyId) {
            SetWindowTextW(g_edit, L"");
            CenterOnActiveMonitor();
        }
        return 0;
    case WM_TIMER:
        if (wParam == kHoverTimerId) { ShowHover(); return 0; }
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
        if (g_vocabularyOpen || g_detailsOpen) return 0;
        SetWindowTextW(g_edit, L"");
        CenterOnActiveMonitor();
        return 0;
    case kTrayMessage:
        if (g_vocabularyOpen || g_detailsOpen) return 0;
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
        CloseHover();
        CancelPendingReturn();
        if (g_foregroundHook) { UnhookWinEvent(g_foregroundHook); g_foregroundHook = nullptr; }
        UnregisterHotKey(window, kHotkeyId);
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool ProcessAppMessage(const MSG& message) {
        // Handle app-local Alt shortcuts before dispatch. Depending on focus and
        // popup state, Windows can address system-key messages to either the
        // child control or the top-level picker.
        const bool altPressed = (GetKeyState(VK_MENU) & 0x8000) ||
                                (message.message == WM_SYSKEYDOWN &&
                                 (message.lParam & (1u << 29)));
        const bool firstKeyPress = !(message.lParam & (1u << 30));
        const bool pickerKey = (message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            (message.hwnd == g_window || IsChild(g_window, message.hwnd));
        if (g_helpWindow && (message.hwnd == g_helpWindow || IsChild(g_helpWindow, message.hwnd)) &&
            message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) {
            DestroyWindow(g_helpWindow); return true;
        }
        if (pickerKey && message.wParam == VK_ESCAPE) { RestoreTargetOnEscape(); return true; }
        if (pickerKey && altPressed && firstKeyPress && message.wParam == 'D') { OpenDetails(); return true; }
        if (pickerKey && altPressed && firstKeyPress && message.wParam == 'A') {
            OpenVocabulary(true);
            return true;
        }
        if (pickerKey && altPressed && firstKeyPress && message.wParam == 'P') {
            ToggleSelectedPin();
            return true;
        }
        if (pickerKey && message.hwnd == g_teachPhrase) {
            if (message.wParam == VK_RETURN) { OpenVocabulary(true); return true; }
            if (message.wParam == VK_ESCAPE) { DismissPicker(); return true; }
        }
        if (pickerKey && message.hwnd == g_copyInstead && message.wParam == VK_ESCAPE) {
            DismissPicker();
            return true;
        }
        if (pickerKey && !g_recoveryMessage.empty() && firstKeyPress &&
            ((altPressed && message.wParam == 'C') ||
             (message.hwnd == g_copyInstead && message.wParam == VK_RETURN))) {
            CopySelection();
            return true;
        }
        if (pickerKey && firstKeyPress && message.wParam == VK_F1) {
            ShowHelp();
            return true;
        }
        if (pickerKey && message.wParam == VK_TAB) {
            CloseHover(); FocusNext((GetKeyState(VK_SHIFT) & 0x8000) != 0);
            return true;
        }
        if (pickerKey && altPressed && firstKeyPress) {
            if (message.wParam == 'F') { CycleEmojiFont(); return true; }
            if (message.wParam == 'T') {
                ToggleSortMode();
                return true;
            }
            if (message.wParam == 'S') {
                ToggleStatusLine();
                return true;
            }
            if (message.wParam == 'I') {
                CycleSkinTone();
                return true;
            }
            if (message.wParam >= '1' && message.wParam <= '3') {
                SetEmojiRows(static_cast<int>(message.wParam - '0'));
                return true;
            }
        }
    return false;
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
                               Px(kPickerWidth), PickerHeight(), nullptr, nullptr, instance, nullptr);
    if (!g_window) return 1;
    UpdateDpi(GetDpiForWindow(g_window));
    g_foregroundHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        nullptr, ForegroundChanged, 0, 0, WINEVENT_OUTOFCONTEXT);
    SendMessageW(g_window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIcon));
    SendMessageW(g_window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIcon));
    if (!RegisterHotKey(g_window, kHotkeyId, MOD_ALT | MOD_NOREPEAT, 'E')) {
        MessageBoxW(nullptr, L"Alt+E er allerede i bruk av et annet program.", L"SwashMoji", MB_ICONWARNING);
    }

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        if (ProcessAppMessage(message)) continue;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (g_uiAutomation) g_uiAutomation->Release();
    if (g_emojiFormat) g_emojiFormat->Release();
    if (g_dwriteFactory) g_dwriteFactory->Release();
    if (g_d2dTarget) g_d2dTarget->Release();
    if (g_d2dFactory) g_d2dFactory->Release();
    DeleteObject(g_emojiFont);
    DeleteObject(g_statusFont);
    DeleteObject(g_resultLabelFont);
    DeleteObject(g_uiFont);
    DeleteObject(g_inputBrush);
    DeleteObject(g_backgroundBrush);
    if (g_comInitialized) CoUninitialize();
    return static_cast<int>(message.wParam);
}
