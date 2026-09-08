// Exercise the actual child-window key handler without foreground activation,
// global hotkeys, synthetic desktop input, or the user's profile.
#define wWinMain SwashMojiUnusedApplicationMain
#include "../main.cpp"
#undef wWinMain
#include "test_support.h"

int main() {
    try {
        CHECK(LoadEmojis());
        g_uiFont = g_statusFont = g_emojiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_backgroundBrush = CreateSolidBrush(kBackground);
        g_inputBrush = CreateSolidBrush(kInputBackground);
        WNDCLASSW type{};
        type.hInstance = GetModuleHandleW(nullptr);
        type.lpszClassName = L"SwashMojiKeyboardTest";
        type.lpfnWndProc = WindowProc;
        CHECK(RegisterClassW(&type));
        g_window = CreateWindowExW(0, type.lpszClassName, L"Keyboard test", WS_POPUP,
            0, 0, kPickerWidth, PickerHeight(), nullptr, nullptr, type.hInstance, nullptr);
        CHECK(g_window);
        for (int rows = 1; rows <= 3; ++rows) {
            g_emojiRows = rows;
            BeginPickerSession();
            CHECK(g_visible.size() > 30);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            // Launch focuses search: no preliminary Tab or Down is required.
            SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
            SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            SendMessageW(g_edit, WM_KEYDOWN, VK_DOWN, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == (rows > 1 ? 1 : 0));
            SendMessageW(g_edit, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            CHECK(g_session.selected == g_displayVisible[0].id);
        }
        SetWindowTextW(g_edit, L"face");
        SendMessageW(g_edit, EM_SETSEL, 2, 2);
        const auto selected = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
        SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
        DWORD caret{};
        SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&caret), 0);
        CHECK(caret == 1);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == selected);
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&caret), 0);
        CHECK(caret == 2);
        // Clearing the query restores immediate navigation in the same session.
        SetWindowTextW(g_edit, L"");
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == g_emojiRows);
        CHECK(g_profile.history.empty());
        DestroyWindow(g_window);
        std::cout << "PASS: launch arrows, all row counts, query caret and cleared search\n";
    } catch (const std::exception& error) {
        if (g_window) DestroyWindow(g_window);
        std::cerr << error.what() << '\n'; return 1;
    }
}
