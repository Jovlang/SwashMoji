#include "vocabulary.h"
#include "vocabulary_ids.h"
#include "edit_controls.h"
#include "native_emoji.h"
#include "native_theme.h"
#include "vocabulary_style.h"
#include "combination_style.h"
#include "picker.h"
#include <algorithm>

namespace SwashMoji {
namespace {
bool EmojiControl(int id) {
    switch (id) {
    case IDC_COMBO_SAVED:
    case IDC_ALIASES:
    case IDC_PINS:
    case IDC_TARGET_RESULTS:
    case IDC_COMBO_RESULTS:
    case IDC_COMBO_VARIANTS:
    case IDC_COMBO_ENTRIES:
    case IDC_COMBO_DETAILS_PAYLOAD:
        return true;
    default:
        return false;
    }
}

bool MeasureEmojiControl(HWND dialog, LPARAM lParam) {
    auto* item = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
    if (!EmojiControl(item->CtlID)) return false;
    const int logicalHeight = (item->CtlID == IDC_TARGET_RESULTS || item->CtlID == IDC_COMBO_RESULTS) ? 48 : item->CtlID == IDC_ALIASES || item->CtlID == IDC_PINS || item->CtlID == IDC_COMBO_SAVED ? 36 : item->CtlID == IDC_COMBO_ENTRIES ? 26 : 20;
    item->itemHeight = MulDiv(logicalHeight, GetDpiForWindow(dialog), 96);
    return true;
}

std::wstring DrawItemText(const DRAWITEMSTRUCT& item) {
    if (item.CtlType == ODT_STATIC || item.itemID == static_cast<UINT>(-1)) {
        std::wstring text(GetWindowTextLengthW(item.hwndItem) + 1, L'\0');
        text.resize(GetWindowTextW(item.hwndItem, text.data(), static_cast<int>(text.size())));
        return text;
    }
    const UINT message = item.CtlType == ODT_COMBOBOX ? CB_GETLBTEXT : LB_GETTEXT;
    const UINT lengthMessage = item.CtlType == ODT_COMBOBOX ? CB_GETLBTEXTLEN : LB_GETTEXTLEN;
    const auto length = SendMessageW(item.hwndItem, lengthMessage, item.itemID, 0);
    if (length < 0) return {};
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    SendMessageW(item.hwndItem, message, item.itemID, reinterpret_cast<LPARAM>(text.data()));
    text.resize(static_cast<size_t>(length));
    return text;
}

bool DrawEmojiControl(HWND dialog, LPARAM lParam) {
    auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    if (!EmojiControl(item->CtlID)) return false;
    static thread_local bool buffering = false;
    if (!buffering && !NativeTheme::HighContrast()) {
        HDC bufferedDc{};
        const auto buffer = BeginBufferedPaint(item->hDC, &item->rcItem, BPBF_COMPATIBLEBITMAP, nullptr, &bufferedDc);
        if (buffer) {
            auto bufferedItem = *item;
            bufferedItem.hDC = bufferedDc;
            buffering = true;
            const auto result = DrawEmojiControl(dialog, reinterpret_cast<LPARAM>(&bufferedItem));
            buffering = false;
            EndBufferedPaint(buffer, TRUE);
            return result;
        }
    }
    if (GetDlgItem(dialog,IDC_COMBO_EDITOR) && item->CtlID == IDC_COMBO_ENTRIES) {
        auto r=item->rcItem; FillRect(item->hDC,&r,VocabularyStyle::InputBrush());
        auto text=DrawItemText(*item);
        if(item->itemID!=static_cast<UINT>(-1)) {
            auto start=text.find(L". ");
            if(start!=std::wstring::npos) text=text.substr(start+2);
            auto end=text.find(L"  "); if(end!=std::wstring::npos) text.resize(end);
            InflateRect(&r,-VocabularyStyle::Px(dialog,3),-VocabularyStyle::Px(dialog,3));
            const bool selected=(item->itemState&ODS_SELECTED)!=0 ||
                (item->itemID != static_cast<UINT>(-1) &&
                 SendMessageW(item->hwndItem, LB_GETCURSEL, 0, 0) == static_cast<LRESULT>(item->itemID));
            const bool hot=reinterpret_cast<UINT_PTR>(GetPropW(item->hwndItem,L"VocabularyHotRow"))==item->itemID+1;
            VocabularyStyle::Round(item->hDC,r,NativeTheme::HighContrast() ? GetSysColor(selected?COLOR_HIGHLIGHT:COLOR_WINDOW) :
                selected ? RGB(48,65,88) : hot ? RGB(53,58,66) : RGB(45,48,54),VocabularyStyle::Px(dialog,8));
        }
        auto color=NativeTheme::HighContrast() && (item->itemState&ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : NativeTheme::Foreground();
        NativeEmoji::DrawLine(item->hDC,r,text,static_cast<float>(VocabularyStyle::Px(dialog,30)),color,true,!NativeTheme::HighContrast(),true);
        return true;
    }
    if (item->CtlID == IDC_COMBO_SAVED || item->CtlID == IDC_COMBO_RESULTS || item->CtlID == IDC_ALIASES || item->CtlID == IDC_PINS || item->CtlID == IDC_TARGET_RESULTS) {
        auto r = item->rcItem;
        FillRect(item->hDC, &r, VocabularyStyle::InputBrush());
        const bool selected = (item->itemState & ODS_SELECTED) != 0 ||
            (item->itemID != static_cast<UINT>(-1) &&
             SendMessageW(item->hwndItem, LB_GETCURSEL, 0, 0) == static_cast<LRESULT>(item->itemID));
        const bool hot = item->itemID != static_cast<UINT>(-1) && reinterpret_cast<UINT_PTR>(GetPropW(item->hwndItem,L"VocabularyHotRow")) == item->itemID+1;
        if (selected || hot) {
            InflateRect(&r, -VocabularyStyle::Px(dialog,3), -VocabularyStyle::Px(dialog,2));
            VocabularyStyle::Round(item->hDC,r,NativeTheme::HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : (selected ? RGB(48,65,88) : RGB(48,52,59)),VocabularyStyle::Px(dialog,6));
        }
        const auto color = selected && NativeTheme::HighContrast() ? GetSysColor(COLOR_HIGHLIGHTTEXT) : NativeTheme::Foreground();
        auto text = DrawItemText(*item);
        r = item->rcItem; InflateRect(&r,-VocabularyStyle::Px(dialog,10),0);
        if (item->CtlID == IDC_COMBO_RESULTS || item->CtlID == IDC_TARGET_RESULTS) {
            const auto split = text.find(L"  ");
            if (split != std::wstring::npos) {
                auto glyph = r; glyph.right = glyph.left + VocabularyStyle::Px(dialog,44);
                NativeEmoji::DrawLine(item->hDC,glyph,text.substr(0,split),static_cast<float>(VocabularyStyle::Px(dialog,26)),color,false,!NativeTheme::HighContrast(),true);
                r.left = glyph.right + VocabularyStyle::Px(dialog,8);
                text = text.substr(split+2);
                const auto translation = text.find(kEmojiNameSeparator);
                const int blockHeight = VocabularyStyle::Px(dialog,40);
                r.top += std::max(0L,(r.bottom-r.top-blockHeight)/2); r.bottom = r.top+blockHeight;
                auto title = r; title.bottom = title.top + blockHeight/2;
                auto secondary = r; secondary.top = title.bottom;
                NativeEmoji::DrawLine(item->hDC,translation==std::wstring::npos ? r : title,text.substr(0,translation),static_cast<float>(VocabularyStyle::Px(dialog,14)),color,false,!NativeTheme::HighContrast(),true);
                if (translation != std::wstring::npos)
                    NativeEmoji::DrawLine(item->hDC,secondary,text.substr(translation+3),static_cast<float>(VocabularyStyle::Px(dialog,12)), selected && NativeTheme::HighContrast() ? color : NativeTheme::SecondaryText(),false,!NativeTheme::HighContrast(),true);
            } else NativeEmoji::DrawLine(item->hDC,r,text,static_cast<float>(VocabularyStyle::Px(dialog,13)),NativeTheme::SecondaryText(),false,!NativeTheme::HighContrast(),true);
        } else NativeEmoji::DrawLine(item->hDC,r,text,static_cast<float>(VocabularyStyle::Px(dialog,14)),color,false,!NativeTheme::HighContrast(),true);
        return true;
    }
    const bool selected = (item->itemState & ODS_SELECTED) != 0;
    HBRUSH fill = selected
        ? CreateSolidBrush(NativeTheme::HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : RGB(42, 86, 128))
        : NativeTheme::SurfaceBrush();
    FillRect(item->hDC, &item->rcItem, fill);
    if (selected) DeleteObject(fill);
    const auto color = selected && NativeTheme::HighContrast()
        ? GetSysColor(COLOR_HIGHLIGHTTEXT) : NativeTheme::Foreground();
    const bool preview = item->CtlID == IDC_COMBO_DETAILS_PAYLOAD;
    const float size = static_cast<float>(MulDiv(preview ? 22 : 14, GetDpiForWindow(dialog), 96));
    NativeEmoji::DrawLine(item->hDC, item->rcItem, DrawItemText(*item), size, color, preview,
                          !NativeTheme::HighContrast());
    if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
    return true;
}

void StyleHeading(HWND dialog, int id) {
    const auto control = GetDlgItem(dialog, id);
    const auto base = reinterpret_cast<HFONT>(SendMessageW(control, WM_GETFONT, 0, 0));
    LOGFONTW description{};
    if (!base || !GetObjectW(base, sizeof(description), &description)) return;
    description.lfWeight = FW_SEMIBOLD;
    const auto font = CreateFontIndirectW(&description);
    if (!font) return;
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    SetPropW(control, L"SwashMojiHeadingFont", font);
}

void ReleaseHeading(HWND dialog, int id) {
    const auto control = GetDlgItem(dialog, id);
    if (const auto font = RemovePropW(control, L"SwashMojiHeadingFont")) DeleteObject(font);
}

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

void EditCombinations(HWND dialog, Editor& editor);

void Status(HWND dialog, const wchar_t* message) { SetDlgItemTextW(dialog, IDC_VOCABULARY_STATUS, message); }

void PinButtons(HWND dialog, const Editor& editor) {
    const auto selected = SendDlgItemMessageW(dialog, IDC_PINS, LB_GETCURSEL, 0, 0);
    const bool valid = selected >= 0 && static_cast<size_t>(selected) < editor.profile.pins.size();
    EnableWindow(GetDlgItem(dialog, IDC_PIN_UP), valid && selected > 0);
    EnableWindow(GetDlgItem(dialog, IDC_PIN_DOWN), valid && static_cast<size_t>(selected + 1) < editor.profile.pins.size());
    EnableWindow(GetDlgItem(dialog, IDC_UNPIN), valid);
}

void RefreshPins(HWND dialog, Editor& editor, const ResultId& selected = {}) {
    const auto list = GetDlgItem(dialog, IDC_PINS);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    size_t selection = 0;
    for (size_t i = 0; i < editor.profile.pins.size(); ++i) {
        const auto& id = editor.profile.pins[i];
        SearchResult result;
        const bool valid = ResolveResult(editor.catalog, editor.profile, id, result);
        const auto label = std::to_wstring(i + 1) + L". " + (valid ? result.payload + L" " + result.label : L"Unavailable: " + id.value);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
        if (id == selected) selection = i;
    }
    if (!editor.profile.pins.empty()) SendMessageW(list, LB_SETCURSEL, selection, 0);
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, FALSE);
    PinButtons(dialog, editor);
}

void RefreshAliases(HWND dialog, Editor& editor) {
    auto list = GetDlgItem(dialog, IDC_ALIASES);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    editor.aliasKeys.clear();
    for (const auto& entry : editor.profile.aliases) {
        const auto index = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.second.phrase.c_str()));
        editor.aliasKeys.push_back(entry.first);
        if (entry.first == editor.original) SendMessageW(list, LB_SETCURSEL, index, 0);
    }
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, FALSE);
    EnableWindow(GetDlgItem(dialog, IDC_DELETE_ALIAS), !editor.original.empty());
}

