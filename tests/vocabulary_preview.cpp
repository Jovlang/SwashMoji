#include "vocabulary.h"
#include "storage.h"
#include "language_preferences.h"
#include <fstream>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR command, int) {
    using namespace SwashMoji;
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    const auto directory = std::filesystem::path(executable).parent_path();
    std::ifstream data(directory / "emojis.txt", std::ios::binary), intents(directory / "intent_phrases.tsv", std::ios::binary);
    Catalog catalog;
    if (!catalog.Load(data) || !catalog.LoadIntents(intents)) return 1;
    ProfileStorage store(directory / "vocabulary-preview-profile");
    auto profile = store.Load(&catalog).profile;
    std::wstring diagnostic;
#ifdef SWASHMOJI_LANGUAGE_PREVIEW
    return ShowLanguagePreferences(nullptr, instance, profile, [&] { return store.Save(profile, diagnostic); }) ? 0 : 1;
#else
    if (std::wstring(command) == L"--languages")
        return ShowLanguagePreferences(nullptr, instance, profile, [&] { return store.Save(profile, diagnostic); }) ? 0 : 1;
    return ShowVocabulary(nullptr, instance, catalog, profile, L"bra jobbet", {ResultKind::Emoji, L"👏"},
        [&] { return store.Save(profile, diagnostic); }) ? 0 : 1;
#endif
}
