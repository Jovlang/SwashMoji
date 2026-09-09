// Exercise the distributed data and a complete choice/restart/recovery path.
// Both profile roots are explicit and unique; never resolve LOCALAPPDATA.
#include <windows.h>
#include "test_support.h"
#include "catalog.h"
#include "search.h"
#include "storage.h"
#include <fstream>

using namespace SwashMoji;
namespace fs = std::filesystem;

namespace {
std::string Read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary); CHECK(input.good());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void Write(const fs::path& path, const std::string& bytes) {
    std::ofstream output(path, std::ios::binary); output << bytes; CHECK(output.good());
}
}

int wmain(int argc, wchar_t** argv) {
    try {
        CHECK(argc == 2);
        const auto package = fs::path(argv[1]);
        CHECK(fs::file_size(package / "SwashMoji.exe") > 0);
        CHECK(!Read(package / "UNICODE_LICENSE.txt").empty());
        Catalog catalog;
        std::ifstream emojis(package / "emojis.txt"), intents(package / "intent_phrases.tsv");
        CHECK(catalog.Load(emojis) && catalog.LoadIntents(intents));
        const auto root = fs::current_path() / (L"release-test-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        CHECK(fs::create_directory(root));
        ProfileStorage storage(root, root / "legacy");
        auto loaded = storage.Load(&catalog); CHECK(!loaded.unsaved);
        auto profile = loaded.profile;
        CHECK(profile.aliases.empty() && profile.combinations.empty());
        for (const auto* query : {L"rocket", L"rakett", L"thank you", L"takk"})
            CHECK(!Search(catalog, profile, query).empty());
        Combination combination; combination.name = L"launch";
        for (const auto* glyph : {L"🚀", L"✨"}) {
            const auto* emoji = catalog.Find(glyph); CHECK(emoji);
            combination.entries.push_back({emoji->family.value, emoji->glyph});
        }
        std::wstring error;
        CHECK(SaveCombination(profile, catalog, combination, error));
        const ResultId id{ResultKind::Combination, combination.id};
        CHECK(SetAlias(profile, catalog, L"på vei hjem", id) == AliasResult::Saved);
        CHECK(Pin(profile, catalog, id) == PinResult::Pinned);
        RecordChoice(profile, id, L"på vei hjem");
        CHECK(storage.Save(profile, error));
        auto restarted = storage.Load(&catalog); CHECK(!restarted.unsaved);
        CHECK(EncodeProfile(restarted.profile) == EncodeProfile(profile));
        const auto results = Search(catalog, restarted.profile, L"PÅ VEI HJEM");
        CHECK(!results.empty() && results.front().id == id && results.front().payload == L"🚀✨");
        // Rotate the authored state into backup, then recover it from truncation.
        CHECK(storage.Save(profile, error));
        Write(root / "profile.tsv", "SwashMoji\t4\n");
        const auto recovered = storage.Load(&catalog);
        CHECK(recovered.recovered && !recovered.unsaved);
        CHECK(EncodeProfile(recovered.profile) == EncodeProfile(profile));
        // An unsupported primary must take precedence over the valid backup.
        const std::string future = "SwashMoji\t999\nend\t0\n";
        Write(root / "profile.tsv", future);
        storage.Load(&catalog); CHECK(storage.ReadOnly());
        CHECK(!storage.Save(profile, error) && Read(root / "profile.tsv") == future);
        const auto migration = root / "migration"; CHECK(fs::create_directory(migration));
        const std::string old = "SwashMoji\t1\nusage\t🚀\t7\nrecent\t🚀\nend\t2\n";
        Write(migration / "profile.tsv", old);
        ProfileStorage oldStorage(migration, root / "legacy");
        const auto migrated = oldStorage.Load(&catalog);
        CHECK(migrated.migrated && !migrated.unsaved);
        CHECK(UsageCount(migrated.profile, L"🚀") == 7);
        CHECK(Read(migration / "profile.tsv.bak") == old);
        CHECK(!oldStorage.Load(&catalog).migrated);
        CHECK(root.parent_path() == fs::current_path());
        fs::remove_all(root);
        std::cout << "PASS: package data, bilingual search, authored choice/restart, migration, backup recovery and future-version protection.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
