#include <windows.h>
#include "personalization.h"
#include "catalog.h"
#include "storage.h"
#include "search.h"
#include "test_support.h"
#include <fstream>
#include <filesystem>
#include <sstream>
using namespace SwashMoji;
Combination Draft(const Catalog& catalog, const wchar_t* name, std::initializer_list<const wchar_t*> glyphs) {
    Combination c; c.name = name;
    for (const auto* glyph : glyphs) {
        const auto* emoji = catalog.Find(glyph); CHECK(emoji);
        c.entries.push_back({emoji->family.value, emoji->glyph});
    }
    return c;
}
int main(int argc, char** argv) {
    try {
        CHECK(argc == 2); Catalog catalog; std::ifstream file(std::filesystem::u8path(argv[1]), std::ios::binary); CHECK(catalog.Load(file));
        Profile p; std::wstring error;
        auto launch = Draft(catalog, L"Launch", {L"🚀", L"✨"});
        auto please = Draft(catalog, L"please", {L"🥺", L"🙏🏽"});
        CHECK(SaveCombination(p, catalog, launch, error)); CHECK(SaveCombination(p, catalog, please, error));
        CHECK(launch.id != please.id && launch.payload == L"🚀✨");
        ResultId id{ResultKind::Combination, launch.id};
        CHECK(Search(catalog, p, L"LAUNCH")[0].id == id);
        CHECK(Search(catalog, p, L"launc")[0].id == id);
        CHECK(Search(catalog, p, L"launxh")[0].id == id);
        CHECK(SetAlias(p, catalog, L"go now", id) == AliasResult::Saved);
        CHECK(Search(catalog, p, L"go now")[0].id == id);
        CHECK(Pin(p, catalog, id) == PinResult::Pinned);
        CHECK(Search(catalog, p, L"")[0].id == id);
        RecordChoice(p, id, L"go now"); RecordChoice(p, id, L"go now");
        const auto old = launch.id; launch.name = L"Lift off";
        CHECK(SaveCombination(p, catalog, launch, error)); CHECK(launch.id == old);
        CHECK(UsageCount(p, id) == 2 && QueryCount(p, L"go now", id) == 2 && IsPinned(p, id));
        CHECK(Search(catalog, p, L"lift off")[0].payload == L"🚀✨");
        CHECK(SetAlias(p, catalog, L"LIFT-OFF", id, {}, true) == AliasResult::InvalidPhrase);
        auto duplicate = please; duplicate.name = L"lift OFF"; const auto before = EncodeProfile(p);
        CHECK(!SaveCombination(p, catalog, duplicate, error)); CHECK(EncodeProfile(p) == before);
        duplicate.name = L"go now"; CHECK(!SaveCombination(p, catalog, duplicate, error));
        duplicate.name = L"!!!"; CHECK(!SaveCombination(p, catalog, duplicate, error));
        duplicate.name = std::wstring(97, L'a'); CHECK(!SaveCombination(p, catalog, duplicate, error));
        duplicate = please; duplicate.entries.resize(1); CHECK(!SaveCombination(p, catalog, duplicate, error));
        duplicate.entries.resize(9, please.entries[0]); CHECK(!SaveCombination(p, catalog, duplicate, error));
        auto complex = Draft(catalog, L"på vei", {L"👩🏽‍💻", L"❤️", L"🙏🏻", L"🚀", L"✨", L"🥺", L"👨‍👧‍👦", L"👍🏿"});
        CHECK(SaveCombination(p, catalog, complex, error));
        const auto exact = complex.payload;
        for (int tone = 0; tone <= 5; ++tone) {
            p.settings.skinTone = tone;
            CHECK(Search(catalog, p, L"på vei")[0].payload == exact);
            CHECK(Search(catalog, p, L"please")[0].payload == L"🥺🙏🏽");
        }
        auto decoded = DecodeProfile(EncodeProfile(p));
        CHECK(decoded.format == ProfileFormat::Valid && decoded.version == 4 && decoded.skippedRecords == 0);
        CHECK(EncodeProfile(decoded.profile) == EncodeProfile(p));
        Catalog missing; CHECK(Search(missing, decoded.profile, L"på vei")[0].payload == exact);
        auto renamed = decoded.profile.combinations.at(complex.id); renamed.name = L"retained";
        CHECK(SaveCombination(decoded.profile, missing, renamed, error)); CHECK(renamed.payload == exact);
        std::swap(complex.entries[0], complex.entries[7]); CHECK(SaveCombination(p, catalog, complex, error));
        CHECK(complex.payload != exact && complex.payload.substr(0, complex.entries[0].payload.size()) == complex.entries[0].payload);
        auto corrupt = complex; corrupt.payload += L"x"; CHECK(!ValidCombination(corrupt));
        corrupt = complex; corrupt.entries[0].payload = std::wstring(1, 0xD800); CHECK(!ValidCombination(corrupt));
        auto badProfile = p; badProfile.combinations[complex.id].payload = L"bad";
        CHECK(DecodeProfile(EncodeProfile(badProfile)).skippedRecords == 1);
        const auto v3 = DecodeProfile("SwashMoji\t3\nsetting\tskin_tone\t5\nend\t1\n");
        CHECK(v3.format == ProfileFormat::Valid && v3.profile.settings.skinTone == 5);
        CHECK(DecodeProfile(EncodeProfile(v3.profile)).version == 4);
        const auto directory = std::filesystem::current_path() / (L"combination-storage-test-" + std::to_wstring(GetCurrentProcessId()));
        CHECK(!std::filesystem::exists(directory)); CHECK(std::filesystem::create_directory(directory));
        const std::string legacy = "SwashMoji\t3\nsetting\tskin_tone\t5\nend\t1\n";
        { std::ofstream out(directory / L"profile.tsv", std::ios::binary); out << legacy; CHECK(out.good()); }
        ProfileStorage storage(directory); const auto migrated = storage.Load(&catalog);
        CHECK(migrated.migrated && migrated.profile.settings.skinTone == 5);
        { std::ifstream backup(directory / L"profile.tsv.bak", std::ios::binary);
          CHECK(std::string(std::istreambuf_iterator<char>(backup), {}) == legacy); }
        CHECK(storage.Save(p, error)); CHECK(EncodeProfile(storage.Load(&catalog).profile) == EncodeProfile(p));
        CHECK(directory.parent_path() == std::filesystem::current_path()); std::filesystem::remove_all(directory);
        ClearHistory(p); CHECK(p.combinations.size() == 3 && p.aliases.size() == 1 && IsPinned(p, id));
        RecordChoice(p, id, L"go now");
        CHECK(DeleteCombination(p, launch.id));
        CHECK(!IsPinned(p, id) && !UsageCount(p, id) && !QueryCount(p, L"go now", id) && p.aliases.empty() && p.history.empty());
        CHECK(p.combinations.size() == 2 && !DeleteCombination(p, launch.id));
        for (size_t i = p.combinations.size(); i < kMaxCombinations; ++i) {
            auto c = please; c.id.clear(); c.name = L"item " + std::to_wstring(i); CHECK(SaveCombination(p, catalog, c, error));
        }
        auto excess = please; excess.id.clear(); excess.name = L"excess"; CHECK(!SaveCombination(p, catalog, excess, error));
        CHECK(SaveCombination(p, catalog, please, error)); // Edits remain possible at the limit.
        std::cout << "PASS: combination lifecycle, search, identity, limits, Unicode, tone isolation, storage and cascade\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
