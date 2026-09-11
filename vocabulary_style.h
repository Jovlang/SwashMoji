#pragma once
#include "native_theme.h"
#include "native_emoji.h"
#include "vocabulary_ids.h"
#include <commctrl.h>

// Native controls retain their input, mnemonic and accessibility implementations.
// This layer owns only surfaces, spacing and interaction-state painting.
namespace SwashMoji::VocabularyStyle {
inline int Px(HWND window, int value) { return MulDiv(value, GetDpiForWindow(window), 96); }
inline COLORREF Panel() { return NativeTheme::HighContrast() ? GetSysColor(COLOR_WINDOW) : RGB(30, 32, 36); }
inline COLORREF Input() { return NativeTheme::HighContrast() ? GetSysColor(COLOR_WINDOW) : RGB(39, 42, 47); }
inline HBRUSH PanelBrush() { static auto brush = CreateSolidBrush(RGB(30,32,36)); return NativeTheme::HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : brush; }
inline HBRUSH InputBrush() { static auto brush = CreateSolidBrush(RGB(39,42,47)); return NativeTheme::HighContrast() ? GetSysColorBrush(COLOR_WINDOW) : brush; }
inline void Round(HDC dc, RECT r, COLORREF color, int radius, COLORREF edge = CLR_INVALID) {
    auto brush = CreateSolidBrush(color);
    auto pen = CreatePen(PS_SOLID, 1, edge == CLR_INVALID ? color : edge);
    auto oldBrush = SelectObject(dc, brush); auto oldPen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius * 2, radius * 2);
    SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(brush); DeleteObject(pen);
}
inline LRESULT CALLBACK ControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR kind) {
    if (message == WM_NCDESTROY) { RemoveWindowSubclass(window, ControlProc, 1); return DefSubclassProc(window, message, wParam, lParam); }
    if (message == WM_MOUSEMOVE && kind == 2) {
        auto hit=SendMessageW(window,LB_ITEMFROMPOINT,0,lParam);
        auto hot=HIWORD(hit) ? 0 : LOWORD(hit)+1;
        if (reinterpret_cast<UINT_PTR>(GetPropW(window,L"VocabularyHotRow"))!=hot) {
            SetPropW(window,L"VocabularyHotRow",reinterpret_cast<HANDLE>(static_cast<UINT_PTR>(hot)));
            InvalidateRect(window,nullptr,FALSE);
        }
    }
    if (message == WM_MOUSEMOVE) {
        if (!GetPropW(window, L"VocabularyHover")) {
            SetPropW(window, L"VocabularyHover", reinterpret_cast<HANDLE>(1));
            TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, window, 0}; TrackMouseEvent(&track);
            InvalidateRect(window, nullptr, FALSE);
        }
    }
    if (message == WM_MOUSELEAVE) { RemovePropW(window, L"VocabularyHover"); RemovePropW(window,L"VocabularyHotRow"); InvalidateRect(window, nullptr, FALSE); }
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS || message == WM_ENABLE || message == BM_SETSTATE || message == BM_SETSTYLE || message == WM_UPDATEUISTATE) {
        auto result = DefSubclassProc(window, message, wParam, lParam);
        InvalidateRect(window, nullptr, FALSE); InvalidateRect(GetParent(window), nullptr, FALSE); return result;
    }
    if (message == WM_PAINT && kind == 4 && !NativeTheme::HighContrast()) {
        PAINTSTRUCT paint{}; auto dc=BeginPaint(window,&paint); RECT r{}; GetClientRect(window,&r);
        FillRect(dc,&r,PanelBrush());
        Round(dc,r,Input(),Px(window,8),GetFocus()==window ? RGB(133,180,244) : CLR_INVALID);
        const auto selected=SendMessageW(window,CB_GETCURSEL,0,0);
        std::wstring label=L"Choose an emoji";
        if(selected>=0) {
            const auto length=SendMessageW(window,CB_GETLBTEXTLEN,selected,0);
            if(length>=0) { label.resize(static_cast<size_t>(length)+1); SendMessageW(window,CB_GETLBTEXT,selected,reinterpret_cast<LPARAM>(label.data())); label.resize(static_cast<size_t>(length)); }
        }
        auto text=r; text.left+=Px(window,8); text.right-=Px(window,32);
        NativeEmoji::DrawLine(dc,text,label,static_cast<float>(Px(window,14)),selected>=0 ? NativeTheme::Foreground() : NativeTheme::SecondaryText(),false,true,true);
        auto arrow=r; arrow.left=arrow.right-Px(window,28);
        NativeEmoji::DrawLine(dc,arrow,L"⌄",static_cast<float>(Px(window,16)),NativeTheme::SecondaryText(),true,false,true);
        EndPaint(window,&paint); return 0;
    }
    if (message == WM_PAINT && kind == 1 && !NativeTheme::HighContrast()) {
        PAINTSTRUCT paint{}; auto dc = BeginPaint(window, &paint); RECT r{}; GetClientRect(window, &r);
        FillRect(dc, &r, (GetDlgCtrlID(window)==IDCANCEL || GetDlgCtrlID(window)==IDC_COMBINATIONS) ? NativeTheme::BackgroundBrush() : PanelBrush());
        int id = GetDlgCtrlID(window);
        bool primary = id == IDC_SAVE_ALIAS || id == IDOK;
        bool quiet = id == IDC_DELETE_ALIAS || id == IDC_UNPIN || id == IDC_COMBINATIONS || id == IDC_COMBO_DELETE || id == IDC_COMBO_REMOVE;
        bool enabled = IsWindowEnabled(window), hover = GetPropW(window, L"VocabularyHover") != nullptr;
        bool pressed = (SendMessageW(window, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;
        auto fill = primary ? RGB(133,180,244) : quiet ? (id==IDC_COMBINATIONS ? NativeTheme::Background : Panel()) : RGB(49,53,60);
        if(id==IDC_COMBO_ADD) fill=RGB(48,70,104);
        if (enabled && hover) fill = primary ? RGB(156,196,250) : RGB(61,66,75);
        if (enabled && pressed) fill = primary ? RGB(105,154,219) : RGB(43,47,54);
        if (!enabled) fill = quiet ? Panel() : RGB(39,42,47);
        Round(dc, r, fill, Px(window, 8));
        wchar_t label[128]{}; GetWindowTextW(window, label, 128);
        if (id == IDC_PIN_UP) wcscpy_s(label, L"↑");
        if (id == IDC_PIN_DOWN) wcscpy_s(label, L"↓");
        if (id == IDC_COMBO_LEFT) wcscpy_s(label,L"←");
        if (id == IDC_COMBO_RIGHT) wcscpy_s(label,L"→");
        auto font = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0)); auto old = SelectObject(dc, font);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, !enabled ? RGB(112,117,125) : primary ? RGB(19,32,51) : quiet && id != IDC_COMBINATIONS ? RGB(228,157,157) : NativeTheme::Foreground());
        DrawTextW(dc, label, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_HIDEPREFIX);
        if (GetFocus() == window) { InflateRect(&r, -Px(window, 3), -Px(window, 3)); DrawFocusRect(dc, &r); }
        SelectObject(dc, old); EndPaint(window, &paint); return 0;
    }
    auto result = DefSubclassProc(window, message, wParam, lParam);
    if (message == WM_PAINT && kind == 3 && GetDlgCtrlID(window)==IDC_COMBO_QUERY && GetWindowTextLengthW(window)==0) {
        auto dc=GetDC(window); RECT r{}; GetClientRect(window,&r);
        FillRect(dc,&r,InputBrush());
        auto old=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(window,WM_GETFONT,0,0)));
        SetBkMode(dc,TRANSPARENT); SetTextColor(dc,NativeTheme::SecondaryText());
        DrawTextW(dc,L"Search emoji in English or Norwegian",-1,&r,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
        SelectObject(dc,old); ReleaseDC(window,dc);
    }
    if (message == WM_PAINT && kind == 2 && SendMessageW(window, LB_GETCOUNT, 0, 0) == 0) {
        auto dc = GetDC(window); RECT r{}; GetClientRect(window, &r); InflateRect(&r, -Px(window,12), 0);
        auto old = SelectObject(dc, reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT,0,0)));
        SetBkMode(dc, TRANSPARENT); SetTextColor(dc, NativeTheme::SecondaryText());
        const auto text = GetDlgCtrlID(window) == IDC_PINS ? L"No pinned favorites yet" : GetDlgCtrlID(window) == IDC_ALIASES ? L"Your saved phrases appear here" : GetDlgCtrlID(window) == IDC_COMBO_SAVED ? L"No saved combinations yet" : GetDlgCtrlID(window) == IDC_COMBO_ENTRIES ? L"No emoji added yet" : L"No matching emoji";
        if(GetDlgCtrlID(window)==IDC_COMBO_SAVED) {
            const int middle=(r.top+r.bottom)/2;
            r.top=middle-Px(window,22); r.bottom=middle;
            DrawTextW(dc,text,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
            r.top=middle; r.bottom=middle+Px(window,22);
            DrawTextW(dc,L"Create one to get started.",-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        } else DrawTextW(dc, text, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
        SelectObject(dc, old); ReleaseDC(window, dc);
    }
    return result;
}
inline void Layout(HWND dialog) {
    // Dialog units follow the current dialog font/DPI. Extra width is shared
    // 40:60; extra height goes to lists, preserving field and action heights.
    RECT base{0,0,552,432}, client{}; MapDialogRect(dialog,&base); GetClientRect(dialog,&client);
    int dx = MulDiv(client.right - base.right, 552, base.right), dy = MulDiv(client.bottom - base.bottom,432,base.bottom);
    int left = dx * 2 / 5, right = dx - left;
    auto place = [&](int id,int x,int y,int w,int h) { RECT r{x,y,x+w,y+h}; MapDialogRect(dialog,&r); MoveWindow(GetDlgItem(dialog,id),r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE); };
    place(IDC_VOCABULARY_INTRO,16,14,520+dx,16);
    place(IDC_ALIASES_HEADING,28,48,176+left,16);
    place(IDC_ALIASES,28,66,176+left,140+dy/2);
    place(IDC_NEW_ALIAS,28,214+dy/2,62,24); place(IDC_DELETE_ALIAS,142+left,214+dy/2,62,24);
    place(IDC_PINS_HEADING,28,256+dy/2,176+left,12);
    place(IDC_PINS,28,274+dy/2,176+left,66+dy-dy/2);
    place(IDC_PIN_UP,28,348+dy,28,24); place(IDC_PIN_DOWN,62,348+dy,28,24); place(IDC_UNPIN,142+left,348+dy,62,24);
    place(IDC_ALIAS_HEADING,244+left,48,280+right,16);
    place(IDC_PHRASE_LABEL,244+left,76,280+right,12); place(IDC_PHRASE,244+left,94,280+right,24);
    place(IDC_SEARCH_LABEL,244+left,130,280+right,12); place(IDC_TARGET_QUERY,244+left,148,280+right,24);
    place(IDC_RESULTS_LABEL,244+left,184,280+right,12); place(IDC_TARGET_RESULTS,244+left,202,280+right,138+dy);
    place(IDC_SAVE_ALIAS,244+left,348+dy,96,24); place(IDC_PIN_TARGET,348+left,348+dy,96,24);
    place(IDC_VOCABULARY_STATUS,244+left,376+dy,280+right,18);
    place(IDC_COMBINATIONS,16,398+dy,176,24); place(IDCANCEL,448+dx,398+dy,88,24);
    for (int id : {IDC_PHRASE,IDC_TARGET_QUERY}) {
        auto control=GetDlgItem(dialog,id); RECT r{}; GetWindowRect(control,&r); MapWindowPoints(nullptr,dialog,reinterpret_cast<POINT*>(&r),2);
        auto dc=GetDC(control); auto old=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(control,WM_GETFONT,0,0)));
        TEXTMETRICW metrics{}; GetTextMetricsW(dc,&metrics); SelectObject(dc,old); ReleaseDC(control,dc);
        int height=metrics.tmHeight+Px(dialog,4);
        MoveWindow(control,r.left,r.top+(r.bottom-r.top-height)/2,r.right-r.left,height,TRUE);
    }
    for (int id : {IDC_ALIASES,IDC_PINS,IDC_TARGET_RESULTS}) {
        auto control=GetDlgItem(dialog,id); RECT r{}; GetWindowRect(control,&r);
        SetWindowRgn(control,CreateRoundRectRgn(0,0,r.right-r.left+1,r.bottom-r.top+1,Px(dialog,16),Px(dialog,16)),TRUE);
        SendMessageW(control,LB_SETITEMHEIGHT,0,Px(dialog,id==IDC_TARGET_RESULTS ? 48 : 36));
    }
    InvalidateRect(dialog,nullptr,FALSE);
}
inline void Paint(HWND dialog) {
    PAINTSTRUCT paint{}; auto dc=BeginPaint(dialog,&paint); RECT client{}; GetClientRect(dialog,&client);
    FillRect(dc,&client,NativeTheme::BackgroundBrush());
    auto bounds = [&](int id) { RECT r{}; GetWindowRect(GetDlgItem(dialog,id),&r); MapWindowPoints(nullptr,dialog,reinterpret_cast<POINT*>(&r),2); return r; };
    auto library=bounds(IDC_ALIASES_HEADING), editor=bounds(IDC_ALIAS_HEADING), bottom=bounds(IDC_VOCABULARY_STATUS);
    library.left-=Px(dialog,18); library.top-=Px(dialog,16); library.right+=Px(dialog,18); library.bottom=bottom.bottom;
    editor.left-=Px(dialog,18); editor.top-=Px(dialog,16); editor.right+=Px(dialog,18); editor.bottom=bottom.bottom;
    Round(dc,library,Panel(),Px(dialog,12)); Round(dc,editor,Panel(),Px(dialog,12));
    for (int id : {IDC_PHRASE,IDC_TARGET_QUERY}) {
        auto r=bounds(id); InflateRect(&r,Px(dialog,8),Px(dialog,4));
        Round(dc,r,Input(),Px(dialog,8),GetFocus()==GetDlgItem(dialog,id) ?
            (NativeTheme::HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : RGB(133,180,244)) : CLR_INVALID);
    }
    for (int id : {IDC_ALIASES,IDC_PINS,IDC_TARGET_RESULTS}) {
        if (GetFocus()==GetDlgItem(dialog,id)) {
            auto r=bounds(id); InflateRect(&r,Px(dialog,2),Px(dialog,2));
            Round(dc,r,Input(),Px(dialog,8),NativeTheme::HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : RGB(133,180,244));
        }
    }
    EndPaint(dialog,&paint);
}
inline void Apply(HWND dialog) {
    for (int id : {IDC_NEW_ALIAS,IDC_DELETE_ALIAS,IDC_PIN_UP,IDC_PIN_DOWN,IDC_UNPIN,IDC_SAVE_ALIAS,IDC_PIN_TARGET,IDCANCEL,IDC_COMBINATIONS})
        SetWindowSubclass(GetDlgItem(dialog,id),ControlProc,1,1);
    for (int id : {IDC_ALIASES,IDC_PINS,IDC_TARGET_RESULTS,IDC_PHRASE,IDC_TARGET_QUERY}) {
        auto control=GetDlgItem(dialog,id);
        SetWindowLongPtrW(control,GWL_EXSTYLE,GetWindowLongPtrW(control,GWL_EXSTYLE)&~WS_EX_CLIENTEDGE);
        SetWindowLongPtrW(control,GWL_STYLE,GetWindowLongPtrW(control,GWL_STYLE)&~WS_BORDER);
        SetWindowPos(control,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        SetWindowSubclass(control,ControlProc,1,id==IDC_PHRASE||id==IDC_TARGET_QUERY ? 3 : 2);
    }
    SendDlgItemMessageW(dialog,IDC_TARGET_QUERY,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(L"Search emoji in English or Norwegian"));
    Layout(dialog);
}
}
