#include "test_support.h"
#include "search.h"
#include "storage.h"
#include <fstream>
#include <sstream>
#include <set>

using namespace SwashMoji;

void Aliases(const Catalog& catalog) {
    Profile profile;
    const ResultId coffee{ResultKind::Emoji, catalog.Find(L"☕")->family.value};
    const ResultId rocket{ResultKind::Emoji, catalog.Find(L"🚀")->family.value};
    CHECK(SetAlias(profile, catalog, L"PÅ  vei!", coffee) == AliasResult::Saved);
    CHECK(SetAlias(profile, catalog, L"pa\u030a-vei", rocket) == AliasResult::Duplicate);
    CHECK(profile.aliases.size() == 1 && profile.aliases.at(L"på vei").target == coffee);
    CHECK(Search(catalog, profile, L"på vei").front().id == coffee);
    CHECK(SetAlias(profile, catalog, L"release time", rocket) == AliasResult::Saved);
    CHECK(SetAlias(profile, catalog, L"release time", coffee, L"på vei") == AliasResult::Duplicate);
    CHECK(profile.aliases.size() == 2); // Declining replacement leaves both intact.
    CHECK(SetAlias(profile, catalog, L"release time", coffee, L"på vei", true) == AliasResult::Saved);
    CHECK(profile.aliases.size() == 1 && profile.aliases.at(L"release time").target == coffee);
    CHECK(SetAlias(profile, catalog, L"rocket", coffee, L"release time") == AliasResult::Saved);
    for (int i = 0; i < 10; ++i) Remember(profile, L"🚀");
    const auto overridden = Search(catalog, profile, L"rocket");
    CHECK(overridden.front().id == coffee && overridden.front().match.tier == 9);
    CHECK(overridden.front().explanation == L"Alias: rocket");
    CHECK(SetAlias(profile, catalog, L"coffee time", rocket) == AliasResult::Saved);
    CHECK(Search(catalog, profile, L"coffee ti").front().id == rocket);
    CHECK(SetAlias(profile, catalog, L"!!!", rocket) == AliasResult::InvalidPhrase);
    CHECK(SetAlias(profile, catalog, std::wstring(97, L'a'), rocket) == AliasResult::InvalidPhrase);
    CHECK(SetAlias(profile, catalog, L"invalid", {ResultKind::Emoji, L"missing"}) == AliasResult::InvalidTarget);
    CHECK(SetAlias(profile, catalog, L"invalid", {ResultKind::Combination, L"future"}) == AliasResult::InvalidTarget);
    CHECK(SetAlias(profile, catalog, L"renamed", coffee, L"gone") == AliasResult::MissingOriginal);
    auto restarted = DecodeProfile(EncodeProfile(profile));
    CHECK(restarted.format == ProfileFormat::Valid && !restarted.skippedRecords);
    CHECK(Search(catalog, restarted.profile, L"rocket").front().id == coffee);
    ClearHistory(restarted.profile);
    CHECK(restarted.profile.aliases.size() == 2);
    CHECK(DeleteAlias(restarted.profile, L"ROCKET!"));
    CHECK(!DeleteAlias(restarted.profile, L"rocket"));
    restarted = DecodeProfile(EncodeProfile(restarted.profile));
    CHECK(Search(catalog, restarted.profile, L"rocket").front().id == rocket);
    CHECK(restarted.profile.aliases.size() == 1);
    for (size_t i = restarted.profile.aliases.size(); i < kMaxAliases; ++i)
        CHECK(SetAlias(restarted.profile, catalog, L"phrase " + std::to_wstring(i), coffee) == AliasResult::Saved);
    CHECK(SetAlias(restarted.profile, catalog, L"one too many", coffee) == AliasResult::LimitReached);
    CHECK(SetAlias(restarted.profile, catalog, L"updated phrase", coffee, L"coffee time") == AliasResult::Saved);
    auto damaged = DecodeProfile("SwashMoji\t2\nalias\tPÅ VEI\temoji\t🚶\nalias\tpå-vei\temoji\t☕\nalias\t!!!\temoji\t☕\nalias\tbad\tother\tx\nend\t4\n");
    CHECK(damaged.format == ProfileFormat::Valid && damaged.skippedRecords == 3);
    CHECK(damaged.profile.aliases.size() == 1);
}

void NormalizationAndRanking() {
    CHECK(NormalizePhrase(L"  Æ Ø Å—god\tidé! ") == L"æ ø å god idé");
    CHECK(NormalizePhrase(L"a\u030a e\u0301") == L"å é");
    CHECK(NormalizePhrase(L"på") != NormalizePhrase(L"pa"));
    std::istringstream data("🚀\trocket\tlaunch\trakett\tfjes\n☕\tcoffee\tcafe\tkaffe\tkafé\n");
    Catalog catalog;
    CHECK(catalog.Load(data));
    Profile profile;
    // Norwegian fields do not inherit the English trailing-s rule.
    CHECK(LexicalScore(*catalog.Find(L"🚀"), {L"fje"}).tier != 4);
    CHECK(Search(catalog, profile, L"fjes").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"kafe\u0301").front().payload == L"☕");
    CHECK(Search(catalog, profile, L"coffee rakett").empty());
    CHECK(SetAlias(profile, catalog, L"rocket crew", {ResultKind::Emoji, L"☕"}) == AliasResult::Saved);
    CHECK(Search(catalog, profile, L"rocket").front().payload == L"🚀");
    CHECK(Search(catalog, profile, L"rock").front().match.tier > 1); // No fuzzy fill alongside ordinary hits.
    std::istringstream intents("going to space\t🚀\n");
    CHECK(catalog.LoadIntents(intents));
    std::istringstream broken("coffee break\t☕\nmalformed\n");
    CHECK(!catalog.LoadIntents(broken));
    CHECK(Search(catalog, profile, L"going to space").front().payload == L"🚀");
    CHECK(catalog.Find(L"☕")->intents.empty());
}

