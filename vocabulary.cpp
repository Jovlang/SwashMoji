#include "vocabulary.h"
#include "vocabulary_ids.h"
#include "edit_controls.h"
#include <algorithm>

namespace SwashMoji {
namespace {
struct Editor {
    const Catalog& catalog;
    Profile& profile;
    std::wstring phrase;
    ResultId target;
    const std::function<bool()>& persist;
    std::wstring original;
    std::vector<std::wstring> aliasKeys;
    std::vector<SearchResult> results;
    bool loading{};
};

std::wstring Text(HWND dialog, int id) {
    const auto control = GetDlgItem(dialog, id);
    std::wstring value(GetWindowTextLengthW(control) + 1, L'\0');
    value.resize(GetWindowTextW(control, value.data(), static_cast<int>(value.size())));
    return value;
}

void Status(HWND dialog, const wchar_t* message) { SetDlgItemTextW(dialog, IDC_VOCABULARY_STATUS, message); }

void PinButtons(HWND dialog, const Editor& editor) {
    const auto selected = SendDlgItemMessageW(dialog, IDC_PINS, LB_GETCURSEL, 0, 0);
    const bool valid = selected >= 0 && static_cast<size_t>(selected) < editor.profile.pins.size();
    EnableWindow(GetDlgItem(dialog, IDC_PIN_UP), valid && selected > 0);
    EnableWindow(GetDlgItem(dialog, IDC_PIN_DOWN), valid && static_cast<size_t>(selected + 1) < editor.profile.pins.size());
    EnableWindow(GetDlgItem(dialog, IDC_UNPIN), valid);
}

void RefreshPins(HWND dialog, Editor& editor, const ResultId& selected = {}) {
    SendDlgItemMessageW(dialog, IDC_PINS, LB_RESETCONTENT, 0, 0);
    size_t selection = 0;
    for (size_t i = 0; i < editor.profile.pins.size(); ++i) {
        const auto& id = editor.profile.pins[i];
        const auto* emoji = id.kind == ResultKind::Emoji ? editor.catalog.FindFamily({id.value}) : nullptr;
        const auto label = std::to_wstring(i + 1) + L". " + (emoji ? emoji->glyph + L" " + emoji->name : L"Unavailable: " + id.value);
        SendDlgItemMessageW(dialog, IDC_PINS, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        if (id == selected) selection = i;
    }
    if (!editor.profile.pins.empty()) SendDlgItemMessageW(dialog, IDC_PINS, LB_SETCURSEL, selection, 0);
    PinButtons(dialog, editor);
}

void RefreshAliases(HWND dialog, Editor& editor) {
    auto list = GetDlgItem(dialog, IDC_ALIASES);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    editor.aliasKeys.clear();
    for (const auto& entry : editor.profile.aliases) {
        const auto index = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.second.phrase.c_str()));
        editor.aliasKeys.push_back(entry.first);
        if (entry.first == editor.original) SendMessageW(list, LB_SETCURSEL, index, 0);
    }
    EnableWindow(GetDlgItem(dialog, IDC_DELETE_ALIAS), !editor.original.empty());
}

void Preview(HWND dialog, Editor& editor) {
    const auto* emoji = editor.target.kind == ResultKind::Emoji ? editor.catalog.FindFamily({editor.target.value}) : nullptr;
    const auto label = emoji ? emoji->glyph + L"  " + emoji->name :
        (editor.target.value.empty() ? L"Choose an emoji above." : L"Unavailable target. Choose a replacement emoji.");
    SetDlgItemTextW(dialog, IDC_TARGET_PREVIEW, label.c_str());
    EnableWindow(GetDlgItem(dialog, IDC_PIN_TARGET), emoji != nullptr);
    SetDlgItemTextW(dialog, IDC_PIN_TARGET, IsPinned(editor.profile, editor.target) ? L"Unpin &favorite" : L"Pin &favorite");
}

void FindTargets(HWND dialog, Editor& editor) {
    Profile neutral;
    editor.results = Search(editor.catalog, neutral, Text(dialog, IDC_TARGET_QUERY));
    // Keep the native list responsive; a more specific query exposes the rest.
    if (editor.results.size() > 200) editor.results.resize(200);
    auto list = GetDlgItem(dialog, IDC_TARGET_RESULTS);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    HDC dc = GetDC(list);
    const auto font = reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0));
    const auto previous = font && dc ? SelectObject(dc, font) : nullptr;
    int textWidth = 0;
    for (const auto& result : editor.results) {
        const auto* emoji = editor.catalog.FindFamily({result.id.value});
        const auto label = result.payload + L"  " + result.label +
            (emoji && !emoji->nbName.empty() ? L" / " + emoji->nbName : L"");
        const auto index = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        if (result.id == editor.target) SendMessageW(list, LB_SETCURSEL, index, 0);
        SIZE extent{};
        if (dc && GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.size()), &extent))
            textWidth = std::max(textWidth, static_cast<int>(extent.cx) + 8);
    }
    if (previous) SelectObject(dc, previous);
    if (dc) ReleaseDC(list, dc);
    SendMessageW(list, LB_SETHORIZONTALEXTENT, textWidth, 0);
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, TRUE);
    Preview(dialog, editor);
    if (editor.results.empty()) Status(dialog, L"No emoji matches. Try a shorter name or another phrase.");
    else Status(dialog, L"Choose a result, then save. Close discards changes to this draft.");
}

