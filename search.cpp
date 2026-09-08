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
    for (const auto& word : emoji.nbNameWords) if (word == token) return {5, 100, true};
    const std::wstring normalizedToken = NormalizeSearchWord(token);
    for (const auto& word : emoji.normalizedNameWords) {
        if (word == normalizedToken) return {5, 90, true};
    }
    for (const auto& word : emoji.keywordWords) if (word == token) return {4, 100, false};
    for (const auto& word : emoji.nbKeywordWords) if (word == token) return {4, 100, false};
    for (const auto& word : emoji.normalizedKeywordWords) {
        if (word == normalizedToken) return {4, 90, false};
    }
    for (const auto& word : emoji.nameWords) if (StartsWith(word, token)) return {3, 80, true};
    for (const auto& word : emoji.nbNameWords) if (StartsWith(word, token)) return {3, 80, true};
    for (const auto& word : emoji.keywordWords) if (StartsWith(word, token)) return {2, 70, false};
    for (const auto& word : emoji.nbKeywordWords) if (StartsWith(word, token)) return {2, 70, false};
    if (emoji.lowerName.find(token) != std::wstring::npos) return {2, 60, true};
    if (emoji.lowerNbName.find(token) != std::wstring::npos) return {2, 60, true};
    if (emoji.lowerKeywords.find(token) != std::wstring::npos) return {1, 50, false};
    if (emoji.lowerNbKeywords.find(token) != std::wstring::npos) return {1, 50, false};
    return {};
}

MatchScore LexicalScore(const Emoji& emoji, const std::vector<std::wstring>& queryWords) {
    if (queryWords.empty()) return {};
    const std::wstring phrase = JoinWords(queryWords);
    if (emoji.lowerName == phrase || emoji.lowerNbName == phrase) return {8, 1000};
    if (StartsWith(emoji.lowerName, phrase) || StartsWith(emoji.lowerNbName, phrase)) return {6, 800};

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
        const size_t limit = token.size() < 4 ? 0 : (token.size() >= 8 ? 2 : 1);
        size_t best = limit + 1;
        for (const auto& word : emoji.nameWords) best = std::min(best, EditDistanceAtMost(token, word, limit));
        for (const auto& word : emoji.keywordWords) best = std::min(best, EditDistanceAtMost(token, word, limit));
        for (const auto& word : emoji.nbNameWords) best = std::min(best, EditDistanceAtMost(token, word, limit));
        for (const auto& word : emoji.nbKeywordWords) best = std::min(best, EditDistanceAtMost(token, word, limit));
        if (best > limit) return -1;
        score += 50 - static_cast<int>(best * 15);
    }
    return score;
}