void TargetButtons(HWND dialog, Editor& editor) {
    SearchResult result;
    const bool valid = ResolveResult(editor.catalog, editor.profile, editor.target, result);
    EnableWindow(GetDlgItem(dialog, IDC_PIN_TARGET), valid);
    SetDlgItemTextW(dialog, IDC_PIN_TARGET, IsPinned(editor.profile, editor.target) ? L"Unpin &favorite" : L"Pin &favorite");
}

void FindTargets(HWND dialog, Editor& editor) {
    Profile neutral;
    neutral.combinations = editor.profile.combinations;
    neutral.settings.displayLanguages = editor.profile.settings.displayLanguages;
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
        const auto label = result.payload + L"  " + result.label;
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
    InvalidateRect(list, nullptr, FALSE);
    TargetButtons(dialog, editor);
    if (editor.results.empty()) Status(dialog, L"No emoji matches. Try a shorter name or another phrase.");
    else Status(dialog, L"");
}

void LoadDraft(HWND dialog, Editor& editor, const std::wstring& phrase, const ResultId& target) {
    editor.loading = true;
    editor.target = target;
    SetDlgItemTextW(dialog, IDC_PHRASE, phrase.c_str());
    const auto* emoji = target.kind == ResultKind::Emoji ? editor.catalog.FindFamily({target.value}) : nullptr;
    const auto combination = target.kind == ResultKind::Combination ? editor.profile.combinations.find(target.value) : editor.profile.combinations.end();
    SetDlgItemTextW(dialog, IDC_TARGET_QUERY, emoji ? GetBestEmojiName(*emoji).c_str() :
        combination != editor.profile.combinations.end() ? combination->second.name.c_str() : L"");
    FindTargets(dialog, editor);
    RefreshAliases(dialog, editor);
    editor.loading = false;
    if (editor.original.empty() && !phrase.empty()) Status(dialog,L"Unsaved changes");
}

