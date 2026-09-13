#pragma once
#include <string>

namespace SwashMoji {
// Low byte: virtual key. High byte: Win32 MOD_ALT/CONTROL/SHIFT/WIN bits.
inline constexpr unsigned int kDefaultActivationHotkey = 0x0145;
inline bool ValidActivationHotkey(unsigned int value) {
    const auto key = value & 255, modifiers = value >> 8;
    return modifiers && modifiers <= 15 && (modifiers & 11) &&
        ((key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9') ||
         (key >= 0x70 && key <= 0x7A)); // F1-F11; F12 is reserved by Windows.
}
inline std::wstring ActivationHotkeyLabel(unsigned int value) {
    std::wstring label;
    const auto modifiers = value >> 8, key = value & 255;
    if (modifiers & 2) label += L"Ctrl+";
    if (modifiers & 1) label += L"Alt+";
    if (modifiers & 4) label += L"Shift+";
    if (modifiers & 8) label += L"Win+";
    if (key >= 0x70 && key <= 0x7A) label += L"F" + std::to_wstring(key - 0x6F);
    else label += static_cast<wchar_t>(key);
    return label;
}
}