void LoadDraft(HWND dialog, Editor& editor, const std::wstring& phrase, const ResultId& target) {
    editor.loading = true;
    editor.target = target;
    SetDlgItemTextW(dialog, IDC_PHRASE, phrase.c_str());
    const auto* emoji = target.kind == ResultKind::Emoji ? editor.catalog.FindFamily({target.value}) : nullptr;
    SetDlgItemTextW(dialog, IDC_TARGET_QUERY, emoji ? emoji->glyph.c_str() : L"");
    FindTargets(dialog, editor);
    RefreshAliases(dialog, editor);
    editor.loading = false;
}

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* editor = reinterpret_cast<Editor*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        editor = reinterpret_cast<Editor*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        SendDlgItemMessageW(dialog, IDC_PHRASE, EM_SETLIMITTEXT, kMaxAliasLength, 0);
        SendDlgItemMessageW(dialog, IDC_TARGET_QUERY, EM_SETLIMITTEXT, 255, 0);
        EnableWordDeletion(GetDlgItem(dialog, IDC_PHRASE));
        EnableWordDeletion(GetDlgItem(dialog, IDC_TARGET_QUERY));
        LoadDraft(dialog, *editor, editor->phrase, editor->target);
        RefreshPins(dialog, *editor);
        SetFocus(GetDlgItem(dialog, IDC_PHRASE));
        return FALSE;
    }
    if (!editor) return FALSE;
    if (message == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    if (message != WM_COMMAND) return FALSE;
    const int id = LOWORD(wParam), notification = HIWORD(wParam);
    if (id == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    if (editor->loading) return FALSE;
    if (id == IDC_PINS && notification == LBN_SELCHANGE) {
        PinButtons(dialog, *editor);
    } else if (id == IDC_PIN_TARGET) {
        if (IsPinned(editor->profile, editor->target)) Unpin(editor->profile, editor->target);
        else {
            const auto result = Pin(editor->profile, editor->catalog, editor->target);
            if (result == PinResult::LimitReached) { Status(dialog, L"You have ten favorites. Unpin one to make room."); return TRUE; }
            if (result != PinResult::Pinned) { Status(dialog, L"Choose an emoji to pin."); return TRUE; }
        }
        RefreshPins(dialog, *editor, editor->target);
        Preview(dialog, *editor);
        Status(dialog, editor->persist() ? L"Favorites saved." : L"Favorites not saved to disk. Changes remain in this session.");
    } else if (id == IDC_PIN_UP || id == IDC_PIN_DOWN || id == IDC_UNPIN) {
        const auto index = SendDlgItemMessageW(dialog, IDC_PINS, LB_GETCURSEL, 0, 0);
        if (index < 0 || static_cast<size_t>(index) >= editor->profile.pins.size()) return TRUE;
        const auto target = editor->profile.pins[index];
        const bool changed = id == IDC_UNPIN ? Unpin(editor->profile, target) : MovePin(editor->profile, target, id == IDC_PIN_UP ? -1 : 1);
        if (!changed) return TRUE;
        RefreshPins(dialog, *editor, target);
        Preview(dialog, *editor);
        Status(dialog, editor->persist() ? L"Favorites saved." : L"Favorites not saved to disk. Changes remain in this session.");
    } else if (id == IDC_NEW_ALIAS) {
        editor->original.clear();
        LoadDraft(dialog, *editor, L"", {});
        SetFocus(GetDlgItem(dialog, IDC_PHRASE));
    } else if (id == IDC_TARGET_QUERY && notification == EN_CHANGE) {
        editor->target = {};
        FindTargets(dialog, *editor);
    } else if (id == IDC_TARGET_RESULTS && notification == LBN_SELCHANGE) {
        const auto index = SendDlgItemMessageW(dialog, id, LB_GETCURSEL, 0, 0);
        if (index >= 0 && static_cast<size_t>(index) < editor->results.size()) editor->target = editor->results[index].id;
        Preview(dialog, *editor);
    } else if (id == IDC_ALIASES && notification == LBN_SELCHANGE) {
        const auto index = SendDlgItemMessageW(dialog, id, LB_GETCURSEL, 0, 0);
        if (index >= 0 && static_cast<size_t>(index) < editor->aliasKeys.size()) {
            editor->original = editor->aliasKeys[index];
            const auto alias = editor->profile.aliases.at(editor->original);
            LoadDraft(dialog, *editor, alias.phrase, alias.target);
        }
    } else if (id == IDC_SAVE_ALIAS) {
        const auto phrase = Text(dialog, IDC_PHRASE);
        auto result = SetAlias(editor->profile, editor->catalog, phrase, editor->target, editor->original);
        if (result == AliasResult::Duplicate) {
            if (MessageBoxW(dialog, L"This phrase already has an alias. Replace its emoji?", L"Replace alias",
                            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return TRUE;
            result = SetAlias(editor->profile, editor->catalog, phrase, editor->target, editor->original, true);
        }
        if (result == AliasResult::Saved) {
            editor->original = NormalizePhrase(phrase);
            RefreshAliases(dialog, *editor);
            Status(dialog, editor->persist() ? L"Alias saved." : L"Changes not saved to disk. Try Save alias again before closing.");
        } else if (result == AliasResult::InvalidPhrase) Status(dialog, L"Enter a phrase containing letters or numbers (up to 96 characters).");
        else if (result == AliasResult::InvalidTarget) Status(dialog, L"Choose an emoji to save this alias.");
        else if (result == AliasResult::LimitReached) Status(dialog, L"Your vocabulary has 500 aliases. Delete one to add another.");
        else Status(dialog, L"The original alias no longer exists. Choose New to save it again.");
    } else if (id == IDC_DELETE_ALIAS && !editor->original.empty()) {
        if (MessageBoxW(dialog, L"Delete this saved alias?", L"Delete alias", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
            DeleteAlias(editor->profile, editor->original);
            editor->original.clear();
            LoadDraft(dialog, *editor, L"", {});
            Status(dialog, editor->persist() ? L"Alias deleted." : L"Deletion not saved to disk. Changes remain in this session.");
        }
    } else return FALSE;
    return TRUE;
}
}

bool ShowVocabulary(HWND owner, HINSTANCE instance, const Catalog& catalog, Profile& profile,
                    const std::wstring& phrase, const ResultId& target, const std::function<bool()>& persist) {
    Editor editor{catalog, profile, phrase, target, persist};
    return DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_VOCABULARY), owner, DialogProc,
                          reinterpret_cast<LPARAM>(&editor)) != -1;
}
}
