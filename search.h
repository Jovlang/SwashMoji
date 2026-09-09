#pragma once
#include "catalog.h"
#include "personalization.h"
#include "text.h"

namespace SwashMoji {
// An empty locale filter searches all canonical localizations. This is independent
// of the two display-language slots; aliases and curated intents remain available.
MatchScore LexicalScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords, const std::vector<std::string>& locales = {});
int FuzzyScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords, const std::vector<std::string>& locales = {});
std::vector<SearchResult> Search(const Catalog& catalog, const Profile& profile, const std::wstring& query,
                                 const RankingPreferences* snapshot = nullptr, const std::vector<std::string>& locales = {});
}