namespace {
MatchScore PhraseMatch(const std::wstring& phrase, const std::vector<std::wstring>& words, bool alias, bool fuzzy = false) {
    const auto query = JoinWords(words);
    if (phrase == query) return {alias ? 9 : 7, 1000};
    if (StartsWith(phrase, query)) return {alias ? 6 : 4, 800};
    const auto phraseWords = SplitWords(phrase);
    int detail = 0;
    for (const auto& token : words) {
        const size_t limit = fuzzy && token.size() >= 4 ? (token.size() >= 8 ? 2 : 1) : 0;
        bool matched = false;
        for (const auto& word : phraseWords) {
            if ((!fuzzy && StartsWith(word, token)) || (fuzzy && EditDistanceAtMost(word, token, limit) <= limit)) {
                matched = true;
                detail += word == token ? 100 : 70;
                break;
            }
        }
        if (!matched) return {};
    }
    return {fuzzy ? 1 : (alias ? 5 : 4), detail};
}

bool Better(MatchScore left, MatchScore right) {
    return left.tier > right.tier || (left.tier == right.tier && left.detail > right.detail);
}

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

std::vector<SearchResult> Search(const Catalog& catalog, const Profile& profile, const std::wstring& query,
                                 const RankingPreferences* snapshot) {
    const auto& preferences = snapshot ? *snapshot : static_cast<const RankingPreferences&>(profile);
    const auto normalized = Lower(query);
    const auto first = normalized.find_first_not_of(L" \t\r\n");
    const auto last = normalized.find_last_not_of(L" \t\r\n");
    const auto glyph = first == std::wstring::npos ? std::wstring{} : normalized.substr(first, last - first + 1);
    if (const auto* exact = catalog.Find(glyph)) {
        return {{{ResultKind::Emoji, exact->family.value}, exact->name, exact->glyph, {8, 1000}, L"Exact emoji"}};
    }
    const auto words = SplitWords(normalized);
    if (words.empty() && !glyph.empty()) return {};
    std::map<std::wstring, unsigned int> queryCounts;
    if (profile.settings.learnQueries && !words.empty()) {
        const auto phrase = JoinWords(words);
        for (const auto& choice : preferences.queryChoices)
            if (choice.query == phrase && choice.target.kind == ResultKind::Emoji) queryCounts[choice.target.value] = choice.count;
    }
    const auto learned = [&queryCounts](const Emoji& emoji) {
        const auto found = queryCounts.find(emoji.family.value);
        return found == queryCounts.end() ? 0u : found->second;
    };
    struct Candidate { const Emoji* emoji; MatchScore match; std::wstring explanation; bool exactVariant{}; };
    std::vector<Candidate> candidates;
    if (words.empty()) {
        for (const auto& emoji : catalog.Entries()) if (!SkinToneIndex(emoji.glyph)) candidates.push_back({&emoji, {}});
        std::stable_sort(candidates.begin(), candidates.end(), [&](const Candidate& left, const Candidate& right) {
            const auto& a = left.emoji->family.value;
            const auto& b = right.emoji->family.value;
            const int preference = SwashMojiRanking::ComparePreference(profile.settings.sortByUsage,
                UsageCount(preferences, a), HistoryBoost(preferences, a), UsageCount(preferences, b), HistoryBoost(preferences, b));
            if (preference) return preference > 0;
            const int popularity = PopularityPrior(left.emoji->glyph) - PopularityPrior(right.emoji->glyph);
            return popularity ? popularity > 0 : a < b;
        });
        std::vector<Candidate> pinned;
        for (const auto& id : profile.pins) {
            if (id.kind != ResultKind::Emoji) continue;
            if (const auto* emoji = catalog.FindFamily({id.value})) pinned.push_back({emoji, {}, L"Favorite"});
        }
        candidates.insert(candidates.begin(), pinned.begin(), pinned.end());
    } else {
        for (const auto& emoji : catalog.Entries()) {
            auto match = LexicalScore(emoji, words);
            const bool toned = SkinToneIndex(emoji.glyph) != 0;
            if (toned && match.tier != 8) continue;
            std::wstring explanation;
            for (const auto& phrase : emoji.intents) {
                const auto intent = PhraseMatch(phrase, words, false);
                if (Better(intent, match)) { match = intent; explanation = L"Intent: " + phrase; }
            }
            if (match.tier) candidates.push_back({&emoji, match, explanation, toned});
        }
        for (const auto& entry : profile.aliases) {
            if (entry.second.target.kind != ResultKind::Emoji) continue;
            const auto* emoji = catalog.FindFamily({entry.second.target.value});
            const auto match = PhraseMatch(entry.first, words, true);
            if (emoji && match.tier) candidates.push_back({emoji, match, L"Alias: " + entry.second.phrase});
        }
        if (candidates.empty()) {
            for (const auto& emoji : catalog.Entries()) {
                if (SkinToneIndex(emoji.glyph)) continue;
                int score = FuzzyScore(emoji, words);
                for (const auto& phrase : emoji.intents) {
                    const auto match = PhraseMatch(phrase, words, false, true);
                    if (match.tier) score = std::max(score, match.detail);
                }
                if (score >= 0) candidates.push_back({&emoji, {1, score}, L"Similar spelling"});
            }
            for (const auto& entry : profile.aliases) {
                if (entry.second.target.kind != ResultKind::Emoji) continue;
                const auto* emoji = catalog.FindFamily({entry.second.target.value});
                const auto match = PhraseMatch(entry.first, words, true, true);
                if (emoji && match.tier) candidates.push_back({emoji, {1, match.detail}, L"Similar alias: " + entry.second.phrase});
            }
        }
        std::stable_sort(candidates.begin(), candidates.end(), [&](const Candidate& left, const Candidate& right) {
            if (left.match.tier != right.match.tier) return left.match.tier > right.match.tier;
            const auto leftCount = learned(*left.emoji), rightCount = learned(*right.emoji);
            if (leftCount != rightCount) return leftCount > rightCount;
            const int preference = SwashMojiRanking::ComparePreference(profile.settings.sortByUsage,
                UsageCount(preferences, left.emoji->family.value), HistoryBoost(preferences, left.emoji->family.value),
                UsageCount(preferences, right.emoji->family.value), HistoryBoost(preferences, right.emoji->family.value));
            if (preference) return preference > 0;
            if (left.match.detail != right.match.detail) return left.match.detail > right.match.detail;
            const auto leftPrior = PopularityPrior(left.emoji->glyph), rightPrior = PopularityPrior(right.emoji->glyph);
            if (leftPrior != rightPrior) return leftPrior > rightPrior;
            if (left.emoji->family.value != right.emoji->family.value) return left.emoji->family.value < right.emoji->family.value;
            return left.emoji->glyph < right.emoji->glyph;
        });
    }
    std::unordered_set<std::wstring> added;
    std::vector<SearchResult> results;
    for (const auto& candidate : candidates) {
        const auto& emoji = *candidate.emoji;
        if ((SkinToneIndex(emoji.glyph) && !candidate.exactVariant) || !added.insert(emoji.family.value).second) continue;
        const auto* variant = candidate.exactVariant ? &emoji : catalog.PreferredVariant(emoji, profile.settings.skinTone);
        results.push_back({{ResultKind::Emoji, emoji.family.value}, variant->name,
                           variant->glyph, candidate.match, candidate.explanation});
    }
    return results;
}

}
