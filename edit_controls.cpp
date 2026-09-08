#include "edit_controls.h"
#include <commctrl.h>
#include <algorithm>
#include <string>
#include <vector>

namespace SwashMoji {
namespace {
bool SplitsSurrogate(const std::wstring& text, size_t position) {
    return position > 0 && position < text.size() &&
        text[position - 1] >= 0xD800 && text[position - 1] <= 0xDBFF &&
        text[position] >= 0xDC00 && text[position] <= 0xDFFF;
}

void DeletePreviousWord(HWND edit) {
    if (GetWindowLongPtrW(edit, GWL_STYLE) & ES_READONLY) return;
    std::wstring text(GetWindowTextLengthW(edit) + 1, L'\0');
    text.resize(GetWindowTextW(edit, text.data(), static_cast<int>(text.size())));
    DWORD first{}, last{};
    SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&first), reinterpret_cast<LPARAM>(&last));
    size_t start = std::min(static_cast<size_t>(first), text.size());
    size_t end = std::min(static_cast<size_t>(last), text.size());
    if (start == end) {
        std::vector<WORD> types(text.size());
        if (text.empty() || !GetStringTypeW(CT_CTYPE1, text.data(), static_cast<int>(text.size()), types.data())) return;
        // Delete trailing whitespace and the preceding token. Punctuation and
        // emoji sequences stay with their token; Norwegian letters remain intact.
        while (start && (types[start - 1] & C1_SPACE)) --start;
        while (start && !(types[start - 1] & C1_SPACE)) --start;
    }
    if (SplitsSurrogate(text, start)) --start;
    if (SplitsSurrogate(text, end)) ++end;
    if (start == end) return;
    SendMessageW(edit, EM_SETSEL, start, end);
    SendMessageW(edit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L""));
}

LRESULT CALLBACK EditSubclass(HWND edit, UINT message, WPARAM wParam, LPARAM lParam,
                               UINT_PTR id, DWORD_PTR) {
    // TranslateMessage emits DEL for Ctrl+Backspace. Consume it here so the
    // standard EDIT cannot insert a visible control character or delete twice.
    if (message == WM_CHAR && wParam == 0x7F) {
        DeletePreviousWord(edit);
        return 0;
    }
    if (message == WM_NCDESTROY) RemoveWindowSubclass(edit, EditSubclass, id);
    return DefSubclassProc(edit, message, wParam, lParam);
}
}

void EnableWordDeletion(HWND edit) { SetWindowSubclass(edit, EditSubclass, 1, 0); }
}