INT_PTR CALLBACK DialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DPICHANGED) PostMessageW(dialog,WM_APP+71,0,0);
    if (message == WM_APP+71) { VocabularyStyle::Layout(dialog); return TRUE; }
    if (message == WM_PAINT) { VocabularyStyle::Paint(dialog); return TRUE; }
    if (message == WM_SIZE && GetDlgItem(dialog,IDC_PHRASE)) { VocabularyStyle::Layout(dialog); return TRUE; }
    if (message == WM_GETMINMAXINFO) {
        RECT r{0,0,552,432}; MapDialogRect(dialog,&r);
        AdjustWindowRectExForDpi(&r,static_cast<DWORD>(GetWindowLongPtrW(dialog,GWL_STYLE)),FALSE,static_cast<DWORD>(GetWindowLongPtrW(dialog,GWL_EXSTYLE)),GetDpiForWindow(dialog));
        auto* info=reinterpret_cast<MINMAXINFO*>(lParam); info->ptMinTrackSize={r.right-r.left,r.bottom-r.top}; return TRUE;
    }
    if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLORSTATIC) {
        auto dc=reinterpret_cast<HDC>(wParam); auto control=reinterpret_cast<HWND>(lParam);
        SetTextColor(dc,GetPropW(control,L"SwashMojiMuted") ? NativeTheme::SecondaryText() : NativeTheme::Foreground());
        SetBkMode(dc,TRANSPARENT); SetBkColor(dc,VocabularyStyle::Input());
        return reinterpret_cast<INT_PTR>(message != WM_CTLCOLORSTATIC ? VocabularyStyle::InputBrush() : GetDlgCtrlID(control)==IDC_VOCABULARY_INTRO ? NativeTheme::BackgroundBrush() : VocabularyStyle::PanelBrush());
    }
    INT_PTR themeResult{};
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themeResult)) return themeResult;
    if (message == WM_MEASUREITEM && MeasureEmojiControl(dialog, lParam)) return TRUE;
    if (message == WM_DRAWITEM && DrawEmojiControl(dialog, lParam)) return TRUE;
    auto* editor = reinterpret_cast<Editor*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        editor = reinterpret_cast<Editor*>(lParam);
        SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        SendDlgItemMessageW(dialog, IDC_PHRASE, EM_SETLIMITTEXT, kMaxAliasLength, 0);
        SendDlgItemMessageW(dialog, IDC_TARGET_QUERY, EM_SETLIMITTEXT, 255, 0);
        EnableWordDeletion(GetDlgItem(dialog, IDC_PHRASE));
        EnableWordDeletion(GetDlgItem(dialog, IDC_TARGET_QUERY));
        NativeTheme::Apply(dialog);
        VocabularyStyle::Apply(dialog);
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_VOCABULARY_INTRO));
        StyleHeading(dialog, IDC_ALIAS_HEADING);
        StyleHeading(dialog, IDC_ALIASES_HEADING);
        StyleHeading(dialog, IDC_PINS_HEADING);
        NativeTheme::ApplyEmojiFont(dialog, {IDC_ALIASES, IDC_PINS, IDC_TARGET_QUERY,
                                             IDC_TARGET_RESULTS});
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_VOCABULARY_STATUS));
        LoadDraft(dialog, *editor, editor->phrase, editor->target);
        RefreshPins(dialog, *editor);
        SetFocus(GetDlgItem(dialog, IDC_PHRASE));
        return FALSE;
    }
    if (!editor) return FALSE;
    if (message == WM_DESTROY) {
        ReleaseHeading(dialog, IDC_ALIAS_HEADING);
        ReleaseHeading(dialog, IDC_ALIASES_HEADING);
        ReleaseHeading(dialog, IDC_PINS_HEADING);
        NativeTheme::ReleaseEmojiFont(dialog);
        return FALSE;
    }
    if (message == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    if (message != WM_COMMAND) return FALSE;
    const int id = LOWORD(wParam), notification = HIWORD(wParam);
    if (id == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    if (editor->loading) return FALSE;
    if (id == IDC_COMBINATIONS) {
        EditCombinations(dialog, *editor);
        RefreshAliases(dialog, *editor); RefreshPins(dialog, *editor); FindTargets(dialog, *editor);
    } else if (id == IDC_PINS && notification == LBN_SELCHANGE) {
        PinButtons(dialog, *editor);
    } else if (id == IDC_PIN_TARGET) {
        if (IsPinned(editor->profile, editor->target)) Unpin(editor->profile, editor->target);
        else {
            const auto result = Pin(editor->profile, editor->catalog, editor->target);
            if (result == PinResult::LimitReached) { Status(dialog, L"You have ten favorites. Unpin one to make room."); return TRUE; }
            if (result != PinResult::Pinned) { Status(dialog, L"Choose an emoji to pin."); return TRUE; }
        }
        RefreshPins(dialog, *editor, editor->target);
        TargetButtons(dialog, *editor);
        Status(dialog, editor->persist() ? L"Favorites saved." : L"Favorites not saved to disk. Changes remain in this session.");
    } else if (id == IDC_PIN_UP || id == IDC_PIN_DOWN || id == IDC_UNPIN) {
        const auto index = SendDlgItemMessageW(dialog, IDC_PINS, LB_GETCURSEL, 0, 0);
        if (index < 0 || static_cast<size_t>(index) >= editor->profile.pins.size()) return TRUE;
        const auto target = editor->profile.pins[index];
        const bool changed = id == IDC_UNPIN ? Unpin(editor->profile, target) : MovePin(editor->profile, target, id == IDC_PIN_UP ? -1 : 1);
        if (!changed) return TRUE;
        RefreshPins(dialog, *editor, target);
        TargetButtons(dialog, *editor);
        Status(dialog, editor->persist() ? L"Favorites saved." : L"Favorites not saved to disk. Changes remain in this session.");
    } else if (id == IDC_NEW_ALIAS) {
        editor->original.clear();
        LoadDraft(dialog, *editor, L"", {});
        SetFocus(GetDlgItem(dialog, IDC_PHRASE));
    } else if (id == IDC_PHRASE && notification == EN_CHANGE) {
        Status(dialog, L"Unsaved changes");
    } else if (id == IDC_TARGET_QUERY && notification == EN_CHANGE) {
        editor->target = {};
        FindTargets(dialog, *editor);
    } else if (id == IDC_TARGET_RESULTS && notification == LBN_SELCHANGE) {
        const auto index = SendDlgItemMessageW(dialog, id, LB_GETCURSEL, 0, 0);
        if (index >= 0 && static_cast<size_t>(index) < editor->results.size()) editor->target = editor->results[index].id;
        TargetButtons(dialog, *editor);
        Status(dialog, L"Unsaved changes");
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
        } else if (result == AliasResult::InvalidPhrase) Status(dialog, L"Enter a unique phrase (up to 96 characters), distinct from combination names.");
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

struct CombinationEditor {
    Editor& parent;
    Combination draft;
    std::vector<std::wstring> keys;
    std::vector<SearchResult> results;
    std::vector<const Emoji*> variants;
    bool loading{};
};
void ComboStatus(HWND dialog, const std::wstring& text) { SetDlgItemTextW(dialog, IDC_COMBO_STATUS, text.c_str()); }
void Sequence(HWND dialog, const Catalog& catalog, const Combination& c, int selection = 0, const DisplayLanguages& languages = {}) {
    const auto list = GetDlgItem(dialog, IDC_COMBO_ENTRIES);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < c.entries.size(); ++i) {
        const auto& entry = c.entries[i];
        const auto* emoji = catalog.Find(entry.payload);
        const auto label = std::to_wstring(i + 1) + L". " + entry.payload + (emoji ? L"  " + FormatEmojiDisplayName(*emoji, languages) : L"");
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    SendMessageW(list, LB_SETCURSEL, selection, 0);
    if (!GetDlgItem(dialog,IDC_COMBO_EDITOR)) SendMessageW(list, LB_SETHORIZONTALEXTENT, 1000, 0);
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, FALSE);
}
void ComboButtons(HWND dialog, const CombinationEditor& e) {
    const auto selected = SendDlgItemMessageW(dialog, IDC_COMBO_ENTRIES, LB_GETCURSEL, 0, 0);
    EnableWindow(GetDlgItem(dialog, IDC_COMBO_REMOVE), selected >= 0);
    EnableWindow(GetDlgItem(dialog, IDC_COMBO_LEFT), selected > 0);
    EnableWindow(GetDlgItem(dialog, IDC_COMBO_RIGHT), selected >= 0 && static_cast<size_t>(selected + 1) < e.draft.entries.size());
    EnableWindow(GetDlgItem(dialog, IDC_COMBO_ADD), !e.variants.empty() && e.draft.entries.size() < 8);
    EnableWindow(GetDlgItem(dialog, IDC_COMBO_DELETE), !e.draft.id.empty());
}
void ComboSaved(HWND dialog, CombinationEditor& e) {
    const auto list = GetDlgItem(dialog, IDC_COMBO_SAVED);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0); e.keys.clear();
    for (const auto& item : e.parent.profile.combinations) {
        const auto index = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.second.name.c_str()));
        e.keys.push_back(item.first);
        if (item.first == e.draft.id) SendMessageW(list, LB_SETCURSEL, index, 0);
    }
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, FALSE);
    const auto heading = L"&Saved combinations (" + std::to_wstring(e.keys.size()) + L")";
    SetDlgItemTextW(dialog, IDC_COMBO_SAVED_HEADING, heading.c_str());
    ComboButtons(dialog, e);
}
void ComboVariants(HWND dialog, CombinationEditor& e) {
    const auto combo = GetDlgItem(dialog, IDC_COMBO_VARIANTS);
    SendMessageW(combo, WM_SETREDRAW, FALSE, 0);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0); e.variants.clear();
    const auto selected = SendDlgItemMessageW(dialog, IDC_COMBO_RESULTS, LB_GETCURSEL, 0, 0);
    if (selected >= 0 && static_cast<size_t>(selected) < e.results.size()) {
        e.variants = CatalogVariants(e.parent.catalog, e.results[selected].id);
        int choice = 0;
        for (size_t i = 0; i < e.variants.size(); ++i) {
            const auto label = e.variants[i]->glyph + L"  " + FormatEmojiDisplayName(*e.variants[i], e.parent.profile.settings.displayLanguages);
            SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
            if (e.variants[i]->glyph == e.results[selected].payload) choice = static_cast<int>(i);
        }
        SendMessageW(combo, CB_SETCURSEL, choice, 0);
        SendMessageW(combo, CB_SETDROPPEDWIDTH, 600, 0);
    }
    SendMessageW(combo, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(combo, nullptr, FALSE);
    const int variantVisibility = e.variants.size() > 1 ? SW_SHOW : SW_HIDE;
    ShowWindow(GetDlgItem(dialog, IDC_COMBO_VARIANT_LABEL), variantVisibility);
    ShowWindow(GetDlgItem(dialog, IDC_COMBO_VARIANTS), variantVisibility);
    InvalidateRect(dialog, nullptr, FALSE);
    ComboButtons(dialog, e);
}
void ComboSearch(HWND dialog, CombinationEditor& e) {
    Profile neutral;
    neutral.settings.displayLanguages = e.parent.profile.settings.displayLanguages;
    e.results = Search(e.parent.catalog, neutral, Text(dialog, IDC_COMBO_QUERY));
    if (e.results.size() > 200) e.results.resize(200);
    const auto list = GetDlgItem(dialog, IDC_COMBO_RESULTS);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (const auto& result : e.results) {
        const auto label = result.payload + L"  " + result.label;
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    SendMessageW(list, LB_SETCURSEL, 0, 0);
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, FALSE);
    ComboVariants(dialog, e);
}
INT_PTR CALLBACK ConfirmCombinationDelete(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    INT_PTR themeResult{};
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themeResult)) return themeResult;
    if (message == WM_INITDIALOG) {
        NativeTheme::Apply(dialog);
        SetDlgItemTextW(dialog, IDC_COMBO_DEPENDENTS, reinterpret_cast<const wchar_t*>(lParam));
        SetFocus(GetDlgItem(dialog, IDCANCEL)); return FALSE;
    }
    if (message == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
        EndDialog(dialog, LOWORD(wParam)); return TRUE;
    }
    if (message == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    return FALSE;
}
INT_PTR CALLBACK CombinationProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    INT_PTR themeResult{};
    if (CombinationStyle::Message(dialog, message, wParam, lParam, themeResult)) return themeResult;
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themeResult)) return themeResult;
    if (message == WM_MEASUREITEM && MeasureEmojiControl(dialog, lParam)) return TRUE;
    if (message == WM_DRAWITEM && DrawEmojiControl(dialog, lParam)) return TRUE;
    auto* e = reinterpret_cast<CombinationEditor*>(GetWindowLongPtrW(dialog, DWLP_USER));
    if (message == WM_INITDIALOG) {
        e = reinterpret_cast<CombinationEditor*>(lParam); SetWindowLongPtrW(dialog, DWLP_USER, lParam);
        SendDlgItemMessageW(dialog, IDC_COMBO_NAME, EM_SETLIMITTEXT, kMaxAliasLength, 0);
        SendDlgItemMessageW(dialog, IDC_COMBO_QUERY, EM_SETLIMITTEXT, 255, 0);
        EnableWordDeletion(GetDlgItem(dialog, IDC_COMBO_NAME)); EnableWordDeletion(GetDlgItem(dialog, IDC_COMBO_QUERY));
        NativeTheme::Apply(dialog);
        StyleHeading(dialog, IDC_COMBO_SAVED_HEADING);
        StyleHeading(dialog, IDC_COMBO_EDITOR);
        StyleHeading(dialog, IDC_COMBO_ADD_HEADING);
        StyleHeading(dialog, IDC_COMBO_SEQUENCE_HEADING);
        NativeTheme::ApplyEmojiFont(dialog, {IDC_COMBO_QUERY, IDC_COMBO_RESULTS, IDC_COMBO_VARIANTS,
                                             IDC_COMBO_ENTRIES});
        CombinationStyle::Apply(dialog);
        NativeTheme::MarkMuted(GetDlgItem(dialog,IDC_COMBO_INTRO));
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_COMBO_SAVED_HINT));
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_COMBO_NAME_HINT));
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_COMBO_ADD_HINT));
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_COMBO_SEQUENCE_HINT));
        NativeTheme::MarkMuted(GetDlgItem(dialog, IDC_COMBO_STATUS));
        ComboSaved(dialog, *e); ComboSearch(dialog, *e);
        ComboStatus(dialog, L"");
        SetFocus(GetDlgItem(dialog, IDC_COMBO_NAME)); return FALSE;
    }
    if (!e) return FALSE;
    if (message == WM_DESTROY) {
        ReleaseHeading(dialog, IDC_COMBO_SAVED_HEADING);
        ReleaseHeading(dialog, IDC_COMBO_EDITOR);
        ReleaseHeading(dialog, IDC_COMBO_ADD_HEADING);
        ReleaseHeading(dialog, IDC_COMBO_SEQUENCE_HEADING);
        NativeTheme::ReleaseEmojiFont(dialog);
        return FALSE;
    }
    if (message == WM_CLOSE) { EndDialog(dialog, IDCANCEL); return TRUE; }
    if (message != WM_COMMAND) return FALSE;
    const int id = LOWORD(wParam), notification = HIWORD(wParam);
    if (id == IDCANCEL) { EndDialog(dialog, IDCANCEL); return TRUE; }
    if (e->loading) return FALSE;
    if (id == IDC_COMBO_NAME && notification == EN_CHANGE) ComboStatus(dialog,L"Unsaved changes");
    else if (id == IDC_COMBO_QUERY && notification == EN_CHANGE) ComboSearch(dialog, *e);
    else if (id == IDC_COMBO_RESULTS && notification == LBN_SELCHANGE) ComboVariants(dialog, *e);
    else if (id == IDC_COMBO_ENTRIES && notification == LBN_SELCHANGE) ComboButtons(dialog, *e);
    else if (id == IDC_COMBO_NEW || (id == IDC_COMBO_SAVED && notification == LBN_SELCHANGE)) {
        Combination draft;
        if (id == IDC_COMBO_SAVED) {
            const auto index = SendDlgItemMessageW(dialog, id, LB_GETCURSEL, 0, 0);
            if (index < 0 || static_cast<size_t>(index) >= e->keys.size()) return TRUE;
            draft = e->parent.profile.combinations.at(e->keys[index]);
        }
        e->draft = draft; e->loading = true;
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, draft.name.c_str());
        e->loading = false;
        Sequence(dialog, e->parent.catalog, draft, 0, e->parent.profile.settings.displayLanguages); ComboSaved(dialog, *e);
        ComboStatus(dialog, L"");
        if (id == IDC_COMBO_NEW) SetFocus(GetDlgItem(dialog, IDC_COMBO_NAME));
    } else if (id == IDC_COMBO_ADD) {
        const auto index = SendDlgItemMessageW(dialog, IDC_COMBO_VARIANTS, CB_GETCURSEL, 0, 0);
        if (index < 0 || static_cast<size_t>(index) >= e->variants.size() || e->draft.entries.size() >= 8) return TRUE;
        const auto* emoji = e->variants[index];
        e->draft.entries.push_back({emoji->family.value, emoji->glyph});
        Sequence(dialog, e->parent.catalog, e->draft, static_cast<int>(e->draft.entries.size() - 1), e->parent.profile.settings.displayLanguages); ComboButtons(dialog, *e);
        ComboStatus(dialog,L"Unsaved changes");
    } else if (id == IDC_COMBO_REMOVE || id == IDC_COMBO_LEFT || id == IDC_COMBO_RIGHT) {
        auto index = static_cast<int>(SendDlgItemMessageW(dialog, IDC_COMBO_ENTRIES, LB_GETCURSEL, 0, 0));
        if (index < 0 || static_cast<size_t>(index) >= e->draft.entries.size()) return TRUE;
        if (id == IDC_COMBO_REMOVE) { e->draft.entries.erase(e->draft.entries.begin() + index); index = std::min(index, static_cast<int>(e->draft.entries.size()) - 1); }
        else {
            const int next = index + (id == IDC_COMBO_LEFT ? -1 : 1);
            if (next < 0 || static_cast<size_t>(next) >= e->draft.entries.size()) return TRUE;
            std::swap(e->draft.entries[index], e->draft.entries[next]); index = next;
        }
        Sequence(dialog, e->parent.catalog, e->draft, index, e->parent.profile.settings.displayLanguages); ComboButtons(dialog, *e);
        ComboStatus(dialog,L"Unsaved changes");
    } else if (id == IDOK) {
        e->draft.name = Text(dialog, IDC_COMBO_NAME);
        std::wstring error;
        if (!SaveCombination(e->parent.profile, e->parent.catalog, e->draft, error)) { ComboStatus(dialog, error); return TRUE; }
        ComboSaved(dialog, *e);
        ComboStatus(dialog, e->parent.persist() ? L"Combination saved." : L"Not saved to disk. Changes remain in this session; try Save again.");
    } else if (id == IDC_COMBO_DELETE && !e->draft.id.empty()) {
        std::wstring text;
        for (const auto& item : e->parent.profile.aliases) if (item.second.target == ResultId{ResultKind::Combination, e->draft.id})
            text += item.second.phrase + L"\r\n";
        if (text.empty()) text = L"No dependent aliases.";
        if (DialogBoxParamW(reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE)),
            MAKEINTRESOURCEW(IDD_COMBO_DELETE), dialog, ConfirmCombinationDelete, reinterpret_cast<LPARAM>(text.c_str())) != IDOK) return TRUE;
        DeleteCombination(e->parent.profile, e->draft.id); e->draft = {};
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, L""); Sequence(dialog, e->parent.catalog, e->draft, 0, e->parent.profile.settings.displayLanguages); ComboSaved(dialog, *e);
        ComboStatus(dialog, e->parent.persist() ? L"Combination deleted." : L"Deletion not saved to disk. Changes remain in this session.");
    } else return FALSE;
    return TRUE;
}
void EditCombinations(HWND dialog, Editor& editor) {
    CombinationEditor state{editor};
    DialogBoxParamW(reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(dialog, GWLP_HINSTANCE)), MAKEINTRESOURCEW(IDD_COMBINATIONS),
        dialog, CombinationProc, reinterpret_cast<LPARAM>(&state));
}
struct CombinationDetails { const Catalog& catalog; const Combination& combination; const DisplayLanguages& languages; };
INT_PTR CALLBACK CombinationDetailsProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    INT_PTR themeResult{};
    if (NativeTheme::HandleMessage(dialog, message, wParam, lParam, themeResult)) return themeResult;
    if (message == WM_INITDIALOG) {
        NativeTheme::Apply(dialog);
        NativeTheme::ApplyEmojiFont(dialog, {IDC_COMBO_DETAILS_PAYLOAD, IDC_COMBO_ENTRIES});
        const auto& state = *reinterpret_cast<CombinationDetails*>(lParam);
        SetDlgItemTextW(dialog, IDC_COMBO_NAME, state.combination.name.c_str());
        Sequence(dialog, state.catalog, state.combination, 0, state.languages);
        std::wstring payload;
        for (const auto& entry : state.combination.entries) payload += entry.payload;
        SetDlgItemTextW(dialog, IDC_COMBO_DETAILS_PAYLOAD, payload.c_str());
        return TRUE;
    }
    if (message == WM_MEASUREITEM && MeasureEmojiControl(dialog, lParam)) return TRUE;
    if (message == WM_DRAWITEM && DrawEmojiControl(dialog, lParam)) return TRUE;
    if (message == WM_DESTROY) { NativeTheme::ReleaseEmojiFont(dialog); return FALSE; }
    if (message == WM_CLOSE || (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)) { EndDialog(dialog, IDCANCEL); return TRUE; }
    return FALSE;
}
}
void ShowCombinationDetails(HWND owner, HINSTANCE instance, const Catalog& catalog, const Combination& combination, const DisplayLanguages& languages) {
    CombinationDetails state{catalog, combination, languages};
    DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_COMBO_DETAILS), owner, CombinationDetailsProc, reinterpret_cast<LPARAM>(&state));
}

bool ShowVocabulary(HWND owner, HINSTANCE instance, const Catalog& catalog, Profile& profile,
                    const std::wstring& phrase, const ResultId& target, const std::function<bool()>& persist) {
    Editor editor{catalog, profile, phrase, target, persist};
    return DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_VOCABULARY), owner, DialogProc,
                          reinterpret_cast<LPARAM>(&editor)) != -1;
}
}
