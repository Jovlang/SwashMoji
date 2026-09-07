#include "search.h"
#include "ranking.h"
#include <algorithm>
#include <unordered_set>

namespace SwashMoji {
struct TokenMatch {
    int tier{};
    int detail{};
    bool fromName{};
};

TokenMatch ScoreToken(const Emoji& emoji, const std::wstring& token) {
    for (const auto& word : emoji.nameWords) if (word == token) return {5, 100, true};
    const std::wstring normalizedToken = NormalizeSearchWord(token);
    for (const auto& word : emoji.normalizedNameWords) {
        if (word == normalizedToken) return {5, 90, true};
    }
    for (const auto& word : emoji.keywordWords) if (word == token) return {4, 100, false};
    for (const auto& word : emoji.normalizedKeywordWords) {
        if (word == normalizedToken) return {4, 90, false};
    }
    for (const auto& word : emoji.nameWords) if (StartsWith(word, token)) return {3, 80, true};
    for (const auto& word : emoji.keywordWords) if (StartsWith(word, token)) return {2, 70, false};
    if (emoji.lowerName.find(token) != std::wstring::npos) return {2, 60, true};
    if (emoji.lowerKeywords.find(token) != std::wstring::npos) return {1, 50, false};
    return {};
}

MatchScore LexicalScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords) {
    if (queryWords.empty()) return {};
    const std::wstring phrase = JoinWords(queryWords);
    if (emoji.lowerName == phrase) return {7, 1000};
    if (StartsWith(emoji.lowerName, phrase)) return {6, 800};

    int weakestTier = 5;
    int detail = 0;
    int nameMatches = 0;
    for (const auto& token : queryWords) {
        const TokenMatch match = ScoreToken(emoji, token);
        if (!match.tier) return {};
        weakestTier = std::min(weakestTier, match.tier);
        detail += match.detail;
        if (match.fromName) ++nameMatches;
    }
    const int extraNameWords = std::max(0, static_cast<int>(emoji.nameWords.size()) - nameMatches);
    return {weakestTier, detail - std::min(40, extraNameWords * 4)};
}

size_t EditDistanceAtMost(const std::wstring& left, const std::wstring& right, size_t limit) {
    const size_t lengthDifference = left.size() > right.size()
        ? left.size() - right.size() : right.size() - left.size();
    if (lengthDifference > limit) return limit + 1;
    if (left.size() == right.size()) {
        size_t firstDifference = 0;
        while (firstDifference < left.size() && left[firstDifference] == right[firstDifference]) {
            ++firstDifference;
        }
        if (firstDifference + 1 < left.size() &&
            left[firstDifference] == right[firstDifference + 1] &&
            left[firstDifference + 1] == right[firstDifference] &&
            left.compare(firstDifference + 2, std::wstring::npos,
                         right, firstDifference + 2, std::wstring::npos) == 0) {
            return 1;
        }
    }

    std::vector<size_t> previous(right.size() + 1);
    std::vector<size_t> current(right.size() + 1);
    for (size_t column = 0; column <= right.size(); ++column) previous[column] = column;
    for (size_t row = 1; row <= left.size(); ++row) {
        current[0] = row;
        size_t rowMinimum = current[0];
        for (size_t column = 1; column <= right.size(); ++column) {
            const size_t substitution = previous[column - 1] + (left[row - 1] == right[column - 1] ? 0 : 1);
            current[column] = std::min({previous[column] + 1, current[column - 1] + 1, substitution});
            rowMinimum = std::min(rowMinimum, current[column]);
        }
        if (rowMinimum > limit) return limit + 1;
        previous.swap(current);
    }
    return previous[right.size()];
}

int FuzzyScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords) {
    if (queryWords.empty()) return -1;
    int score = 0;
    for (const auto& token : queryWords) {
        if (token.size() < 4) return -1;
        const size_t limit = token.size() >= 8 ? 2 : 1;
        size_t best = limit + 1;
        for (const auto& word : emoji.nameWords) best = std::min(best, EditDistanceAtMost(token, word, limit));
        for (const auto& word : emoji.keywordWords) best = std::min(best, EditDistanceAtMost(token, word, limit));
        if (best > limit) return -1;
        score += 50 - static_cast<int>(best * 15);
    }
    return score;
}

namespace {
int PopularityPrior(const std::wstring& glyph) {
    static const wchar_t* popular[]{
        L"😊", L"🙂", L"😂", L"❤️", L"😍", L"🥰", L"😄", L"😁",
        L"😃", L"😀", L"😉", L"😆", L"😎", L"🤣", L"😭", L"😘"
    };
    for (size_t index = 0; index < std::size(popular); ++index) {
        if (glyph == popular[index]) return static_cast<int>(std::size(popular) - index);
    }
    return 0;
}
}

std::vector<SearchResult> Search(const Catalog& catalog, const Profile& profile, const std::wstring& query) {
    const auto words = SplitWords(Lower(query));
    struct Candidate { const Emoji* emoji; MatchScore match; };
    std::vector<Candidate> candidates;
    if (words.empty()) {
        if (profile.settings.sortByUsage) {
            for (const auto& emoji : catalog.Entries()) {
                if (UsageCount(profile, emoji.glyph)) candidates.push_back({&emoji, {}});
            }
            std::stable_sort(candidates.begin(), candidates.end(), [&profile](const Candidate& left, const Candidate& right) {
                return SwashMojiRanking::ComparePreference(true,
                    UsageCount(profile, left.emoji->glyph), HistoryBoost(profile, left.emoji->glyph),
                    UsageCount(profile, right.emoji->glyph), HistoryBoost(profile, right.emoji->glyph)) > 0;
            });
        } else {
            for (const auto& glyph : profile.history) {
                if (const Emoji* emoji = catalog.Find(glyph)) candidates.push_back({emoji, {}});
            }
        }
        for (const auto& emoji : catalog.Entries()) candidates.push_back({&emoji, {}});
    } else {
        for (const auto& emoji : catalog.Entries()) {
            const auto match = LexicalScore(emoji, words);
            if (match.tier) candidates.push_back({&emoji, match});
        }
        if (candidates.empty()) {
            for (const auto& emoji : catalog.Entries()) {
                const int score = FuzzyScore(emoji, words);
                if (score >= 0) candidates.push_back({&emoji, {1, score}});
            }
        }
        std::stable_sort(candidates.begin(), candidates.end(), [&profile](const Candidate& left, const Candidate& right) {
            if (left.match.tier != right.match.tier) return left.match.tier > right.match.tier;
            const int preference = SwashMojiRanking::ComparePreference(profile.settings.sortByUsage,
                UsageCount(profile, left.emoji->glyph), HistoryBoost(profile, left.emoji->glyph),
                UsageCount(profile, right.emoji->glyph), HistoryBoost(profile, right.emoji->glyph));
            if (preference) return preference > 0;
            if (left.match.detail != right.match.detail) return left.match.detail > right.match.detail;
            return PopularityPrior(left.emoji->glyph) > PopularityPrior(right.emoji->glyph);
        });
    }
    std::unordered_set<std::wstring> added;
    std::vector<SearchResult> results;
    for (const auto& candidate : candidates) {
        const auto& emoji = *candidate.emoji;
        if (SkinToneIndex(emoji.glyph) || !added.insert(emoji.glyph).second) continue;
        const auto* variant = catalog.PreferredVariant(emoji, profile.settings.skinTone);
        results.push_back({{ResultKind::Emoji, emoji.family.value}, variant->name,
                           variant->glyph, candidate.match, {}});
    }
    return results;
}

}
