#include "personalization.h"
#include "catalog.h"
#include "text.h"
#include <algorithm>
#include <limits>

namespace SwashMoji {
void Remember(Profile& profile, const std::wstring& glyph) {
    if (glyph.empty()) return;
    auto& history = profile.history;
    history.erase(std::remove(history.begin(), history.end(), glyph), history.end());
    history.insert(history.begin(), glyph);
    if (history.size() > kMaxHistory) history.resize(kMaxHistory);
    auto& count = profile.usage[glyph];
    if (count < std::numeric_limits<unsigned int>::max()) ++count;
}

void ClearHistory(Profile& profile) { profile.history.clear(); profile.usage.clear(); }

int HistoryBoost(const Profile& profile, const std::wstring& glyph) {
    const auto found = std::find(profile.history.begin(), profile.history.end(), glyph);
    if (found == profile.history.end()) return 0;
    return std::max(1, 20 - static_cast<int>(std::distance(profile.history.begin(), found)));
}

unsigned int UsageCount(const Profile& profile, const std::wstring& glyph) {
    const auto found = profile.usage.find(glyph);
    return found == profile.usage.end() ? 0 : found->second;
}

AliasResult SetAlias(Profile& profile, const Catalog& catalog, const std::wstring& phrase,
                     const ResultId& target, const std::wstring& original, bool replace) {
    const auto key = NormalizePhrase(phrase);
    const auto oldKey = NormalizePhrase(original);
    if (key.empty() || phrase.size() > kMaxAliasLength || phrase.find(L'\0') != std::wstring::npos || WideToUtf8(phrase).empty()) return AliasResult::InvalidPhrase;
    if (target.kind != ResultKind::Emoji || !catalog.FindFamily({target.value})) return AliasResult::InvalidTarget;
    if (!original.empty() && !profile.aliases.count(oldKey)) return AliasResult::MissingOriginal;
    if (key != oldKey && profile.aliases.count(key) && !replace) return AliasResult::Duplicate;
    if (original.empty() && !profile.aliases.count(key) && profile.aliases.size() >= kMaxAliases) return AliasResult::LimitReached;
    if (!oldKey.empty() && oldKey != key) profile.aliases.erase(oldKey);
    profile.aliases[key] = {phrase, target};
    return AliasResult::Saved;
}

bool DeleteAlias(Profile& profile, const std::wstring& phrase) { return profile.aliases.erase(NormalizePhrase(phrase)) != 0; }
}
