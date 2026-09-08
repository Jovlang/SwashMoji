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
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
            SendMessageW(g_edit, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            CHECK(g_session.selected == g_displayVisible[0].id);
            // After typing, every plain arrow still navigates the same grid.
            SetWindowTextW(g_edit, L"face");
            CHECK(g_visible.size() > 30);
            SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
            SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            SendMessageW(g_edit, WM_KEYDOWN, VK_DOWN, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
            SendMessageW(g_edit, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            CHECK(g_session.query == L"face");
            CHECK(g_session.selected == g_displayVisible[0].id);
            SendMessageW(g_list, WM_KEYDOWN, VK_DOWN, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
            SendMessageW(g_list, WM_KEYDOWN, VK_UP, 0);
            CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
            // Ctrl+all four arrows work in search and results, empty or typed.
            BYTE savedKeys[256]{}, controlKeys[256]{};
            CHECK(GetKeyboardState(savedKeys));
            controlKeys[VK_CONTROL] = 0x80;
            CHECK(SetKeyboardState(controlKeys));
            for (const auto* query : {L"", L"face"}) {
                SetWindowTextW(g_edit, query);
                SendMessageW(g_edit, EM_SETSEL, 0, 0);
                for (const auto control : {g_edit, g_list}) {
                    SendMessageW(g_list, LB_SETCURSEL, 0, 0);
                    SendMessageW(control, WM_KEYDOWN, VK_RIGHT, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == rows);
                    SendMessageW(control, WM_KEYDOWN, VK_LEFT, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
                    SendMessageW(control, WM_KEYDOWN, VK_DOWN, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
                    SendMessageW(control, WM_KEYDOWN, VK_UP, 0);
                    CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 0);
                    CHECK(g_session.query == query);
                }
                DWORD caret{};
                SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&caret), 0);
                CHECK(caret == 0);
            }
            CHECK(SetKeyboardState(savedKeys));
            SetWindowTextW(g_edit, L"");
        }
        g_emojiRows = 3;
        SetWindowTextW(g_edit, L"stone");
        CHECK(g_visible.size() > 1 && g_visible.size() <= 10);
        RECT compact{}, expanded{}, first{}, second{}, list{};
        GetWindowRect(g_window, &compact);
        GetClientRect(g_list, &list);
        CHECK(list.bottom == Px(kResultSize));
        CHECK(g_emojiRows == 3); // The preference survives automatic compaction.
        SendMessageW(g_list, LB_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&first));
        SendMessageW(g_list, LB_GETITEMRECT, 1, reinterpret_cast<LPARAM>(&second));
        CHECK(first.top == second.top && second.left > first.left);
        SendMessageW(g_edit, WM_KEYDOWN, VK_DOWN, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 1);
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == 2);
        const auto compactSelection = g_session.selected;
        RefreshList();
        CHECK(g_session.selected == compactSelection);
        SetWindowTextW(g_edit, L"face");
        GetWindowRect(g_window, &expanded);
        GetClientRect(g_list, &list);
        CHECK(list.bottom == Px(3 * kResultSize));
        CHECK(expanded.bottom - expanded.top == compact.bottom - compact.top + Px(2 * kResultSize));
        SendMessageW(g_edit, EM_SETSEL, 2, 2);
        const auto selected = SendMessageW(g_list, LB_GETCURSEL, 0, 0);
        // SetKeyboardState affects only this test thread, never desktop input.
        BYTE originalKeys[256]{}, modifiedKeys[256]{};
        CHECK(GetKeyboardState(originalKeys));
        modifiedKeys[VK_SHIFT] = 0x80;
        CHECK(SetKeyboardState(modifiedKeys));
        SendMessageW(g_edit, WM_KEYDOWN, VK_LEFT, 0);
        DWORD start{}, end{};
        SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
        CHECK(SetKeyboardState(originalKeys));
        CHECK(start == 1 && end == 2);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == selected);
        // Ctrl+Shift still selects words rather than moving the result.
        modifiedKeys[VK_SHIFT] = 0x80;
        modifiedKeys[VK_CONTROL] = 0x80;
        CHECK(SetKeyboardState(modifiedKeys));
        SendMessageW(g_edit, EM_SETSEL, 0, 0);
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        SendMessageW(g_edit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));
        CHECK(SetKeyboardState(originalKeys));
        CHECK(start == 0 && end == 4);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == selected);
        SendMessageW(g_edit, EM_SETSEL, 4, 4);
        SendMessageW(g_edit, WM_CHAR, L's', 0);
        CHECK(g_session.query == L"faces");
        // Clearing the query restores immediate navigation in the same session.
        SetWindowTextW(g_edit, L"");
        SendMessageW(g_edit, WM_KEYDOWN, VK_RIGHT, 0);
        CHECK(SendMessageW(g_list, LB_GETCURSEL, 0, 0) == g_emojiRows);
        CHECK(g_profile.history.empty());
        DestroyWindow(g_window);
        std::cout << "PASS: launch and typed-query arrows, all row counts, modified text editing and cleared search\n";
    } catch (const std::exception& error) {
        if (g_window) DestroyWindow(g_window);
        std::cerr << error.what() << '\n'; return 1;
    }
}
