#pragma once
#include "activation_settings.h"
#include "native_theme.h"
#include "localization.h"
#include <functional>

namespace SwashMoji {
struct ActivationPreferencesState {
    unsigned int hotkey;
    bool startup;
    std::string uiLanguage{"en"};
    std::wstring diagnostic;
    std::function<bool(unsigned int, bool, std::wstring&)> apply;
    std::function<bool(std::wstring&)> importProfile;
    std::function<bool(std::wstring&)> exportProfile;
};
inline void SelectActivationChoice(HWND dialog, int control, unsigned int value) {
    const auto count = SendDlgItemMessageW(dialog, control, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i)
        if (SendDlgItemMessageW(dialog, control, CB_GETITEMDATA, i, 0) == value)
            SendDlgItemMessageW(dialog, control, CB_SETCURSEL, i, 0);
}
inline INT_PTR CALLBACK ActivationPreferencesProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    INT_PTR themed{};
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themed)) return themed;
    auto* state = reinterpret_cast<ActivationPreferencesState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        state = reinterpret_cast<ActivationPreferencesState*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        NativeTheme::Apply(dialog);
        LocalizeDialog(dialog, state->uiLanguage);
        CheckDlgButton(dialog, 701, state->startup ? BST_CHECKED : BST_UNCHECKED);
        const auto add = [&](int control, const std::wstring& label, unsigned int value) {
            auto row = SendDlgItemMessageW(dialog, control, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            SendDlgItemMessageW(dialog, control, CB_SETITEMDATA, row, value);
        };
        for (unsigned int modifiers = 1; modifiers <= 15; ++modifiers) if (modifiers & 11) {
            auto label = ActivationHotkeyLabel((modifiers << 8) | 'A');
            label.resize(label.size() - 2);
            add(702, label, modifiers);
        }
        for (unsigned int key = 'A'; key <= 'Z'; ++key) add(703, std::wstring(1, static_cast<wchar_t>(key)), key);
        for (unsigned int key = '0'; key <= '9'; ++key) add(703, std::wstring(1, static_cast<wchar_t>(key)), key);
        for (unsigned int key = 0x70; key <= 0x7A; ++key) add(703, L"F" + std::to_wstring(key - 0x6F), key);
        SelectActivationChoice(dialog, 702, state->hotkey >> 8);
        SelectActivationChoice(dialog, 703, state->hotkey & 255);
        SetDlgItemTextW(dialog, 705, state->diagnostic.c_str());
        if (!state->diagnostic.empty()) EnableWindow(GetDlgItem(dialog, IDOK), FALSE);
        return TRUE;
    }
    if (!state) return FALSE;
    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        if (id == 704) {
            SelectActivationChoice(dialog, 702, 1); SelectActivationChoice(dialog, 703, 'E'); return TRUE;
        }
        if (id == 706 || id == 707) {
            std::wstring error;
            const bool imported = id == 706;
            const bool ok = imported ? state->importProfile(error) : state->exportProfile(error);
            SetDlgItemTextW(dialog, 705, error.c_str());
            if (ok && imported) EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (id == IDOK) {
            const auto get = [&](int control) { return static_cast<unsigned int>(SendDlgItemMessageW(dialog, control, CB_GETITEMDATA,
                SendDlgItemMessageW(dialog, control, CB_GETCURSEL, 0, 0), 0)); };
            std::wstring error;
            if (state->apply((get(702) << 8) | get(703), IsDlgButtonChecked(dialog, 701) == BST_CHECKED, error)) EndDialog(dialog, IDOK);
            else SetDlgItemTextW(dialog, 705, error.c_str());
            return TRUE;
        }
        if (id == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    }
    if (message == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    return FALSE;
}
inline bool ShowActivationPreferences(HWND owner, HINSTANCE instance, ActivationPreferencesState& state) {
    return DialogBoxParamW(instance, MAKEINTRESOURCEW(700), owner, ActivationPreferencesProc, reinterpret_cast<LPARAM>(&state)) != -1;
}
}
