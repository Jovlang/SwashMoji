#include "edit_controls.h"
#include "test_support.h"
#include <string>

using namespace SwashMoji;

std::wstring Text(HWND edit) {
    wchar_t text[256]{};
    GetWindowTextW(edit, text, 256);
    return text;
}

void CheckDeletion(HWND edit, const wchar_t* input, int first, int last, const wchar_t* expected, int caret) {
    SetWindowTextW(edit, input);
    SendMessageW(edit, EM_SETSEL, first, last);
    SendMessageW(edit, WM_CHAR, 0x7F, 1);
    CHECK(Text(edit) == expected);
    DWORD start{}, end{};
    SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
    CHECK(start == caret && end == caret);
}

int main() {
    HWND owner{}, edit{};
    try {
        owner = CreateWindowExW(0, L"STATIC", L"Edit tests", WS_POPUP, 0, 0, 300, 100, nullptr, nullptr, nullptr, nullptr);
        CHECK(owner);
        edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL, 0, 0, 300, 30, owner, nullptr, nullptr, nullptr);
        CHECK(edit);
        EnableWordDeletion(edit);
        CheckDeletion(edit, L"hello world", 11, 11, L"hello ", 6);
        CHECK(SendMessageW(edit, EM_CANUNDO, 0, 0));
        SendMessageW(edit, WM_UNDO, 0, 0);
        CHECK(Text(edit) == L"hello world");
        CheckDeletion(edit, L"hello world   ", 14, 14, L"hello ", 6);
        CheckDeletion(edit, L"hello world", 8, 8, L"hello rld", 6);
        CheckDeletion(edit, L"hello world", 2, 8, L"herld", 2);
        CheckDeletion(edit, L"på vei", 6, 6, L"på ", 3);
        CheckDeletion(edit, L"god\u00a0idé", 7, 7, L"god\u00a0", 4);
        CheckDeletion(edit, L"hei 👩‍💻", 9, 9, L"hei ", 4);
        CheckDeletion(edit, L"hei 👍🏽", 8, 8, L"hei ", 4);
        CheckDeletion(edit, L"hei 👍", 5, 5, L"hei ", 4); // Cannot leave half a surrogate pair.
        CheckDeletion(edit, L"hello, world!", 13, 13, L"hello, ", 7);
        CheckDeletion(edit, L"   ", 3, 3, L"", 0);
        CheckDeletion(edit, L"word", 0, 0, L"word", 0);
        CheckDeletion(edit, L"", 0, 0, L"", 0);
        SetWindowTextW(edit, L"one two three");
        SendMessageW(edit, EM_SETSEL, 13, 13);
        SendMessageW(edit, WM_CHAR, 0x7F, 1);
        SendMessageW(edit, WM_CHAR, 0x7F, 1);
        CHECK(Text(edit) == L"one ");
        SendMessageW(edit, WM_CHAR, VK_BACK, 1);
        CHECK(Text(edit) == L"one"); // Ordinary Backspace still deletes one character.
        SendMessageW(edit, EM_SETREADONLY, TRUE, 0);
        SendMessageW(edit, WM_CHAR, 0x7F, 1);
        CHECK(Text(edit) == L"one");
        DestroyWindow(owner);
        std::cout << "Ctrl+Backspace selection, caret, Unicode, repeat, undo and ordinary Backspace checks passed.\n";
    } catch (const std::exception& error) {
        if (owner) DestroyWindow(owner);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
