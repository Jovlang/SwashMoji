#include "test_support.h"
#include "search.h"
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>

using namespace SwashMoji;

Catalog Fixture() {
    std::istringstream input(
        "😀\tgrinning face\tsmile | happy | joy\n"
        "😊\tsmiling face\tsmile | happy | blush\n"
        "😂\tface with tears of joy\tlol | laugh | happy\n"
        "🚀\trocket\tlaunch | ship\n"
        "👍\tthumbs up\tgood | nice\n"
        "👍🏽\tthumbs up: medium skin tone\tgood | nice | medium skin tone\n"
        "🏽\tmedium skin tone\tmedium | skin\n"
        "👩‍💻\twoman technologist\tdeveloper\n"
        "👨‍💻\tman technologist\tdeveloper\n");
    Catalog catalog;
    CHECK(catalog.Load(input));
    return catalog;
}

void SearchBehavior() {
    const auto catalog = Fixture();
    Profile profile;
    CHECK(Search(catalog, profile, L"rocket").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"ROCKET").front().match.tier == 7);
    CHECK(Search(catalog, profile, L"rock").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"launch").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"rcoket").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"rocxet").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"grinning rocket").empty());
    CHECK(Search(catalog, profile, L"zzzzzzzz").empty());
    CHECK(Search(catalog, profile, L"lol").front().payload == L"😂");
    CHECK(Search(catalog, profile, L"laughing").front().payload == L"😂");
    CHECK(Search(catalog, profile, L"grinning face").front().payload == L"😀");
    auto results = Search(catalog, profile, L"happy");
    CHECK(results.size() == 3);
    CHECK(results[0].payload == L"😊"); // existing popularity tiebreak
    Remember(profile, L"😀");
    Remember(profile, L"😀");
    Remember(profile, L"😂");
    CHECK(Search(catalog, profile, L"happy").front().payload == L"😂");
    CHECK(Search(catalog, profile, L"").front().payload == L"😂");
    profile.settings.sortByUsage = true;
    CHECK(Search(catalog, profile, L"happy").front().payload == L"😀");
    CHECK(Search(catalog, profile, L"").front().payload == L"😀");
    CHECK(Search(catalog, profile, L"smiling face").front().payload == L"😊");
    profile.settings.skinTone = 3;
    results = Search(catalog, profile, L"thumbs up");
    CHECK(results.size() == 1);
    CHECK(results[0].payload == L"👍🏽");
    CHECK(results[0].id == (ResultId{ResultKind::Emoji, L"👍"}));
    CHECK(results[0].label == L"thumbs up: medium skin tone");
    for (const auto& result : Search(catalog, profile, L"")) CHECK(!result.payload.empty());
}

void CatalogIdentity() {
    auto catalog = Fixture();
    CHECK(catalog.Find(L"👍")->family == catalog.Find(L"👍🏽")->family);
    CHECK(!(catalog.Find(L"👩‍💻")->family == catalog.Find(L"👨‍💻")->family));
    CHECK(catalog.Find(L"🏽")->family.value == L"🏽");
    CHECK(!catalog.Find(L""));
    const auto id = catalog.Find(L"🚀")->family;
    auto moved = std::move(catalog);
    CHECK(moved.Find(L"🚀")->family == id);
    auto copied = moved;
    CHECK(copied.Find(L"🚀")->family == id);
    std::istringstream replacement("🚀 rocket\nmalformed\n");
    CHECK(catalog.Load(replacement));
    CHECK(catalog.Entries().size() == 1);
    CHECK(catalog.Find(L"🚀")->family == id);
    std::istringstream bad("\xff\tbad\n");
    CHECK(!catalog.Load(bad));
}

void Personalization() {
    Profile profile;
    for (int i = 0; i < 50; ++i) Remember(profile, std::to_wstring(i));
    CHECK(profile.history.size() == kMaxHistory);
    CHECK(profile.history.front() == L"49");
    CHECK(profile.history.back() == L"10");
    Remember(profile, L"25");
    CHECK(profile.history.front() == L"25");
    CHECK(profile.history.size() == kMaxHistory);
    CHECK(UsageCount(profile, L"25") == 2);
    profile.usage[L"25"] = std::numeric_limits<unsigned int>::max();
    Remember(profile, L"25");
    CHECK(UsageCount(profile, L"25") == std::numeric_limits<unsigned int>::max());
    CHECK(HistoryBoost(profile, L"25") == 20);
    CHECK(HistoryBoost(profile, L"missing") == 0);
    profile.settings.skinTone = 3;
    ClearHistory(profile);
    CHECK(profile.history.empty() && profile.usage.empty());
    CHECK(profile.settings.skinTone == 3);
}

void BundledCatalog(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    Catalog catalog;
    CHECK(catalog.Load(input));
    CHECK(catalog.Entries().size() > 1000);
    Profile profile;
    for (const auto& query : {L"rocket", L"grinning face", L"thumbs up", L"lol"}) {
        CHECK(!Search(catalog, profile, query).empty());
    }
    for (int tone = 0; tone <= 5; ++tone) {
        profile.settings.skinTone = tone;
        std::set<ResultId> ids;
        for (const auto& result : Search(catalog, profile, L"")) {
            CHECK(!result.payload.empty());
            CHECK(catalog.Find(result.payload));
            CHECK(ids.insert(result.id).second);
            CHECK(!result.label.empty());
        }
    }
}

int main(int argc, char** argv) {
    try {
        SearchBehavior(); CatalogIdentity(); Personalization();
        CHECK(argc == 2);
        BundledCatalog(std::filesystem::u8path(argv[1]));
        std::cout << "Catalog, search, stable identity and history checks passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
