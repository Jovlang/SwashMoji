#include <windows.h>
#include "text.h"
#include <algorithm>
#include <cwctype>
#include <unordered_map>

namespace SwashMoji {
std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) return L"";
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (!length) return L"";
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                        result.data(), length);
    return result;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) return "";
    const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (!length) return "";
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                        result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring Lower(std::wstring text) {
    if (text.empty()) return {};
    int size = NormalizeString(NormalizationKC, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return {};
    std::wstring normalized(size, L'\0');
    int length = NormalizeString(NormalizationKC, text.data(), static_cast<int>(text.size()), normalized.data(), size);
    if (length < 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        normalized.resize(-length);
        length = NormalizeString(NormalizationKC, text.data(), static_cast<int>(text.size()), normalized.data(), static_cast<int>(normalized.size()));
    }
    if (length <= 0) return {};
    normalized.resize(length);
    size = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, normalized.data(), length, nullptr, 0, nullptr, nullptr, 0);
    if (!size) return {};
    std::wstring lower(size, L'\0');
    if (!LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, normalized.data(), length, lower.data(), size, nullptr, nullptr, 0)) return {};
    return lower;
}

std::vector<std::wstring> SplitWords(const std::wstring& text) {
    std::vector<std::wstring> words;
    std::vector<WORD> types(text.size());
    if (text.empty() || !GetStringTypeW(CT_CTYPE1, text.data(), static_cast<int>(text.size()), types.data())) return words;
    const auto isWord = [&types](size_t i) { return (types[i] & (C1_ALPHA | C1_DIGIT)) != 0; };
    size_t start = 0;
    while (start < text.size()) {
        while (start < text.size() && !isWord(start)) ++start;
        if (start == text.size()) break;
        size_t end = start;
        while (end < text.size() && isWord(end)) ++end;
        words.push_back(text.substr(start, end - start));
        start = end;
    }
    return words;
}

std::wstring NormalizePhrase(const std::wstring& text) { return JoinWords(SplitWords(Lower(text))); }

bool StartsWith(const std::wstring& word, const std::wstring& prefix) {
    return word.size() >= prefix.size() && word.compare(0, prefix.size(), prefix) == 0;
}

std::wstring NormalizeSearchWord(const std::wstring& word) {
    static const std::unordered_map<std::wstring, std::wstring> forms{
        {L"smiles", L"smile"}, {L"smiled", L"smile"}, {L"smiling", L"smile"},
        {L"grins", L"grin"}, {L"grinned", L"grin"}, {L"grinning", L"grin"},
        {L"laughs", L"laugh"}, {L"laughed", L"laugh"}, {L"laughing", L"laugh"},
        {L"cries", L"cry"}, {L"cried", L"cry"}, {L"crying", L"cry"},
        {L"rolls", L"roll"}, {L"rolled", L"roll"}, {L"rolling", L"roll"},
        {L"blushes", L"blush"}, {L"blushed", L"blush"}, {L"blushing", L"blush"}
    };
    const auto known = forms.find(word);
    if (known != forms.end()) return known->second;
    if (word.size() > 3 && word.back() == L's' && word.compare(word.size() - 2, 2, L"ss") != 0) {
        return word.substr(0, word.size() - 1);
    }
    return word;
}

std::wstring JoinWords(const std::vector<std::wstring>& words) {
    std::wstring result;
    for (const auto& word : words) {
        if (!result.empty()) result += L' ';
        result += word;
    }
    return result;
}

}
