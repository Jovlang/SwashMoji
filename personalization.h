#pragma once
#include "models.h"
#include <map>
#include <string>
#include <vector>

namespace SwashMoji {
class Catalog;
constexpr size_t kMaxHistory = 40;
constexpr size_t kMaxAliases = 500;
constexpr size_t kMaxAliasLength = 96;
constexpr size_t kMaxPins = 10;
constexpr size_t kMaxQueryChoices = 1000;
constexpr size_t kMaxQueryLength = 256;

struct Alias { std::wstring phrase; ResultId target; };

struct Settings {
    bool positionAboveTextField{};
    bool sortByUsage{};
    int emojiRows{1};
    int skinTone{};
    bool learnQueries{true};
    DisplayLanguages displayLanguages;
};

struct QueryChoice {
    std::wstring query;
    ResultId target;
    unsigned int count{};
};

struct RankingPreferences {
    std::vector<ResultId> history;
    std::map<ResultId, unsigned int> usage;
    // Most recently chosen pair first. Searching alone does not touch this LRU.
    std::vector<QueryChoice> queryChoices;
};

constexpr size_t kMaxCombinations = 200;
struct CombinationEntry { std::wstring family, payload; };
struct Combination {
    std::wstring id, name, payload;
    std::vector<CombinationEntry> entries;
};

struct Profile : RankingPreferences {
    Settings settings;
    std::map<std::wstring, Combination> combinations;
    std::map<std::wstring, Alias> aliases;
    std::vector<ResultId> pins;
};

bool ResolveResult(const Catalog& catalog, const Profile& profile, const ResultId& id, SearchResult& result);
bool ValidCombination(const Combination& combination);
// Empty id creates a new persistent identity; edits retain the identity.
bool SaveCombination(Profile& profile, const Catalog& catalog, Combination& draft, std::wstring& error);
bool DeleteCombination(Profile& profile, const std::wstring& id);

void Remember(Profile& profile, const ResultId& target);
// Legacy import/test convenience. Live selections use their catalog-backed ID.
void Remember(Profile& profile, const std::wstring& glyph);
void RecordChoice(Profile& profile, const ResultId& target, const std::wstring& query);
bool NormalizeFamilyHistory(Profile& profile, const Catalog& catalog);
void ClearHistory(Profile& profile);
int HistoryBoost(const RankingPreferences& profile, const ResultId& target);
unsigned int UsageCount(const RankingPreferences& profile, const ResultId& target);
int HistoryBoost(const RankingPreferences& profile, const std::wstring& glyph);
unsigned int UsageCount(const RankingPreferences& profile, const std::wstring& glyph);
unsigned int QueryCount(const RankingPreferences& profile, const std::wstring& query, const ResultId& target);
enum class PinResult { Pinned, AlreadyPinned, InvalidTarget, LimitReached };
PinResult Pin(Profile& profile, const Catalog& catalog, const ResultId& target);
bool Unpin(Profile& profile, const ResultId& target);
bool MovePin(Profile& profile, const ResultId& target, int direction);
bool IsPinned(const Profile& profile, const ResultId& target);
enum class AliasResult { Saved, Duplicate, InvalidPhrase, InvalidTarget, LimitReached, MissingOriginal };
AliasResult SetAlias(Profile& profile, const Catalog& catalog, const std::wstring& phrase,
                     const ResultId& target, const std::wstring& original = {}, bool replace = false);
bool DeleteAlias(Profile& profile, const std::wstring& phrase);
}
