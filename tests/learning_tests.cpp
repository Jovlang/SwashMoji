#include "test_support.h"
#include "search.h"
#include "storage.h"
#include <sstream>
#include <algorithm>
#include <limits>
#include <set>

using namespace SwashMoji;
const ResultId thumbs{ResultKind::Emoji, L"👍"}, okay{ResultKind::Emoji, L"👌"}, rocket{ResultKind::Emoji, L"🚀"};

Catalog Fixture(bool reversed = false) {
    std::vector<std::string> rows{
        "👍\tthumbs up\tgood | nice", "👍🏽\tthumbs up medium skin tone\tgood | nice",
        "👌\tok hand\tgood | nice", "👌🏽\tok hand medium skin tone\tgood | nice",
        "🚀\trocket\tlaunch", "🙂\tnice\tpleasant", "🎉\tparty\tcelebration",
        "☕\tcoffee\tbreak", "❤️\tred heart\tlove", "🔥\tfire\thot", "👋\twaving hand\thello",
        "🎂\tbirthday cake\tfood", "💡\tlight bulb\tidea", "✅\tcheck mark\tdone", "🌮\ttaco\tfood"};
    if (reversed) std::reverse(rows.begin(), rows.end());
    std::stringstream input;
    for (const auto& row : rows) input << row << '\n';
    Catalog catalog;
    CHECK(catalog.Load(input));
    return catalog;
}

std::vector<ResultId> Ids(const std::vector<SearchResult>& results) {
    std::vector<ResultId> ids;
    for (const auto& result : results) ids.push_back(result.id);
    return ids;
}

void LearningAndSnapshot(const Catalog& catalog) {
    Profile profile;
    Remember(profile, thumbs);
    CHECK(Search(catalog, profile, L"good").front().id == thumbs);
    const RankingPreferences snapshot = profile;
    const auto before = Ids(Search(catalog, profile, L"good", &snapshot));
    for (int i = 0; i < 3; ++i) RecordChoice(profile, okay, L" GOOD! ");
    CHECK(QueryCount(profile, L"good", okay) == 3);
    CHECK(QueryCount(profile, L"goo", okay) == 0);
    CHECK(QueryCount(profile, L"nice", okay) == 0);
    CHECK(Ids(Search(catalog, profile, L"good", &snapshot)) == before);
    CHECK(Search(catalog, profile, L"good").front().id == okay);
    // Give the rival more recent and more frequent usage. Query-specific choices still win.
    for (int i = 0; i < 10; ++i) Remember(profile, thumbs);
    for (bool mostUsed : {false, true}) {
        profile.settings.sortByUsage = mostUsed;
        CHECK(Search(catalog, profile, L"good").front().id == okay);
        CHECK(Search(catalog, profile, L"nice")[1].id == thumbs);
    }
    RecordChoice(profile, okay, L"nice");
    CHECK(Search(catalog, profile, L"nice").front().label == L"nice"); // An exact name stays ahead.
    RecordChoice(profile, rocket, L"nice");
    for (const auto& result : Search(catalog, profile, L"nice")) CHECK(!(result.id == rocket));
    CHECK(SetAlias(profile, catalog, L"nice", thumbs) == AliasResult::Saved);
    CHECK(Search(catalog, profile, L"nice").front().id == thumbs);
    const auto encoded = EncodeProfile(profile);
    Search(catalog, profile, L"good"); Search(catalog, profile, L"goo");
    CHECK(EncodeProfile(profile) == encoded); // Typing does not learn or refresh LRU order.
    profile.settings.learnQueries = false;
    CHECK(Search(catalog, profile, L"good").front().id == thumbs);
    RecordChoice(profile, okay, L"good");
    CHECK(QueryCount(profile, L"good", okay) == 3);
    profile.settings.learnQueries = true;
    CHECK(Search(catalog, profile, L"good").front().id == okay);
    auto restarted = DecodeProfile(EncodeProfile(profile));
    CHECK(restarted.profile.settings.learnQueries);
    CHECK(QueryCount(restarted.profile, L"good", okay) == 3);
    CHECK(Search(catalog, restarted.profile, L"good").front().id == okay);
}

void BoundsAndLru(const Catalog& catalog) {
    Profile profile;
    for (size_t i = 0; i < kMaxQueryChoices; ++i) RecordChoice(profile, thumbs, L"query " + std::to_wstring(i));
    CHECK(profile.queryChoices.size() == kMaxQueryChoices);
    RecordChoice(profile, thumbs, L"query 0");
    RecordChoice(profile, okay, L"query 0"); // Same query/different target is a distinct record.
    CHECK(QueryCount(profile, L"query 0", thumbs) == 2);
    CHECK(QueryCount(profile, L"query 0", okay) == 1);
    CHECK(QueryCount(profile, L"query 1", thumbs) == 0);
    CHECK(profile.queryChoices.size() == kMaxQueryChoices);
    profile.queryChoices[0].count = std::numeric_limits<unsigned int>::max();
    profile.usage[okay] = std::numeric_limits<unsigned int>::max();
    RecordChoice(profile, okay, L"query 0");
    CHECK(profile.queryChoices[0].count == std::numeric_limits<unsigned int>::max());
    CHECK(UsageCount(profile, okay) == std::numeric_limits<unsigned int>::max());
    const auto saved = EncodeProfile(profile);
    auto restarted = DecodeProfile(saved);
    CHECK(restarted.format == ProfileFormat::Valid && !restarted.skippedRecords);
    CHECK(EncodeProfile(restarted.profile) == saved);
    RecordChoice(restarted.profile, okay, L"new query");
    CHECK(QueryCount(restarted.profile, L"query 2", thumbs) == 0);
    RecordChoice(restarted.profile, okay, std::wstring(kMaxQueryLength + 1, L'x'));
    CHECK(restarted.profile.queryChoices.front().query == L"new query");
    RecordChoice(restarted.profile, okay, L"!!!");
    CHECK(restarted.profile.queryChoices.front().query == L"new query");
    CHECK(Pin(restarted.profile, catalog, thumbs) == PinResult::Pinned);
    CHECK(SetAlias(restarted.profile, catalog, L"favorite hand", thumbs) == AliasResult::Saved);
    restarted.profile.settings.skinTone = 3;
    ClearHistory(restarted.profile);
    CHECK(restarted.profile.history.empty() && restarted.profile.usage.empty() && restarted.profile.queryChoices.empty());
    CHECK(restarted.profile.pins == std::vector<ResultId>{thumbs});
    CHECK(restarted.profile.aliases.size() == 1 && restarted.profile.settings.skinTone == 3);
}

