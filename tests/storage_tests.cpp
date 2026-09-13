#include <windows.h>
#include "test_support.h"
#include "storage.h"
#include "activation_win32.h"
#include "text.h"
#include "catalog.h"
#include <fstream>
#include <iterator>
#include <sstream>

using namespace SwashMoji;
namespace fs = std::filesystem;

struct TestDirectory {
    fs::path parent = fs::current_path();
    fs::path path = parent / (L"storage-tests-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
    TestDirectory() { CHECK(fs::create_directory(path)); }
    ~TestDirectory() {
        // This fixture creates and removes only its own direct child of the test cwd.
        if (path.parent_path() == parent && path.filename().wstring().find(L"storage-tests-") == 0) {
            std::error_code error;
            fs::remove_all(path, error);
        }
    }
};

void Write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << bytes;
    CHECK(file.good());
}
std::string Read(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    CHECK(file.good());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void Codec() {
    Profile profile;
    profile.settings = {true, true, 3, 5};
    profile.settings.learnQueries = false;
    Remember(profile, L"👍🏽");
    Remember(profile, L"🚀\t\\\n\r✨");
    profile.aliases[L"på vei"] = {L"PÅ\tvei", {ResultKind::Emoji, L"🚶"}};
    profile.aliases[L"future combo"] = {L"future combo", {ResultKind::Combination, L"reserved-42"}};
    profile.pins = {{ResultKind::Emoji, L"👍"}, {ResultKind::Combination, L"reserved-42"}};
    profile.queryChoices = {{L"på vei", {ResultKind::Emoji, L"🚶"}, 8}, {L"launch", {ResultKind::Combination, L"reserved-42"}, 3}};
    const auto bytes = EncodeProfile(profile);
    auto decoded = DecodeProfile(bytes);
    CHECK(decoded.format == ProfileFormat::Valid && decoded.skippedRecords == 0);
    CHECK(decoded.profile.history == profile.history);
    CHECK(decoded.profile.usage == profile.usage);
    CHECK(decoded.profile.settings.positionAboveTextField);
    CHECK(decoded.profile.settings.sortByUsage);
    CHECK(!decoded.profile.settings.learnQueries);
    CHECK(decoded.profile.pins == profile.pins);
    CHECK(QueryCount(decoded.profile, L"PÅ VEI", {ResultKind::Emoji, L"🚶"}) == 8);
    CHECK(decoded.profile.settings.emojiRows == 3 && decoded.profile.settings.skinTone == 5);
    CHECK(EncodeProfile(decoded.profile) == bytes);
    for (size_t size = 0; size < bytes.size(); ++size) CHECK(DecodeProfile(bytes.substr(0, size)).format != ProfileFormat::Valid);
    CHECK(decoded.profile.aliases.at(L"på vei").target.value == L"🚶");
    CHECK(DecodeProfile("SwashMoji\t7\nend\t0\n").format == ProfileFormat::Unsupported);
    CHECK(DecodeProfile("SwashMoji\t7").format == ProfileFormat::Unsupported);
    CHECK(DecodeProfile("\xEF\xBB\xBF" "SwashMoji\t1\r\nsetting\temoji_rows\t2\r\nend\t1\r\n").profile.settings.emojiRows == 2);
    decoded = DecodeProfile("SwashMoji\t1\nusage\trocket\t4294967296\nrecent\tbad\\q\nsetting\temoji_rows\t99\nusage\tgood\t2\nend\t4\n");
    CHECK(decoded.format == ProfileFormat::Valid && decoded.skippedRecords == 3);
    CHECK(decoded.profile.settings.emojiRows == 1 && UsageCount(decoded.profile, L"good") == 2);
    CHECK(DecodeProfile("SwashMoji\t1\nend\t0\ntrailing\n").format == ProfileFormat::Invalid);
    CHECK(DecodeProfile(std::string(4 * 1024 * 1024 + 1, 'x')).format == ProfileFormat::Invalid);
    decoded = DecodeProfile("SwashMoji\t3\nquery\tPÅ VEI\temoji\t🚶\t8\nquery\tpå-vei\temoji\t🚶\t9\nquery\t!!!\temoji\t🚶\t1\nquery\tbad\temoji\t🚶\t4294967296\npin\temoji\t🚶\npin\temoji\t🚶\nend\t6\n");
    CHECK(decoded.format == ProfileFormat::Valid && decoded.skippedRecords == 4);
    CHECK(decoded.profile.queryChoices.size() == 1 && decoded.profile.pins.size() == 1);
    CHECK(QueryCount(decoded.profile, L"på vei", {ResultKind::Emoji, L"🚶"}) == 8);
    std::string excessive = "SwashMoji\t3\n";
    for (size_t i = 0; i <= kMaxQueryChoices; ++i) excessive += "query\tquery " + std::to_string(i) + "\temoji\t🚶\t1\n";
    excessive += "end\t" + std::to_string(kMaxQueryChoices + 1) + "\n";
    decoded = DecodeProfile(excessive);
    CHECK(decoded.format == ProfileFormat::Valid && decoded.skippedRecords == 1);
    CHECK(decoded.profile.queryChoices.size() == kMaxQueryChoices);
}

void ActivationSettings(const fs::path& root) {
    Profile profile;
    CHECK(profile.settings.activationHotkey == kDefaultActivationHotkey);
    CHECK(ActivationHotkeyLabel(kDefaultActivationHotkey) == L"Alt+E");
    CHECK(ActivationHotkeyLabel(0x067A) == L"Ctrl+Shift+F11");
    for (unsigned int invalid : {0u, 0x45u, 0x0445u, 0x0140u, 0x017Bu, 0x1045u, 0x1000145u}) {
        CHECK(!ValidActivationHotkey(invalid));
        const auto decoded = DecodeProfile("SwashMoji\t6\nsetting\tactivation_hotkey\t" + std::to_string(invalid) + "\nend\t1\n");
        CHECK(decoded.format == ProfileFormat::Valid && decoded.skippedRecords == 1);
        CHECK(decoded.profile.settings.activationHotkey == kDefaultActivationHotkey);
    }
    for (unsigned int value : {0x0145u, 0x0351u, 0x067Au, 0x0A39u}) {
        profile.settings.activationHotkey = value;
        auto decoded = DecodeProfile(EncodeProfile(profile));
        CHECK(decoded.format == ProfileFormat::Valid && !decoded.skippedRecords);
        CHECK(decoded.profile.settings.activationHotkey == value);
        ClearHistory(profile);
        CHECK(profile.settings.activationHotkey == value);
    }
    const auto path = root / L"activation";
    const std::string v5 = "SwashMoji\t5\nsetting\tskin_tone\t3\ndisplay_languages\tes\nend\t2\n";
    Write(path / L"profile.tsv", v5);
    ProfileStorage storage(path);
    auto loaded = storage.Load();
    CHECK(loaded.migrated && !loaded.unsaved);
    CHECK(loaded.profile.settings.activationHotkey == kDefaultActivationHotkey);
    CHECK(loaded.profile.settings.skinTone == 3 && loaded.profile.settings.displayLanguages.Locales() == std::vector<std::string>{"es"});
    CHECK(Read(path / L"profile.tsv.bak") == v5);
    loaded.profile.settings.activationHotkey = 0x0351;
    std::wstring error;
    CHECK(storage.Save(loaded.profile, error));
    CHECK(storage.Load().profile.settings.activationHotkey == 0x0351);

    // Simulate a conflicting desktop registration without taking any real hotkeys.
    std::map<int, UINT> registrations;
    bool conflict{};
    ActivationRegistration active([&](HWND, int id, UINT mods, UINT key) -> BOOL {
        CHECK(mods & MOD_NOREPEAT);
        if (conflict) return FALSE;
        registrations[id] = key; return TRUE;
    }, [&](HWND, int id) -> BOOL { return registrations.erase(id) != 0; });
    CHECK(active.Prepare(nullptr, 0x0145)); active.Commit(nullptr, 0x0145);
    const auto original = active.Id();
    conflict = true;
    CHECK(!active.Prepare(nullptr, 0x0351));
    CHECK(active.Id() == original && registrations.size() == 1 && registrations.at(original) == 'E');
    conflict = false;
    CHECK(active.Prepare(nullptr, 0x0351));
    CHECK(registrations.size() == 2);
    active.Cancel(nullptr); // e.g. a failed startup write
    CHECK(active.Id() == original && registrations.size() == 1);
    CHECK(active.Prepare(nullptr, 0x0351)); active.Commit(nullptr, 0x0351);
    CHECK(active.Id() != original && registrations.size() == 1 && registrations.at(active.Id()) == 'Q');
    CHECK(active.Prepare(nullptr, 0x0351)); active.Commit(nullptr, 0x0351);
    CHECK(registrations.size() == 1);
    active.Clear(nullptr); CHECK(registrations.empty());

    // Never touch the Run key: this adapter fixture has its own disposable key.
    const auto key = L"Software\\SwashMoji-test-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    struct Cleanup { std::wstring key; ~Cleanup() { RegDeleteKeyW(HKEY_CURRENT_USER, key.c_str()); } } cleanup{key};
    std::wstring command;
    CHECK(ReadStartupCommand(command, key.c_str()) == ERROR_SUCCESS && command.empty());
    const auto expected = StartupCommand(L"C:\\Emoji tools\\æøå\\SwashMoji.exe");
    CHECK(expected == L"\"C:\\Emoji tools\\æøå\\SwashMoji.exe\"");
    CHECK(WriteStartupCommand(expected, key.c_str()) == ERROR_SUCCESS);
    CHECK(ReadStartupCommand(command, key.c_str()) == ERROR_SUCCESS && command == expected);
    CHECK(WriteStartupCommand(StartupCommand(L"D:\\Moved\\SwashMoji.exe"), key.c_str()) == ERROR_SUCCESS);
    CHECK(ReadStartupCommand(command, key.c_str()) == ERROR_SUCCESS && command == StartupCommand(L"D:\\Moved\\SwashMoji.exe"));
    CHECK(WriteStartupCommand(L"", key.c_str()) == ERROR_SUCCESS);
    CHECK(ReadStartupCommand(command, key.c_str()) == ERROR_SUCCESS && command.empty());
    CHECK(WriteStartupCommand(L"", key.c_str()) == ERROR_SUCCESS);
}

void ImportExport(const fs::path& root) {
    const auto directory = root / L"import-export";
    Profile original;
    original.settings.skinTone = 4;
    original.aliases[L"launch"] = {L"launch", {ResultKind::Emoji, L"rocket"}};
    std::wstring diagnostic;
    const auto exported = directory / L"SwashMoji-profile.tsv";
    CHECK(WriteProfileExport(exported, original, diagnostic));
    Profile imported;
    CHECK(ReadProfileExport(exported, imported, diagnostic));
    CHECK(imported.settings.skinTone == 4 && imported.aliases.at(L"launch").target.value == L"rocket");
    Write(directory / L"truncated.tsv", "SwashMoji\t6\nsetting\tskin_tone\t2\n");
    CHECK(!ReadProfileExport(directory / L"truncated.tsv", imported, diagnostic) && !diagnostic.empty());
    Write(directory / L"future.tsv", "SwashMoji\t99\nend\t0\n");
    CHECK(!ReadProfileExport(directory / L"future.tsv", imported, diagnostic) && !diagnostic.empty());
    Write(directory / L"skipped.tsv", "SwashMoji\t6\nbogus\nend\t1\n");
    CHECK(!ReadProfileExport(directory / L"skipped.tsv", imported, diagnostic) && !diagnostic.empty());
}

void FamilyMigration(const fs::path& root) {
    std::istringstream data("👍\tthumbs up\tgood\n👍🏽\tthumbs up medium skin tone\tgood\n🚀\trocket\tlaunch\n");
    Catalog catalog;
    CHECK(catalog.Load(data));
    const std::string v2 = "SwashMoji\t2\nrecent\t👍🏽\nrecent\t🚀\nrecent\t👍\nusage\t👍\t7\nusage\t👍🏽\t8\nusage\t🚀\t4\nalias\tlaunch\temoji\t🚀\nsetting\tskin_tone\t3\nend\t8\n";
    const auto directory = root / L"family-migration";
    Write(directory / L"profile.tsv", v2);
    ProfileStorage store(directory);
    auto loaded = store.Load(&catalog);
    CHECK(loaded.migrated && !loaded.unsaved);
    CHECK(loaded.profile.settings.skinTone == 3 && loaded.profile.settings.learnQueries);
    CHECK(loaded.profile.history == (std::vector<ResultId>{{ResultKind::Emoji, L"👍"}, {ResultKind::Emoji, L"🚀"}}));
    CHECK(UsageCount(loaded.profile, L"👍") == 15);
    CHECK(loaded.profile.aliases.at(L"launch").target.value == L"🚀");
    CHECK(Read(directory / L"profile.tsv.bak") == v2);
    CHECK(DecodeProfile(Read(directory / L"profile.tsv")).version == 6);
    loaded = store.Load(&catalog);
    CHECK(!loaded.migrated && UsageCount(loaded.profile, L"👍") == 15);
    RecordChoice(loaded.profile, {ResultKind::Emoji, L"👍"}, L"good");
    std::wstring diagnostic;
    CHECK(store.Save(loaded.profile, diagnostic));
    CHECK(UsageCount(store.Load(&catalog).profile, L"👍") == 16);

    const auto locked = root / L"locked-family-migration";
    Write(locked / L"profile.tsv", v2);
    ProfileStorage blocked(locked);
    const auto handle = CreateFileW((locked / L"profile.tsv").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(handle != INVALID_HANDLE_VALUE);
    loaded = blocked.Load(&catalog);
    CloseHandle(handle);
    CHECK(loaded.unsaved && !loaded.migrated && !loaded.diagnostic.empty());
    CHECK(UsageCount(loaded.profile, L"👍") == 15);
    CHECK(Read(locked / L"profile.tsv") == v2);
    CHECK(blocked.Save(loaded.profile, diagnostic));
    CHECK(!blocked.Load(&catalog).migrated && UsageCount(blocked.Load(&catalog).profile, L"👍") == 15);
}

void LanguageSettings(const fs::path& root) {
    Profile profile;
    const std::vector<std::string> defaults{"en", "nb"};
    CHECK(profile.settings.displayLanguages.Locales() == defaults);
    for (const auto& invalid : std::vector<std::vector<std::string>>{{}, {"en", "en"}, {"en", "nb", "de"}, {"zz"}, {"en", ""}}) {
        CHECK(!profile.settings.displayLanguages.Set(invalid));
        CHECK(profile.settings.displayLanguages.Locales() == defaults);
    }
    for (const auto& selection : std::vector<std::vector<std::string>>{{"en"}, {"nb"}, {"nb", "en"}, {"en", "nb"}, {"it"}, {"de"}, {"it", "de"}, {"de", "it"}}) {
        CHECK(profile.settings.displayLanguages.Set(selection));
        const auto decoded = DecodeProfile(EncodeProfile(profile));
        CHECK(decoded.format == ProfileFormat::Valid && decoded.version == 6 && !decoded.skippedRecords);
        CHECK(decoded.profile.settings.displayLanguages.Locales() == selection);
        ClearHistory(profile);
        CHECK(profile.settings.displayLanguages.Locales() == selection);
    }
    for (const auto& record : {"display_languages", "display_languages\ten\ten", "display_languages\ten\tnb\tde", "display_languages\tzz", "display_languages\ten\t"}) {
        const auto decoded = DecodeProfile(std::string("SwashMoji\t5\n") + record + "\nend\t1\n");
        CHECK(decoded.format == ProfileFormat::Valid && decoded.skippedRecords == 1);
        CHECK(decoded.profile.settings.displayLanguages.Locales() == defaults);
    }
    const auto duplicate = DecodeProfile("SwashMoji\t5\ndisplay_languages\tnb\ndisplay_languages\ten\nend\t2\n");
    CHECK(duplicate.skippedRecords == 1 && duplicate.profile.settings.displayLanguages.Locales() == std::vector<std::string>{"nb"});
    const std::string v4 = "SwashMoji\t4\nsetting\tskin_tone\t3\nrecent\tcombination\tcombo-1\nalias\tlaunch\tcombination\tcombo-1\ncombination\tcombo-1\tLift off\t🚀✨\t🚀\t🚀\t✨\t✨\nend\t4\n";
    const auto path = root / L"languages";
    Write(path / L"profile.tsv", v4);
    ProfileStorage storage(path);
    auto loaded = storage.Load();
    CHECK(loaded.migrated && !loaded.unsaved);
    CHECK(loaded.profile.settings.displayLanguages.Locales() == defaults);
    CHECK(loaded.profile.settings.skinTone == 3 && loaded.profile.combinations.at(L"combo-1").payload == L"🚀✨");
    CHECK(loaded.profile.aliases.at(L"launch").target.value == L"combo-1");
    CHECK(Read(path / L"profile.tsv.bak") == v4);
    CHECK(DecodeProfile(Read(path / L"profile.tsv")).version == 6);
    CHECK(!storage.Load().migrated);
    CHECK(loaded.profile.settings.displayLanguages.Set({"nb"}));
    std::wstring diagnostic;
    CHECK(storage.Save(loaded.profile, diagnostic));
    CHECK(storage.Load().profile.settings.displayLanguages.Locales() == std::vector<std::string>{"nb"});
    CHECK(storage.Save(loaded.profile, diagnostic));
    Write(path / L"profile.tsv", "SwashMoji\t5\n");
    const auto recovered = storage.Load();
    CHECK(recovered.recovered && recovered.profile.settings.displayLanguages.Locales() == std::vector<std::string>{"nb"});
}

void Migration(const fs::path& root) {
    const auto v1Directory = root / L"version-one";
    const std::string v1 = "SwashMoji\t1\nsetting\tskin_tone\t3\nusage\t👍🏽\t8\nrecent\t👍🏽\nend\t3\n";
    Write(v1Directory / L"profile.tsv", v1);
    ProfileStorage oldStore(v1Directory);
    const auto upgraded = oldStore.Load();
    CHECK(upgraded.migrated && !upgraded.unsaved);
    CHECK(upgraded.profile.settings.skinTone == 3 && UsageCount(upgraded.profile, L"👍🏽") == 8);
    CHECK(upgraded.profile.history == (std::vector<ResultId>{{ResultKind::Emoji, L"👍🏽"}}));
    CHECK(Read(v1Directory / L"profile.tsv.bak") == v1);
    CHECK(DecodeProfile(Read(v1Directory / L"profile.tsv")).version == 6);
    CHECK(!oldStore.Load().migrated);
    const auto directory = root / L"migration";
    const auto fallback = root / L"WinMoji";
    const std::string settings = "position_above_text_field=1\r\nsort_by_usage=1\r\nemoji_rows=3\r\nskin_tone=4\r\n";
    Write(directory / L"settings.txt", settings);
    Write(fallback / L"history.txt", "👍🏽\r\n🚀\r\n");
    Write(fallback / L"usage.txt", "👍🏽\t9\r\n🚀\t3\r\nbad\t-1\r\n");
    ProfileStorage store(directory, fallback);
    auto loaded = store.Load();
    CHECK(loaded.migrated && !loaded.unsaved && !store.ReadOnly());
    CHECK(loaded.profile.settings.skinTone == 4 && loaded.profile.settings.emojiRows == 3);
    CHECK(loaded.profile.history == (std::vector<ResultId>{{ResultKind::Emoji, L"👍🏽"}, {ResultKind::Emoji, L"🚀"}}));
    CHECK(UsageCount(loaded.profile, L"👍🏽") == 9);
    CHECK(!loaded.diagnostic.empty());
    CHECK(Read(directory / L"settings.txt") == settings);
    CHECK(fs::exists(fallback / L"history.txt"));
    CHECK(!fs::exists(directory / L"history.txt"));
    auto again = store.Load();
    CHECK(!again.migrated && again.profile.usage == loaded.profile.usage);
    Remember(again.profile, L"🚀");
    std::wstring diagnostic;
    CHECK(store.Save(again.profile, diagnostic));
    CHECK(UsageCount(store.Load().profile, L"🚀") == 4);
    CHECK(Read(directory / L"settings.txt") == settings);
}

void Recovery(const fs::path& root) {
    const auto directory = root / L"recovery";
    ProfileStorage store(directory);
    auto loaded = store.Load();
    CHECK(loaded.migrated);
    Remember(loaded.profile, L"🚀");
    std::wstring diagnostic;
    CHECK(store.Save(loaded.profile, diagnostic));
    const auto first = Read(directory / L"profile.tsv");
    Remember(loaded.profile, L"🚀");
    CHECK(store.Save(loaded.profile, diagnostic));
    CHECK(Read(directory / L"profile.tsv.bak") == first);
    Write(directory / L"profile.tsv.tmp.interrupted", "SwashMoji\t1\nusage\t");
    CHECK(UsageCount(store.Load().profile, L"🚀") == 2);
    Write(directory / L"profile.tsv", "SwashMoji\t1\nusage\t");
    auto recovered = store.Load();
    CHECK(recovered.recovered && !recovered.unsaved);
    CHECK(UsageCount(recovered.profile, L"🚀") == 1);
    CHECK(Read(directory / L"profile.tsv") == first);
    CHECK(Read(directory / L"profile.tsv.bak") == first);
    fs::remove(directory / L"profile.tsv");
    CHECK(store.Load().recovered);
}

void ProtectFutureAndCorrupt(const fs::path& root) {
    const auto directory = root / L"future";
    Write(directory / L"profile.tsv", "SwashMoji\t99\nfuture-data\n");
    Write(directory / L"profile.tsv.bak", EncodeProfile(Profile{}));
    ProfileStorage store(directory);
    CHECK(!store.Load().diagnostic.empty() && store.ReadOnly());
    std::wstring diagnostic;
    CHECK(!store.Save(Profile{}, diagnostic));
    CHECK(Read(directory / L"profile.tsv") == "SwashMoji\t99\nfuture-data\n");
    const auto broken = root / L"broken";
    Write(broken / L"profile.tsv", "corrupt");
    ProfileStorage corrupt(broken);
    CHECK(!corrupt.Load().diagnostic.empty() && corrupt.ReadOnly());
    CHECK(!corrupt.Save(Profile{}, diagnostic));
    CHECK(Read(broken / L"profile.tsv") == "corrupt");
    const auto futureBackup = root / L"future-backup";
    Write(futureBackup / L"profile.tsv.bak", "SwashMoji\t99");
    ProfileStorage backupStore(futureBackup);
    CHECK(!backupStore.Load().diagnostic.empty() && backupStore.ReadOnly());
    CHECK(!backupStore.Save(Profile{}, diagnostic));
    CHECK(!fs::exists(futureBackup / L"profile.tsv"));
}

void FailedWrites(const fs::path& root) {
    const auto directory = root / L"locked";
    ProfileStorage store(directory);
    auto loaded = store.Load();
    const auto previous = Read(directory / L"profile.tsv");
    const HANDLE lock = CreateFileW((directory / L"profile.tsv").c_str(), GENERIC_READ,
                                    FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    CHECK(lock != INVALID_HANDLE_VALUE);
    Remember(loaded.profile, L"🚀");
    std::wstring diagnostic;
    const bool saved = store.Save(loaded.profile, diagnostic);
    CloseHandle(lock);
    CHECK(!saved && !diagnostic.empty());
    CHECK(UsageCount(loaded.profile, L"🚀") == 1); // in-memory state survives
    CHECK(Read(directory / L"profile.tsv") == previous);
    CHECK(store.Save(loaded.profile, diagnostic));
    CHECK(UsageCount(store.Load().profile, L"🚀") == 1);
    for (const auto& entry : fs::directory_iterator(directory)) CHECK(entry.path().filename().wstring().find(L".tmp.") == std::wstring::npos);

    const auto blocked = root / L"blocked";
    Write(blocked, "not a directory");
    ProfileStorage failed(blocked);
    CHECK(!failed.Save(loaded.profile, diagnostic));
    CHECK(Read(blocked) == "not a directory");
}

int main() {
    try {
        TestDirectory directory;
        Codec(); ImportExport(directory.path); ActivationSettings(directory.path); Migration(directory.path); FamilyMigration(directory.path); LanguageSettings(directory.path); Recovery(directory.path);
        ProtectFutureAndCorrupt(directory.path); FailedWrites(directory.path);
        std::cout << "Profile codec, migration, recovery and write-failure checks passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
