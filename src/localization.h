#pragma once
#include <windows.h>
#include <string>

namespace SwashMoji {
std::wstring UiText(const std::string& locale, const wchar_t* english);
void LocalizeDialog(HWND dialog, const std::string& locale);
}