void FavoritesAndFamilies(const Catalog& catalog) {
    Profile profile;
    CHECK(Pin(profile, catalog, rocket) == PinResult::Pinned);
    CHECK(Pin(profile, catalog, thumbs) == PinResult::Pinned);
    CHECK(Pin(profile, catalog, thumbs) == PinResult::AlreadyPinned);
    CHECK(Pin(profile, catalog, {ResultKind::Emoji, L"missing"}) == PinResult::InvalidTarget);
    RecordChoice(profile, okay, L"good");
    for (bool mostUsed : {false, true}) for (int tone : {0, 3, 5}) {
        profile.settings.sortByUsage = mostUsed;
        profile.settings.skinTone = tone;
        const auto results = Search(catalog, profile, L"");
        CHECK(results[0].id == rocket && results[1].id == thumbs && results[2].id == okay);
        CHECK(results[1].payload == (tone == 3 ? L"👍🏽" : L"👍"));
        std::set<ResultId> unique;
        for (const auto& result : results) CHECK(unique.insert(result.id).second);
        CHECK(Search(catalog, profile, L"good").front().id == okay); // Pins do not boost nonempty queries.
        for (const auto& result : Search(catalog, profile, L"good")) CHECK(!(result.id == rocket));
    }
    CHECK(MovePin(profile, thumbs, -1));
    CHECK(Search(catalog, profile, L"").front().id == thumbs);
    CHECK(!MovePin(profile, thumbs, -1));
    CHECK(MovePin(profile, thumbs, 1));
    CHECK(!MovePin(profile, thumbs, 1));
    CHECK(!MovePin(profile, thumbs, 3));
    CHECK(Unpin(profile, rocket));
    CHECK(!Unpin(profile, rocket));
    for (const auto& emoji : catalog.Entries()) {
        if (profile.pins.size() == kMaxPins) break;
        if (!SkinToneIndex(emoji.glyph)) Pin(profile, catalog, {ResultKind::Emoji, emoji.family.value});
    }
    CHECK(profile.pins.size() == kMaxPins);
    CHECK(Pin(profile, catalog, {ResultKind::Emoji, L"🌮"}) == PinResult::LimitReached);
    CHECK(DecodeProfile(EncodeProfile(profile)).profile.pins == profile.pins);

    Profile legacy;
    legacy.history = {{ResultKind::Emoji, L"👍🏽"}, rocket, thumbs, {ResultKind::Emoji, L"unknown"}};
    legacy.usage = {{thumbs, 10}, {{ResultKind::Emoji, L"👍🏽"}, 7}, {rocket, 3}, {{ResultKind::Emoji, L"unknown"}, 2}};
    CHECK(NormalizeFamilyHistory(legacy, catalog));
    CHECK(legacy.history == (std::vector<ResultId>{thumbs, rocket, {ResultKind::Emoji, L"unknown"}}));
    CHECK(UsageCount(legacy, thumbs) == 17 && UsageCount(legacy, L"👍🏽") == 0);
    CHECK(UsageCount(legacy, L"unknown") == 2);
    CHECK(!NormalizeFamilyHistory(legacy, catalog));
    RecordChoice(legacy, thumbs, L"nice");
    CHECK(legacy.history.size() == 3 && UsageCount(legacy, thumbs) == 18);
    legacy.usage[{ResultKind::Emoji, L"👍🏽"}] = std::numeric_limits<unsigned int>::max();
    CHECK(NormalizeFamilyHistory(legacy, catalog));
    CHECK(UsageCount(legacy, thumbs) == std::numeric_limits<unsigned int>::max());
}

void Determinism(const Catalog& catalog) {
    const auto reverse = Fixture(true);
    for (const auto& query : {L"", L"good", L"nice", L"food", L"goof"})
        CHECK(Ids(Search(catalog, Profile{}, query)) == Ids(Search(reverse, Profile{}, query)));
}

int main() {
    try {
        const auto catalog = Fixture();
        LearningAndSnapshot(catalog); BoundsAndLru(catalog); FavoritesAndFamilies(catalog); Determinism(catalog);
        std::cout << "Query learning, LRU bounds, preference snapshots, favorites and family aggregation passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
