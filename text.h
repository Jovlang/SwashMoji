#pragma once
#include <string>
#include <vector>

namespace SwashMoji {
std::wstring Utf8ToWide(const std::string& value);
std::string WideToUtf8(const std::wstring& value);
std::wstring Lower(std::wstring text);
std::wstring NormalizePhrase(const std::wstring& text);
std::vector<std::wstring> SplitWords(const std::wstring& text);
bool StartsWith(const std::wstring& word, const std::wstring& prefix);
std::wstring NormalizeSearchWord(const std::wstring& word);
std::wstring JoinWords(const std::vector<std::wstring>& words);
}
