#pragma once
#include "vocabulary_style.h"

namespace SwashMoji::CombinationStyle {
using namespace VocabularyStyle;
inline constexpr int Width = 592, Height = 466;
inline RECT Bounds(HWND dialog, int id) {
    RECT r{}; GetWindowRect(GetDlgItem(dialog,id),&r);
    MapWindowPoints(nullptr,dialog,reinterpret_cast<POINT*>(&r),2); return r;
}
inline void Layout(HWND dialog) {
    RECT base{0,0,Width,Height}, client{}; MapDialogRect(dialog,&base); GetClientRect(dialog,&client);
    const int dx=MulDiv(client.right-base.right,Width,base.right);
    const int dy=MulDiv(client.bottom-base.bottom,Height,base.bottom);
    // The library stays stable; extra space belongs to results and the editor.
    auto place=[&](int id,int x,int y,int w,int h) {
        RECT r{x,y,x+w,y+h}; MapDialogRect(dialog,&r);
        MoveWindow(GetDlgItem(dialog,id),r.left,r.top,r.right-r.left,r.bottom-r.top,TRUE);
    };
    place(IDC_COMBO_INTRO,16,12,560+dx,14);
    place(IDC_COMBO_LIBRARY,28,40,160,16); place(IDC_COMBO_EDITOR,228,40,336+dx,16);
    place(IDC_COMBO_SAVED_HEADING,28,66,160,12); place(IDC_COMBO_SAVED,28,84,160,294+dy);
    place(IDC_COMBO_NEW,28,386+dy,60,22); place(IDC_COMBO_DELETE,128,386+dy,60,22);
    place(IDC_COMBO_NAME_HEADING,228,66,336+dx,12); place(IDC_COMBO_NAME,228,82,336+dx,20);
    place(IDC_COMBO_NAME_HINT,228,106,336+dx,12);
    place(IDC_COMBO_ADD_HEADING,228,126,336+dx,12); place(IDC_COMBO_QUERY,228,142,336+dx,20);
    place(IDC_COMBO_RESULTS_LABEL,228,168,336+dx,12); place(IDC_COMBO_RESULTS,228,184,336+dx,58+dy);
    place(IDC_COMBO_VARIANT_LABEL,228,253+dy,42,12); place(IDC_COMBO_VARIANTS,274,248+dy,218+dx,120);
    place(IDC_COMBO_ADD,500+dx,248+dy,64,22);
    place(IDC_COMBO_SEQUENCE_HEADING,228,282+dy,100,12); place(IDC_COMBO_SEQUENCE_HINT,350,282+dy,214+dx,12);
    place(IDC_COMBO_ENTRIES,228,298+dy,336+dx,34);
    place(IDC_COMBO_LEFT,228,340+dy,28,22); place(IDC_COMBO_RIGHT,264,340+dy,28,22); place(IDC_COMBO_REMOVE,504+dx,340+dy,60,22);
    place(IDOK,440+dx,372+dy,124,24); place(IDC_COMBO_STATUS,228,400+dy,336+dx,16); place(IDCANCEL,488+dx,434+dy,88,22);
    for (int id : {IDC_COMBO_NAME,IDC_COMBO_QUERY}) {
        auto control=GetDlgItem(dialog,id); auto r=Bounds(dialog,id);
        auto dc=GetDC(control); auto old=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(control,WM_GETFONT,0,0)));
        TEXTMETRICW metrics{}; GetTextMetricsW(dc,&metrics); SelectObject(dc,old); ReleaseDC(control,dc);
        const int h=metrics.tmHeight+Px(dialog,4);
        MoveWindow(control,r.left,r.top+(r.bottom-r.top-h)/2,r.right-r.left,h,TRUE);
    }
    for (int id : {IDC_COMBO_SAVED,IDC_COMBO_RESULTS,IDC_COMBO_ENTRIES}) {
        auto control=GetDlgItem(dialog,id); RECT r{}; GetWindowRect(control,&r);
        SetWindowRgn(control,CreateRoundRectRgn(0,0,r.right-r.left+1,r.bottom-r.top+1,Px(dialog,16),Px(dialog,16)),TRUE);
    }
    SendDlgItemMessageW(dialog,IDC_COMBO_SAVED,LB_SETITEMHEIGHT,0,Px(dialog,36));
    SendDlgItemMessageW(dialog,IDC_COMBO_RESULTS,LB_SETITEMHEIGHT,0,Px(dialog,48));
    RECT sequence{}; GetClientRect(GetDlgItem(dialog,IDC_COMBO_ENTRIES),&sequence);
    SendDlgItemMessageW(dialog,IDC_COMBO_ENTRIES,LB_SETITEMHEIGHT,0,sequence.bottom);
    SendDlgItemMessageW(dialog,IDC_COMBO_ENTRIES,LB_SETCOLUMNWIDTH,std::max(Px(dialog,48),static_cast<int>(sequence.right)/8),0);
    SendDlgItemMessageW(dialog,IDC_COMBO_VARIANTS,CB_SETITEMHEIGHT,static_cast<WPARAM>(-1),Px(dialog,30));
    SendDlgItemMessageW(dialog,IDC_COMBO_VARIANTS,CB_SETITEMHEIGHT,0,Px(dialog,34));
    InvalidateRect(dialog,nullptr,TRUE);
}
inline void Paint(HWND dialog) {
    PAINTSTRUCT paint{}; auto dc=BeginPaint(dialog,&paint); RECT client{}; GetClientRect(dialog,&client);
    FillRect(dc,&client,NativeTheme::BackgroundBrush());
    for (int id : {IDC_COMBO_LIBRARY,IDC_COMBO_EDITOR}) {
        auto r=Bounds(dialog,id); r.bottom=Bounds(dialog,IDC_COMBO_STATUS).bottom;
        InflateRect(&r,Px(dialog,18),0); r.top-=Px(dialog,16);
        Round(dc,r,Panel(),Px(dialog,12));
    }
    for (int id : {IDC_COMBO_NAME,IDC_COMBO_QUERY,IDC_COMBO_VARIANTS,IDC_COMBO_SAVED,IDC_COMBO_RESULTS,IDC_COMBO_ENTRIES}) {
        if (!(GetWindowLongPtrW(GetDlgItem(dialog,id),GWL_STYLE) & WS_VISIBLE)) continue;
        auto r=Bounds(dialog,id);
        const bool edit=id==IDC_COMBO_NAME||id==IDC_COMBO_QUERY;
        if(edit) InflateRect(&r,Px(dialog,8),Px(dialog,4));
        else InflateRect(&r,Px(dialog,2),Px(dialog,2));
        Round(dc,r,Input(),Px(dialog,8),GetFocus()==GetDlgItem(dialog,id) ?
            NativeTheme::HighContrast() ? GetSysColor(COLOR_HIGHLIGHT) : RGB(133,180,244) : CLR_INVALID);
    }
    EndPaint(dialog,&paint);
}
inline bool Message(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam, INT_PTR& result) {
    if(message==WM_DPICHANGED) PostMessageW(dialog,WM_APP+72,0,0);
    if(message==WM_APP+72 || (message==WM_SIZE && GetDlgItem(dialog,IDC_COMBO_NAME))) { Layout(dialog); result=TRUE; return true; }
    if(message==WM_PAINT) { Paint(dialog); result=TRUE; return true; }
    if(message==WM_GETMINMAXINFO) {
        RECT r{0,0,Width,Height}; MapDialogRect(dialog,&r);
        AdjustWindowRectExForDpi(&r,static_cast<DWORD>(GetWindowLongPtrW(dialog,GWL_STYLE)),FALSE,static_cast<DWORD>(GetWindowLongPtrW(dialog,GWL_EXSTYLE)),GetDpiForWindow(dialog));
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize={r.right-r.left,r.bottom-r.top}; result=TRUE; return true;
    }
    if(message==WM_CTLCOLOREDIT || message==WM_CTLCOLORLISTBOX || message==WM_CTLCOLORSTATIC) {
        auto dc=reinterpret_cast<HDC>(wParam); auto control=reinterpret_cast<HWND>(lParam);
        SetTextColor(dc,GetPropW(control,L"SwashMojiMuted") ? NativeTheme::SecondaryText() : NativeTheme::Foreground());
        SetBkMode(dc,TRANSPARENT); SetBkColor(dc,Input());
        result=reinterpret_cast<INT_PTR>(message!=WM_CTLCOLORSTATIC ? InputBrush() : GetDlgCtrlID(control)==IDC_COMBO_INTRO ? NativeTheme::BackgroundBrush() : PanelBrush()); return true;
    }
    return false;
}
inline void Apply(HWND dialog) {
    SetWindowSubclass(GetDlgItem(dialog,IDC_COMBO_VARIANTS),ControlProc,1,4);
    for(int id : {IDC_COMBO_NEW,IDC_COMBO_DELETE,IDC_COMBO_ADD,IDC_COMBO_LEFT,IDC_COMBO_RIGHT,IDC_COMBO_REMOVE,IDOK,IDCANCEL})
        SetWindowSubclass(GetDlgItem(dialog,id),ControlProc,1,1);
    for(int id : {IDC_COMBO_NAME,IDC_COMBO_QUERY,IDC_COMBO_SAVED,IDC_COMBO_RESULTS,IDC_COMBO_ENTRIES}) {
        auto control=GetDlgItem(dialog,id);
        SetWindowLongPtrW(control,GWL_EXSTYLE,GetWindowLongPtrW(control,GWL_EXSTYLE)&~WS_EX_CLIENTEDGE);
        SetWindowLongPtrW(control,GWL_STYLE,GetWindowLongPtrW(control,GWL_STYLE)&~WS_BORDER);
        SetWindowPos(control,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        SetWindowSubclass(control,ControlProc,1,id==IDC_COMBO_NAME||id==IDC_COMBO_QUERY ? 3 : 2);
    }
    SendDlgItemMessageW(dialog,IDC_COMBO_QUERY,EM_SETCUEBANNER,TRUE,reinterpret_cast<LPARAM>(L"Search emoji in English or Norwegian"));
    Layout(dialog);
}
}
