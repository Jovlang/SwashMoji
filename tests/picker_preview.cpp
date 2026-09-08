#include "picker_vocabulary_tests.cpp"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    wchar_t executable[] = L"SwashMojiPickerPreview";
    wchar_t preview[] = L"--preview";
    wchar_t* args[]{executable, preview};
    return wmain(2, args);
}
