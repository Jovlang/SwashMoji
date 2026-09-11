#pragma once
#include "personalization.h"
#include "catalog.h"
#include "native_theme.h"
#include <functional>

namespace SwashMoji {
inline constexpr int kLanguageDialog = 600;
inline constexpr int kPrimaryLanguage = 601;
inline constexpr int kSecondaryLanguage = 602;
inline constexpr int kLanguageStatus = 603;

struct LanguagePreferencesState {
    Profile& profile;
    std::function<bool()> persist;
};

inline int SelectedLocale(HWND dialog, int control) {
    const auto selection = SendDlgItemMessageW(dialog, control, CB_GETCURSEL, 0, 0);
    return selection == CB_ERR ? -1 : static_cast<int>(SendDlgItemMessageW(dialog, control, CB_GETITEMDATA, selection, 0));
}

inline void FillLanguageChoices(HWND dialog, int control, int selected, int excluded = -1) {
    SendDlgItemMessageW(dialog, control, CB_RESETCONTENT, 0, 0);
    const auto add = [&](const wchar_t* label, int locale) {
        const auto row = SendDlgItemMessageW(dialog, control, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label));
        SendDlgItemMessageW(dialog, control, CB_SETITEMDATA, row, locale);
        if (locale == selected) SendDlgItemMessageW(dialog, control, CB_SETCURSEL, row, 0);
    };
    if (control == kSecondaryLanguage) add(L"None", -1);
    const auto& locales = SupportedLocales();
    for (size_t i = 0; i < locales.size(); ++i) if (static_cast<int>(i) != excluded) add(locales[i].label, static_cast<int>(i));
    if (SendDlgItemMessageW(dialog, control, CB_GETCURSEL, 0, 0) == CB_ERR)
        SendDlgItemMessageW(dialog, control, CB_SETCURSEL, 0, 0);
}

inline bool ApplyLanguageChoices(HWND dialog, LanguagePreferencesState& state) {
    const int primary = SelectedLocale(dialog, kPrimaryLanguage), secondary = SelectedLocale(dialog, kSecondaryLanguage);
    const auto& locales = SupportedLocales();
    if (primary < 0 || static_cast<size_t>(primary) >= locales.size()) return false;
    std::vector<std::string> selected{locales[primary].code};
    if (secondary >= 0 && static_cast<size_t>(secondary) < locales.size()) selected.push_back(locales[secondary].code);
    if (!state.profile.settings.displayLanguages.Set(selected)) return false;
    if (state.persist()) return true;
    SetDlgItemTextW(dialog, kLanguageStatus, L"Applied for this session, but could not save. Try Save again.");
    return false;
}

inline INT_PTR CALLBACK LanguagePreferencesProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    INT_PTR themed{};
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themed)) return themed;
    auto* state = reinterpret_cast<LanguagePreferencesState*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        state = reinterpret_cast<LanguagePreferencesState*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        NativeTheme::Apply(dialog);
        const auto& selected = state->profile.settings.displayLanguages.Locales();
        const auto index = [&](const std::string& code) {
            const auto& locales = SupportedLocales();
            for (size_t i = 0; i < locales.size(); ++i) if (code == locales[i].code) return static_cast<int>(i);
            return -1;
        };
        const int primary = index(selected[0]);
        FillLanguageChoices(dialog, kPrimaryLanguage, primary);
        FillLanguageChoices(dialog, kSecondaryLanguage, selected.size() == 2 ? index(selected[1]) : -1, primary);
        return TRUE;
    }
    if (!state) return FALSE;
    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        if (id == kPrimaryLanguage && HIWORD(wParam) == CBN_SELCHANGE) {
            FillLanguageChoices(dialog, kSecondaryLanguage, SelectedLocale(dialog, kSecondaryLanguage), SelectedLocale(dialog, kPrimaryLanguage));
            return TRUE;
        }
        if (id == IDOK) { if (ApplyLanguageChoices(dialog, *state)) EndDialog(dialog, IDOK); return TRUE; }
        if (id == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    }
    if (message == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    // PMv2 dialog management scales the resource controls/fonts on DPI changes.
    return FALSE;
}

inline bool ShowLanguagePreferences(HWND owner, HINSTANCE instance, Profile& profile, const std::function<bool()>& persist) {
    LanguagePreferencesState state{profile, persist};
    return DialogBoxParamW(instance, MAKEINTRESOURCEW(kLanguageDialog), owner, LanguagePreferencesProc,
                           reinterpret_cast<LPARAM>(&state)) != -1;
}
}
