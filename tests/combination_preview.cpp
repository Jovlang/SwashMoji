// Isolated visual harness; never reads the user's application profile.
#include "../src/vocabulary.cpp"
#include "../src/storage.h"
#include <fstream>
INT_PTR CALLBACK PreviewProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
    const auto handled=SwashMoji::CombinationProc(dialog,message,wParam,lParam);
    if(message==WM_INITDIALOG && SendDlgItemMessageW(dialog,IDC_COMBO_SAVED,LB_GETCOUNT,0,0)>0) {
        SendDlgItemMessageW(dialog,IDC_COMBO_SAVED,LB_SETCURSEL,0,0);
        SendMessageW(dialog,WM_COMMAND,MAKEWPARAM(IDC_COMBO_SAVED,LBN_SELCHANGE),0);
    }
    return handled;
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR arguments, int) {
    using namespace SwashMoji;
    wchar_t executable[MAX_PATH]{}; GetModuleFileNameW(nullptr,executable,MAX_PATH);
    const auto directory=std::filesystem::path(executable).parent_path();
    std::ifstream data(directory/"emojis.txt",std::ios::binary);
    Catalog catalog; if(!catalog.Load(data)) return 1;
    ProfileStorage storage(directory/"combination-preview-profile");
    auto profile=storage.Load(&catalog).profile;
    std::wstring diagnostic; std::function<bool()> persist=[&]{return storage.Save(profile,diagnostic);};
    if(std::wstring(arguments).find(L"--empty")==std::wstring::npos && profile.combinations.empty()) {
        Combination sample; sample.name=L"celebrate";
        for(const auto* glyph : {L"🎉",L"❤️",L"✨",L"🙂",L"🚀",L"🥳",L"🎶",L"🙌"}) {
            const auto* emoji=catalog.Find(glyph); if(emoji) sample.entries.push_back({emoji->family.value,glyph});
        }
        SaveCombination(profile,catalog,sample,diagnostic);
    }
    Editor parent{catalog,profile,{},{},persist}; CombinationEditor state{parent};
    return DialogBoxParamW(instance,MAKEINTRESOURCEW(IDD_COMBINATIONS),nullptr,PreviewProc,reinterpret_cast<LPARAM>(&state))==-1 ? 1 : 0;
}
