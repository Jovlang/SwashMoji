#pragma once
#include <windows.h>

namespace SwashMoji {
// Adds Ctrl+Backspace to a standard Win32 EDIT without replacing its undo behavior.
void EnableWordDeletion(HWND edit);
}
