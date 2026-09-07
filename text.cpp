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
    std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::towlower(c));
    });
    return text;
}

std::vector<std::wstring> SplitWords(const std::wstring& text) {
    std::vector<std::wstring> words;
    size_t start = 0;
    while (start < text.size()) {
        while (start < text.size() && !std::iswalnum(text[start])) ++start;
        if (start == text.size()) break;
        size_t end = start;
        while (end < text.size() && std::iswalnum(text[end])) ++end;
        words.push_back(text.substr(start, end - start));
        start = end;
    }
    return words;
}

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
