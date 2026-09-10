#include "test_support.h"
#include "search.h"
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>

using namespace SwashMoji;

void LocalizedNames() {
    Emoji emoji;
    CHECK(FormatEmojiDisplayName(emoji).empty());
    SetEmojiLocalization(emoji, "en", L"smiling face", L"smiles | happy");
    CHECK(FormatEmojiDisplayName(emoji, "en", "") == L"smiling face");
    CHECK(FormatEmojiDisplayName(emoji) == L"smiling face");
    SetEmojiLocalization(emoji, "nb", L"smiler litt", L"glad");
    CHECK(FormatEmojiDisplayName(emoji) == L"smiling face · smiler litt");
    CHECK(FormatEmojiDisplayName(emoji, "fr", "nb") == L"smiler litt");
    CHECK(FormatEmojiDisplayName(emoji, "fr", "de") == L"smiling face");
    CHECK(FormatEmojiDisplayName(emoji, "nb", "nb") == L"smiler litt");
    CHECK(GetBestEmojiName(emoji, {"fr", "nb"}) == L"smiler litt");
    CHECK(LexicalScore(emoji, SplitWords(L"smiling face")).tier == 8);
    CHECK(LexicalScore(emoji, SplitWords(L"smiler litt")).tier == 8);
    SetEmojiLocalization(emoji, "de", L"lächelndes Gesicht", L"fröhlich");
    CHECK(FormatEmojiDisplayName(emoji, "de", "nb") == L"lächelndes Gesicht · smiler litt");
    CHECK(LexicalScore(emoji, SplitWords(NormalizePhrase(L"lächelndes Gesicht")), {"de"}).tier == 8);
    CHECK(LexicalScore(emoji, SplitWords(L"fröhlich"), {"en", "nb", "de"}).tier == 4);
    CHECK(LexicalScore(emoji, SplitWords(L"fröhlich"), {"en", "nb"}).tier == 0);
    CHECK(FuzzyScore(emoji, SplitWords(L"fröhlih"), {"de"}) >= 0);
    CHECK(FuzzyScore(emoji, SplitWords(L"fröhlih"), {"en"}) == -1);
    SetEmojiLocalization(emoji, "de", L"smiling face");
    CHECK(FormatEmojiDisplayName(emoji, "en", "de") == L"smiling face");
    CHECK(LexicalScore(emoji, SplitWords(L"fröhlich"), {"de"}).tier == 0); // caches replaced

    Catalog catalog;
    std::istringstream input("🙂\tsmiling face\tsmile\tsmiler litt\tglad\n🚀\t\t\trakett\tromskip\n");
    CHECK(catalog.Load(input));
    CHECK(GetEmojiName(*catalog.Find(L"🙂"), "nb") == L"smiler litt");
    CHECK(FormatEmojiDisplayName(*catalog.Find(L"🚀")) == L"rakett");
    CHECK(Search(catalog, Profile{}, L"smiler litt").front().label == L"smiling face · smiler litt");
    CHECK(Search(catalog, Profile{}, L"smiler litt", nullptr, {"en"}).empty());
    Profile profile;
    CHECK(profile.settings.displayLanguages.Set({"nb"}));
    const auto result = Search(catalog, profile, L"smiling face").front();
    CHECK(result.label == L"smiler litt" && result.match.tier == 8);
    CHECK(Search(catalog, profile, L"🙂").front().label == L"smiler litt");
    CHECK(SetAlias(profile, catalog, L"my smile", result.id) == AliasResult::Saved);
    CHECK(Search(catalog, profile, L"my smile").front().label == L"smiler litt");
    CHECK(Pin(profile, catalog, result.id) == PinResult::Pinned);
    CHECK(Search(catalog, profile, L"").front().label == L"smiler litt");
    CHECK(profile.settings.displayLanguages.Set({"en"}));
    CHECK(Search(catalog, profile, L"smiler litt").front().label == L"smiling face");
}

