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

struct Alias { std::wstring phrase; ResultId target; };

struct Settings {
    bool positionAboveTextField{};
    bool sortByUsage{};
    int emojiRows{1};
    int skinTone{};
};

struct Profile {
    Settings settings;
    // Preserve exact glyph usage until the family aggregation milestone (M3).
    std::vector<std::wstring> history;
    std::map<std::wstring, unsigned int> usage;
    std::map<std::wstring, Alias> aliases;
};

void Remember(Profile& profile, const std::wstring& glyph);
void ClearHistory(Profile& profile);
int HistoryBoost(const Profile& profile, const std::wstring& glyph);
unsigned int UsageCount(const Profile& profile, const std::wstring& glyph);
enum class AliasResult { Saved, Duplicate, InvalidPhrase, InvalidTarget, LimitReached, MissingOriginal };
AliasResult SetAlias(Profile& profile, const Catalog& catalog, const std::wstring& phrase,
                     const ResultId& target, const std::wstring& original = {}, bool replace = false);
bool DeleteAlias(Profile& profile, const std::wstring& phrase);
}
