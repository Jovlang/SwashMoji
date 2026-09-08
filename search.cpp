#include <set>
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
    std::vector<SearchResult> candidates;
    const auto addEmoji = [&](const Emoji& emoji, MatchScore match, const std::wstring& why, bool exact = false) {
        const auto* variant = exact ? &emoji : catalog.PreferredVariant(emoji, profile.settings.skinTone);
        candidates.push_back({{ResultKind::Emoji, emoji.family.value}, variant->name, variant->glyph, match, why});
    };
    const auto addPersonal = [&](bool fuzzy) {
        for (const auto& item : profile.combinations) {
            const auto& c = item.second;
            const auto match = words.empty() ? MatchScore{} : PhraseMatch(NormalizePhrase(c.name), words, true, fuzzy);
            if (words.empty() || match.tier) candidates.push_back({{ResultKind::Combination, c.id}, c.name, c.payload, match, L"Saved combination"});
        }
        if (words.empty()) return;
        for (const auto& entry : profile.aliases) {
            const auto match = PhraseMatch(entry.first, words, true, fuzzy);
            SearchResult result;
            if (match.tier && ResolveResult(catalog, profile, entry.second.target, result)) {
                if (result.id.kind == ResultKind::Emoji) {
                    const auto* variant = catalog.PreferredVariant(*catalog.FindFamily({result.id.value}), profile.settings.skinTone);
                    result.payload = variant->glyph; result.label = variant->name;
                }
                result.match = match; result.explanation = L"Alias: " + entry.second.phrase;
                candidates.push_back(result);
            }
        }
    };
    if (words.empty()) {
        for (const auto& emoji : catalog.Entries()) if (!SkinToneIndex(emoji.glyph)) addEmoji(emoji, {}, L"");
        addPersonal(false);
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
            if (match.tier) addEmoji(emoji, match, explanation, toned);
        }
        addPersonal(false);
        if (candidates.empty()) {
            for (const auto& emoji : catalog.Entries()) {
                if (SkinToneIndex(emoji.glyph)) continue;
                int score = FuzzyScore(emoji, words);
                for (const auto& phrase : emoji.intents) {
                    const auto match = PhraseMatch(phrase, words, false, true);
                    if (match.tier) score = std::max(score, match.detail);
                }
                if (score >= 0) addEmoji(emoji, {1, score}, L"Similar spelling");
            }
            addPersonal(true);
        }
    }
    const auto phrase = JoinWords(words);
    std::map<ResultId, unsigned> queryCounts;
    if (profile.settings.learnQueries && !words.empty())
        for (const auto& choice : preferences.queryChoices) if (choice.query == phrase) queryCounts[choice.target] = choice.count;
    const auto learned = [&](const ResultId& id) {
        const auto found = queryCounts.find(id); return found == queryCounts.end() ? 0u : found->second;
    };
    const auto prior = [&](const SearchResult& result) {
        const auto* emoji = result.id.kind == ResultKind::Emoji ? catalog.FindFamily({result.id.value}) : nullptr;
        return emoji ? PopularityPrior(emoji->glyph) : 0;
    };
    std::stable_sort(candidates.begin(), candidates.end(), [&](const SearchResult& a, const SearchResult& b) {
        if (a.match.tier != b.match.tier) return a.match.tier > b.match.tier;
        if (profile.settings.learnQueries && !words.empty()) {
            const auto ac = learned(a.id), bc = learned(b.id);
            if (ac != bc) return ac > bc;
        }
        const int preference = SwashMojiRanking::ComparePreference(profile.settings.sortByUsage,
            UsageCount(preferences, a.id), HistoryBoost(preferences, a.id), UsageCount(preferences, b.id), HistoryBoost(preferences, b.id));
        if (preference) return preference > 0;
        if (a.match.detail != b.match.detail) return a.match.detail > b.match.detail;
        if (prior(a) != prior(b)) return prior(a) > prior(b);
        if (!(a.id == b.id)) return a.id < b.id;
        return a.payload < b.payload;
    });
    if (words.empty()) {
        std::vector<SearchResult> pinned;
        for (const auto& id : profile.pins) {
            SearchResult result;
            if (!ResolveResult(catalog, profile, id, result)) continue;
            if (id.kind == ResultKind::Emoji) {
                const auto* variant = catalog.PreferredVariant(*catalog.FindFamily({id.value}), profile.settings.skinTone);
                result.payload = variant->glyph; result.label = variant->name;
            }
            result.explanation = L"Favorite"; pinned.push_back(result);
        }
        candidates.insert(candidates.begin(), pinned.begin(), pinned.end());
    }
    std::set<ResultId> added;
    std::vector<SearchResult> results;
    for (const auto& candidate : candidates) if (added.insert(candidate.id).second) results.push_back(candidate);
    return results;
}

}
