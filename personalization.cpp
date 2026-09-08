#include "personalization.h"
#include "catalog.h"
#include "text.h"
#include <algorithm>
#include <limits>

namespace SwashMoji {
void Remember(Profile& profile, const ResultId& target) {
    if (target.value.empty()) return;
    auto& history = profile.history;
    history.erase(std::remove(history.begin(), history.end(), target), history.end());
    history.insert(history.begin(), target);
    if (history.size() > kMaxHistory) history.resize(kMaxHistory);
    auto& count = profile.usage[target];
    if (count < std::numeric_limits<unsigned int>::max()) ++count;
}

void Remember(Profile& profile, const std::wstring& glyph) { Remember(profile, ResultId{ResultKind::Emoji, glyph}); }

void RecordChoice(Profile& profile, const ResultId& target, const std::wstring& query) {
    if (target.value.empty()) return;
    Remember(profile, target);
    const auto key = NormalizePhrase(query);
    if (!profile.settings.learnQueries || key.empty() || key.size() > kMaxQueryLength) return;
    auto& choices = profile.queryChoices;
    const auto found = std::find_if(choices.begin(), choices.end(), [&](const QueryChoice& choice) {
        return choice.query == key && choice.target == target;
    });
    unsigned int count = found == choices.end() ? 0 : found->count;
    if (found != choices.end()) choices.erase(found);
    if (count < std::numeric_limits<unsigned int>::max()) ++count;
    choices.insert(choices.begin(), {key, target, count});
    if (choices.size() > kMaxQueryChoices) choices.resize(kMaxQueryChoices);
}

bool NormalizeFamilyHistory(Profile& profile, const Catalog& catalog) {
    const auto resolve = [&](const ResultId& id) {
        if (id.kind == ResultKind::Emoji) {
            if (const auto* emoji = catalog.Find(id.value)) return ResultId{ResultKind::Emoji, emoji->family.value};
        }
        return id; // Preserve unknown IDs for a later catalog; never fabricate a base.
    };
    std::vector<ResultId> history;
    for (const auto& id : profile.history) {
        const auto family = resolve(id);
        if (std::find(history.begin(), history.end(), family) == history.end()) history.push_back(family);
    }
    std::map<ResultId, unsigned int> usage;
    for (const auto& entry : profile.usage) {
        auto& count = usage[resolve(entry.first)];
        count += std::min(entry.second, std::numeric_limits<unsigned int>::max() - count);
    }
    const bool changed = history != profile.history || usage != profile.usage;
    profile.history = std::move(history);
    profile.usage = std::move(usage);
    return changed;
}

void ClearHistory(Profile& profile) { profile.history.clear(); profile.usage.clear(); profile.queryChoices.clear(); }

int HistoryBoost(const RankingPreferences& profile, const ResultId& target) {
    const auto found = std::find(profile.history.begin(), profile.history.end(), target);
    if (found == profile.history.end()) return 0;
    return std::max(1, static_cast<int>(kMaxHistory) - static_cast<int>(std::distance(profile.history.begin(), found)));
}

unsigned int UsageCount(const RankingPreferences& profile, const ResultId& target) {
    const auto found = profile.usage.find(target);
    return found == profile.usage.end() ? 0 : found->second;
}

int HistoryBoost(const RankingPreferences& profile, const std::wstring& glyph) { return HistoryBoost(profile, ResultId{ResultKind::Emoji, glyph}); }
unsigned int UsageCount(const RankingPreferences& profile, const std::wstring& glyph) { return UsageCount(profile, ResultId{ResultKind::Emoji, glyph}); }

unsigned int QueryCount(const RankingPreferences& profile, const std::wstring& query, const ResultId& target) {
    const auto key = NormalizePhrase(query);
    for (const auto& choice : profile.queryChoices) if (choice.query == key && choice.target == target) return choice.count;
    return 0;
}

bool IsPinned(const Profile& profile, const ResultId& target) {
    return std::find(profile.pins.begin(), profile.pins.end(), target) != profile.pins.end();
}

PinResult Pin(Profile& profile, const Catalog& catalog, const ResultId& target) {
    if (target.kind != ResultKind::Emoji || !catalog.FindFamily({target.value})) return PinResult::InvalidTarget;
    if (IsPinned(profile, target)) return PinResult::AlreadyPinned;
    if (profile.pins.size() >= kMaxPins) return PinResult::LimitReached;
    profile.pins.push_back(target);
    return PinResult::Pinned;
}

bool Unpin(Profile& profile, const ResultId& target) {
    const auto found = std::find(profile.pins.begin(), profile.pins.end(), target);
    if (found == profile.pins.end()) return false;
    profile.pins.erase(found);
    return true;
}

bool MovePin(Profile& profile, const ResultId& target, int direction) {
    const auto found = std::find(profile.pins.begin(), profile.pins.end(), target);
    if (found == profile.pins.end() || (direction != -1 && direction != 1)) return false;
    const auto index = std::distance(profile.pins.begin(), found), next = index + direction;
    if (next < 0 || next >= static_cast<ptrdiff_t>(profile.pins.size())) return false;
    std::iter_swap(found, profile.pins.begin() + next);
    return true;
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
