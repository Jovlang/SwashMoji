#include "personalization.h"
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
}