void Corpus(const Catalog& catalog, const std::filesystem::path& root) {
    Profile profile;
    for (auto phrase : {L"min kaffepause", L"kaffepause nå"})
        CHECK(SetAlias(profile, catalog, phrase, {ResultKind::Emoji, catalog.Find(L"☕")->family.value}) == AliasResult::Saved);
    std::ifstream file(root / "tests/search_corpus.tsv", std::ios::binary);
    CHECK(file.good());
    std::string line;
    size_t count = 0, passed = 0, intents = 0, intentPassed = 0;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto tab = line.find('\t'), next = line.find('\t', tab + 1);
        CHECK(tab != std::string::npos && next != std::string::npos);
        const auto type = line.substr(0, tab), query = line.substr(tab + 1, next - tab - 1);
        const auto expected = Utf8ToWide(line.substr(next + 1));
        const auto results = Search(catalog, profile, Utf8ToWide(query));
        bool found = expected == L"-" && results.empty();
        for (size_t i = 0; i < std::min(size_t{3}, results.size()); ++i) found |= results[i].payload == expected;
        ++count;
        if (found) ++passed;
        if (type == "intent") { ++intents; if (found) ++intentPassed; }
        if (!found) {
            std::cerr << "Corpus miss: " << query << " expected " << WideToUtf8(expected) << "; got";
            for (size_t i = 0; i < std::min(size_t{3}, results.size()); ++i) std::cerr << " " << WideToUtf8(results[i].payload);
            std::cerr << '\n';
        }
        std::set<ResultId> unique;
        for (const auto& result : results) CHECK(unique.insert(result.id).second);
    }
    std::cout << "Corpus: " << passed << '/' << count << " top-three; intents " << intentPassed << '/' << intents << '\n';
    CHECK(count >= 100 && passed == count);
    // Every exact catalog name must retain its matching family in the exact-name tier.
    for (const auto& emoji : catalog.Entries()) {
        if (SkinToneIndex(emoji.glyph)) continue;
        for (const auto& locale : SupportedLocales()) {
            const auto name = GetEmojiName(emoji, locale.code);
            if (name.empty()) continue;
            const auto results = Search(catalog, Profile{}, name);
            bool exact = false;
            for (const auto& result : results) exact |= result.id.value == emoji.family.value && result.match.tier == 8;
            CHECK(exact);
        }
    }
}

void ItalianAndGerman(const Catalog& catalog) {
    for (const auto& emoji : catalog.Entries()) {
        CHECK(!GetEmojiName(emoji, "de").empty());
        CHECK(!GetEmojiName(emoji, "it").empty());
    }
    Profile profile;
    CHECK(profile.settings.displayLanguages.Set({"it", "de"}));
    for (const auto* query : {L"razzo", L"Rakete"}) {
        const auto results = Search(catalog, profile, query);
        CHECK(!results.empty() && results.front().payload == L"🚀");
        CHECK(results.front().label == L"razzo · Rakete" && results.front().match.tier == 8);
    }
    for (const auto* query : {L"Heißgetränk", L"bevanda calda"})
        CHECK(Search(catalog, profile, query).front().payload == L"☕");
    const auto results = Search(catalog, profile, L"leicht lächelndes Gesicht");
    CHECK(results.front().payload == L"🙂");
    CHECK(results.front().label == L"faccina con sorriso accennato · leicht lächelndes Gesicht");
    CHECK(profile.settings.displayLanguages.Set({"en"}));
    CHECK(Search(catalog, profile, L"razzo").front().label == L"rocket");
}

void French(const Catalog& catalog) {
    for (const auto& emoji : catalog.Entries()) CHECK(!GetEmojiName(emoji, "fr").empty());
    Profile profile;
    CHECK(profile.settings.displayLanguages.Set({"fr", "en"}));
    auto results = Search(catalog, profile, L"fusée");
    CHECK(!results.empty() && results.front().payload == L"🚀");
    CHECK(results.front().label == L"fusée · rocket");
    CHECK(Search(catalog, profile, L"visage avec un léger sourire").front().payload == L"🙂");
}

void Spanish(const Catalog& catalog) {
    for (const auto& emoji : catalog.Entries()) CHECK(!GetEmojiName(emoji, "es").empty());
    Profile profile;
    CHECK(profile.settings.displayLanguages.Set({"es", "en"}));
    auto results = Search(catalog, profile, L"cohete");
    CHECK(!results.empty() && results.front().payload == L"🚀");
    CHECK(results.front().label == L"cohete · rocket");
}

int main(int argc, char** argv) {
    try {
        CHECK(argc == 2);
        const auto root = std::filesystem::u8path(argv[1]);
        std::ifstream data(root / "emojis.txt", std::ios::binary), intents(root / "intent_phrases.tsv", std::ios::binary);
        Catalog catalog;
        CHECK(catalog.Load(data) && catalog.LoadIntents(intents));
        Aliases(catalog); NormalizationAndRanking(); Corpus(catalog, root); ItalianAndGerman(catalog); French(catalog); Spanish(catalog);
        std::cout << "Bilingual names, Unicode normalization, alias CRUD/restart and ranking invariants passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
