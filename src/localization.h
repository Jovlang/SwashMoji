#pragma once
#include <windows.h>
#include <string>

namespace SwashMoji {
std::wstring UiText(const std::string& locale, const wchar_t* english);
// Only for application-owned diagnostics, including joined storage messages.
std::wstring UiDiagnostic(const std::string& locale, const std::wstring& english);
std::wstring UiHelpText(const std::string& locale);
std::wstring UiProfileFilter(const std::string& locale);
void LocalizeDialog(HWND dialog, const std::string& locale);
}
