#pragma once

#include <windows.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <initializer_list>

namespace SwashMoji::NativeTheme {

inline constexpr COLORREF Background = RGB(24, 24, 24);
inline constexpr COLORREF Surface = RGB(35, 35, 35);
inline constexpr COLORREF Text = RGB(235, 235, 235);
inline constexpr COLORREF MutedText = RGB(155, 155, 155);

inline bool HighContrast() {
    HIGHCONTRASTW value{sizeof(value)};
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(value), &value, 0) &&
           (value.dwFlags & HCF_HIGHCONTRASTON);
}

inline HBRUSH BackgroundBrush() {
    static HBRUSH brush = CreateSolidBrush(Background);
    return HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : brush;
}

inline HBRUSH SurfaceBrush() {
    static HBRUSH brush = CreateSolidBrush(Surface);
    return HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : brush;
}

inline COLORREF Foreground() { return HighContrast() ? GetSysColor(COLOR_WINDOWTEXT) : Text; }
inline COLORREF SecondaryText() { return HighContrast() ? GetSysColor(COLOR_WINDOWTEXT) : MutedText; }
inline void MarkMuted(HWND control) { SetPropW(control, L"SwashMojiMuted", reinterpret_cast<HANDLE>(1)); }

inline void ApplyEmojiFont(HWND dialog, std::initializer_list<int> controlIds) {
    const auto base = reinterpret_cast<HFONT>(SendMessageW(dialog, WM_GETFONT, 0, 0));
    LOGFONTW description{};
    if (!base || !GetObjectW(base, sizeof(description), &description)) return;
    wcscpy_s(description.lfFaceName, L"Segoe UI Emoji");
    description.lfQuality = CLEARTYPE_QUALITY;
    const auto font = CreateFontIndirectW(&description);
    if (!font) return;
    for (const int id : controlIds)
        if (const auto control = GetDlgItem(dialog, id))
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SetPropW(dialog, L"SwashMojiEmojiFont", font);
}

inline void ReleaseEmojiFont(HWND dialog) {
    if (const auto font = RemovePropW(dialog, L"SwashMojiEmojiFont")) DeleteObject(font);
}

inline void Apply(HWND dialog) {
    const bool enabled = !HighContrast();
    BOOL dark = enabled;
    // Attribute 20 is DWMWA_USE_IMMERSIVE_DARK_MODE on supported Windows builds.
    DwmSetWindowAttribute(dialog, 20, &dark, sizeof(dark));
    SetWindowTheme(dialog, enabled ? L"DarkMode_Explorer" : nullptr, nullptr);
    EnumChildWindows(dialog, [](HWND child, LPARAM parameter) -> BOOL {
        SetWindowTheme(child, parameter ? L"DarkMode_Explorer" : nullptr, nullptr);
        return TRUE;
    }, enabled);
    InvalidateRect(dialog, nullptr, TRUE);
}

inline bool HandleMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam, INT_PTR& result) {
    if (message == WM_SETTINGCHANGE) {
        Apply(dialog);
        return false;
    }
    auto dc = reinterpret_cast<HDC>(wParam);
    switch (message) {
    case WM_CTLCOLORDLG:
        result = reinterpret_cast<INT_PTR>(BackgroundBrush());
        return true;
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        SetTextColor(dc, Foreground());
        SetBkColor(dc, HighContrast() ? GetSysColor(COLOR_WINDOW) : Surface);
        result = reinterpret_cast<INT_PTR>(SurfaceBrush());
        return true;
    case WM_CTLCOLORSTATIC:
        SetTextColor(dc, GetPropW(reinterpret_cast<HWND>(lParam), L"SwashMojiMuted") ? SecondaryText() : Foreground());
        SetBkMode(dc, TRANSPARENT);
        result = reinterpret_cast<INT_PTR>(BackgroundBrush());
        return true;
    default:
        return false;
    }
}

} // namespace SwashMoji::NativeTheme
