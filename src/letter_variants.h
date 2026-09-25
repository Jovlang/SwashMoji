#pragma once
#include "personalization.h"
#include "ranking.h"
#include <algorithm>
#include <limits>

namespace SwashMoji {
// Explicit paired cases avoid locale-dependent casing (notably i/I and ß/ẞ).
inline std::wstring LetterVariants(wchar_t base) {
    const bool upper = base >= L'A' && base <= L'Z';
    if (upper) base += L'a' - L'A';
    const wchar_t* lower = L"";
    const wchar_t* caps = L"";
    switch (base) {
    case L'a': lower = L"äáàâãåæāăą"; caps = L"ÄÁÀÂÃÅÆĀĂĄ"; break;
    case L'c': lower = L"çćčĉċ"; caps = L"ÇĆČĈĊ"; break;
    case L'd': lower = L"ðďđ"; caps = L"ÐĎĐ"; break;
    case L'e': lower = L"éèêëēėęěĕ"; caps = L"ÉÈÊËĒĖĘĚĔ"; break;
    case L'g': lower = L"ğģĝġ"; caps = L"ĞĢĜĠ"; break;
    case L'h': lower = L"ħĥ"; caps = L"ĦĤ"; break;
    case L'i': lower = L"íìîïīįıĩĭ"; caps = L"ÍÌÎÏĪĮİĨĬ"; break;
    case L'j': lower = L"ĵ"; caps = L"Ĵ"; break;
    case L'k': lower = L"ķ"; caps = L"Ķ"; break;
    case L'l': lower = L"łĺľļŀ"; caps = L"ŁĹĽĻĿ"; break;
    case L'n': lower = L"ñńňņŋ"; caps = L"ÑŃŇŅŊ"; break;
    case L'o': lower = L"öóòôõøœōőŏ"; caps = L"ÖÓÒÔÕØŒŌŐŎ"; break;
    case L'r': lower = L"řŕŗ"; caps = L"ŘŔŖ"; break;
    case L's': lower = L"ßśšşŝș"; caps = L"ẞŚŠŞŜȘ"; break;
    case L't': lower = L"þťţŧț"; caps = L"ÞŤŢŦȚ"; break;
    case L'u': lower = L"üúùûūůűŭũų"; caps = L"ÜÚÙÛŪŮŰŬŨŲ"; break;
    case L'w': lower = L"ŵ"; caps = L"Ŵ"; break;
    case L'y': lower = L"ýÿŷ"; caps = L"ÝŸŶ"; break;
    case L'z': lower = L"žźż"; caps = L"ŽŹŻ"; break;
    }
    return upper ? caps : lower;
}
inline bool ValidLetterChoice(const std::wstring& key) {
    return key.size() == 2 && LetterVariants(key[0]).find(key[1]) != std::wstring::npos;
}
inline std::vector<SearchResult> SearchLetterVariants(const std::wstring& query,
        const std::map<std::wstring, unsigned int>& counts,
        const std::vector<std::wstring>& history = {}, bool sortByUsage = true) {
    std::vector<SearchResult> results;
    if (query.size() > 1) return results;
    const std::wstring bases = query.empty() ? L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" : query;
    for (auto base : bases) for (auto letter : LetterVariants(base)) {
        const std::wstring payload(1, letter);
        // The global and filtered views use the same stable choice key.
        // These transient picker IDs never enter emoji/combination history.
        results.push_back({{ResultKind::Emoji, std::wstring(1, base) + payload}, payload, payload, {}, {}});
    }
    auto count = [&](const SearchResult& result) {
        const auto found = counts.find(result.id.value);
        return found == counts.end() ? 0u : found->second;
    };
    auto recency = [&](const SearchResult& result) {
        const auto found = std::find(history.begin(), history.end(), result.id.value);
        return found == history.end() ? 0 : static_cast<int>(history.end() - found);
    };
    std::stable_sort(results.begin(), results.end(), [&](const auto& a, const auto& b) {
        return SwashMojiRanking::ComparePreference(sortByUsage, count(a), recency(a), count(b), recency(b)) > 0;
    });
    return results;
}
inline void RecordLetterChoice(Profile& profile, const std::wstring& key) {
    if (!profile.settings.learnQueries || !ValidLetterChoice(key)) return;
    auto& history = profile.letterHistory;
    history.erase(std::remove(history.begin(), history.end(), key), history.end());
    history.insert(history.begin(), key);
    if (history.size() > kMaxHistory) history.resize(kMaxHistory);
    auto& count = profile.letterUsage[key];
    if (count != std::numeric_limits<unsigned int>::max()) ++count;
}
}