void PreferredSearchLanguages() {
    Catalog catalog;
    std::istringstream input(
        "👧\tgirl\tchild\tjente\tbarn\tit\tragazza\tfanciulla\n"
        "🚀\trocket\tlaunch\trakett\tromskip\tit\trazzo\tspazio\n"
        "😀\tsunshine\tbright\t\t\tit\tstella\tluce\n"
        "😊\tmoonbeam\tdim\t\t\tit\tsunshine\tbagliore\n"
        "☕\tragazzaria\tcompanion\t\t\tit\tcaffè\tbevanda\n");
    CHECK(catalog.Load(input));
    Profile profile;
    auto results = Search(catalog, profile, L"ragazza");
    CHECK(results.size() == 2);
    CHECK(results[0].payload == L"👧" && results[0].match.tier == 8);
    CHECK(results[1].payload == L"☕" && results[1].match.tier == 6);
    CHECK(results[0].label == L"girl · jente");
    CHECK(Search(catalog, profile, L"fanciulla").front().match.tier == 4);
    results = Search(catalog, profile, L"ragazz");
    CHECK(results.size() == 2 && results[1].payload == L"👧"); // Prefix survives; preferred prefix leads.
    CHECK(Search(catalog, profile, L"rocxet").front().match.tier == 1);
    CHECK(Search(catalog, profile, L"raxett").front().match.tier == 1); // Secondary locale too.
    CHECK(Search(catalog, profile, L"ragazxa").empty());
    CHECK(Search(catalog, profile, L"fanciulxa").empty());
    CHECK(Search(catalog, profile, L"sunshine").front().payload == L"😀"); // Beats popularity on a tie.
    Remember(profile, L"😊");
    CHECK(Search(catalog, profile, L"sunshine").front().payload == L"😊"); // Learning order retained.
    ClearHistory(profile);
    CHECK(profile.settings.displayLanguages.Set({"it"}));
    CHECK(Search(catalog, profile, L"ragazxa").front().label == L"ragazza");
    CHECK(Search(catalog, profile, L"rocxet").empty());
    CHECK(Search(catalog, profile, L"sunshine").front().payload == L"😊");

    // Search preference can have any length and does not change display labels.
    const SearchLanguagePolicy policy{{"en", "nb", "it", "unknown"}};
    CHECK(Search(catalog, profile, L"rocxet", nullptr, {}, &policy).front().label == L"razzo");
    CHECK(Search(catalog, profile, L"rocxet", nullptr, {"it"}, &policy).empty());
    CHECK(Search(catalog, profile, L"ragazxa", nullptr, {"en"}).empty()); // Empty intersection is not all.
    CHECK(Search(catalog, profile, L"ragazza", nullptr, {"en"}).front().payload == L"☕");
    const SearchLanguagePolicy none{};
    CHECK(Search(catalog, profile, L"ragazxa", nullptr, {}, &none).empty());
    CHECK(Search(catalog, profile, L"ragazza", nullptr, {}, &none).front().payload == L"👧");

    // Curated and personal phrases have no locale tags and stay available.
    std::istringstream intents("partenza stellare\t🚀\n");
    CHECK(catalog.LoadIntents(intents));
    CHECK(profile.settings.displayLanguages.Set({"en", "nb"}));
    CHECK(Search(catalog, profile, L"partenza stellare").front().match.tier == 7);
    CHECK(Search(catalog, profile, L"partenza stel").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"partenxa stellare").front().match.tier == 1);
    CHECK(SetAlias(profile, catalog, L"ragazza", {ResultKind::Emoji, L"🚀"}) == AliasResult::Saved);
    CHECK(Search(catalog, profile, L"ragazza").front().match.tier == 9);
}

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
    CHECK(Search(catalog, profile, L"ROCKET").front().match.tier == 8);
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
    std::istringstream multilingual("🚀\trocket\tlaunch\trakett\tromskip\tde\tRakete\tWeltraum\tit\trazzo\tspazio\tfr\tfusée\tespace\n"
        "🙂\tsmile\t\t\t\tde\tbroken\n" // Incomplete locale triple.
        "☕\tcoffee\t\t\t\ten\toverwrite\tbad\n"); // Duplicate canonical locale.
    CHECK(catalog.Load(multilingual));
    CHECK(catalog.Entries().size() == 1);
    CHECK(GetEmojiName(*catalog.Find(L"🚀"), "de") == L"Rakete");
    CHECK(GetEmojiName(*catalog.Find(L"🚀"), "it") == L"razzo");
    CHECK(GetEmojiName(*catalog.Find(L"🚀"), "fr") == L"fusée");
    CHECK(Search(catalog, Profile{}, L"espace").front().payload == L"🚀");
}

void Personalization() {
    Profile profile;
    for (int i = 0; i < 50; ++i) Remember(profile, std::to_wstring(i));
    CHECK(profile.history.size() == kMaxHistory);
    CHECK(profile.history.front().value == L"49");
    CHECK(profile.history.back().value == L"10");
    Remember(profile, L"25");
    CHECK(profile.history.front().value == L"25");
    CHECK(profile.history.size() == kMaxHistory);
    CHECK(UsageCount(profile, L"25") == 2);
    profile.usage[{ResultKind::Emoji, L"25"}] = std::numeric_limits<unsigned int>::max();
    Remember(profile, L"25");
    CHECK(UsageCount(profile, L"25") == std::numeric_limits<unsigned int>::max());
    CHECK(HistoryBoost(profile, L"25") == kMaxHistory);
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
        LocalizedNames(); PreferredSearchLanguages(); SearchBehavior(); CatalogIdentity(); Personalization();
        CHECK(argc == 2);
        BundledCatalog(std::filesystem::u8path(argv[1]));
        std::cout << "Catalog, search, stable identity and history checks passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
