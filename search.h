#pragma once
#include "catalog.h"
#include "personalization.h"
#include "text.h"

namespace SwashMoji {
MatchScore LexicalScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords);
int FuzzyScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords);
std::vector<SearchResult> Search(const Catalog& catalog, const Profile& profile, const std::wstring& query,
                                 const RankingPreferences* snapshot = nullptr);
}
