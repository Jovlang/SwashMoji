#pragma once
#include "catalog.h"
#include "personalization.h"
#include "text.h"

namespace SwashMoji {
// Search preference is independent of capability and has no two-locale limit.
// An explicit empty preference disables catalog fuzzy matching.
struct SearchLanguagePolicy {
    std::vector<std::string> preferredLocales;
};

// An empty locale filter searches all canonical localizations. This is independent
// of the two display-language slots; aliases and curated intents remain available.
MatchScore LexicalScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords, const std::vector<std::string>& locales = {});
int FuzzyScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords, const std::vector<std::string>& locales = {});
// Without an override, Search uses display locales as preferences, never as a
// hard filter. Non-fuzzy matching still uses locales; fuzzy uses their intersection
// with preferredLocales. Curated intents and personal phrases are locale-neutral.
std::vector<SearchResult> Search(const Catalog& catalog, const Profile& profile, const std::wstring& query,
                                 const RankingPreferences* snapshot = nullptr, const std::vector<std::string>& locales = {},
                                 const SearchLanguagePolicy* languagePolicy = nullptr);
}
