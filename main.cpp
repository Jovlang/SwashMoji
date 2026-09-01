#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <d2d1.h>
#include <dwrite.h>
#include <dwrite_3.h>
#include <dwmapi.h>
#include <uiautomation.h>

#include "ranking.h"

#include <algorithm>
#include <cstring>
#include <cwctype>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

constexpr wchar_t kClassName[] = L"SwashMojiWindow";
constexpr wchar_t kHelpClassName[] = L"SwashMojiHelpWindow";
constexpr UINT kHotkeyId = 1;
constexpr int kPickerWidth = 500;
constexpr int kPickerHeight = 136;
constexpr int kInputHeight = 34;
constexpr int kResultSize = 48;
constexpr int kEmojiColumns = 10;
constexpr int kMinEmojiRows = 1;
constexpr int kMaxEmojiRows = 3;
constexpr int kStatusHeight = 18;
constexpr int kHelpWidth = 720;
constexpr int kHelpHeight = 840;
constexpr int kEditId = 100;
constexpr int kListId = 101;
constexpr int kStatusId = 102;
constexpr int kExitId = 200;
constexpr int kPositionAboveTextFieldId = 201;
constexpr int kSortRecentId = 202;
constexpr int kSortMostUsedId = 203;
constexpr int kClearUsageHistoryId = 204;
constexpr int kAppIconId = 101;
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kShowPickerMessage = WM_APP + 2;
constexpr UINT_PTR kStatusTimerId = 1;
constexpr UINT_PTR kRestorePickerFocusTimerId = 2;
constexpr size_t kMaxHistory = 40;
constexpr COLORREF kBackground = RGB(24, 24, 24);
constexpr COLORREF kInputBackground = RGB(35, 35, 35);
constexpr COLORREF kText = RGB(235, 235, 235);
constexpr COLORREF kMutedText = RGB(155, 155, 155);
constexpr COLORREF kSelected = RGB(38, 79, 120);

struct Emoji {
    std::wstring glyph;
    std::wstring name;
    std::wstring keywords;
    std::wstring lowerName;
    std::wstring lowerKeywords;
    std::vector<std::wstring> nameWords;
    std::vector<std::wstring> keywordWords;
};

struct EmojiFont {
    std::wstring name;
    bool color;
};

HWND g_window{};
HWND g_edit{};
HWND g_list{};
HWND g_status{};
HWND g_helpWindow{};
HWND g_targetWindow{};
bool g_positionAboveTextField{};
bool g_sortByUsage{};
bool g_statusVisible{true};
int g_emojiRows{kMinEmojiRows};
int g_skinToneIndex{};
WNDPROC g_editProc{};
WNDPROC g_listProc{};
HBRUSH g_backgroundBrush{};
HBRUSH g_inputBrush{};
HICON g_appIcon{};
HFONT g_uiFont{};
HFONT g_statusFont{};
HFONT g_helpTitleFont{};
HFONT g_helpHeadingFont{};
HFONT g_helpBodyFont{};
HFONT g_emojiFont{};
size_t g_emojiFontIndex{};
std::vector<EmojiFont> g_emojiFonts;
NOTIFYICONDATAW g_tray{};
ID2D1Factory* g_d2dFactory{};
ID2D1DCRenderTarget* g_d2dTarget{};
IDWriteFactory3* g_dwriteFactory{};
IUIAutomation* g_uiAutomation{};
bool g_comInitialized{};
IDWriteTextFormat* g_emojiFormat{};
std::vector<const Emoji*> g_visible;
std::vector<const Emoji*> g_displayVisible;
std::vector<std::wstring> g_history;
std::unordered_map<std::wstring, unsigned int> g_usageCounts;
std::vector<Emoji> g_emojis;

int SkinToneIndex(const std::wstring& glyph) {
    for (size_t index = 0; index + 1 < glyph.size(); ++index) {
        if (glyph[index] == 0xD83C && glyph[index + 1] >= 0xDFFB && glyph[index + 1] <= 0xDFFF) {
            return glyph[index + 1] - 0xDFFB + 1;
        }
    }
    return 0;
}

bool UsesOnlySkinTone(const std::wstring& glyph, int tone) {
    bool found{};
    for (size_t index = 0; index + 1 < glyph.size(); ++index) {
        if (glyph[index] != 0xD83C || glyph[index + 1] < 0xDFFB || glyph[index + 1] > 0xDFFF) continue;
        found = true;
        if (glyph[index + 1] - 0xDFFB + 1 != tone) return false;
        ++index;
    }
    return found;
}

std::wstring WithoutSkinTone(const std::wstring& glyph) {
    std::wstring result;
    result.reserve(glyph.size());
    for (size_t index = 0; index < glyph.size(); ++index) {
        if (index + 1 < glyph.size() && glyph[index] == 0xD83C &&
            glyph[index + 1] >= 0xDFFB && glyph[index + 1] <= 0xDFFF) {
            ++index;
            continue;
        }
        result.push_back(glyph[index]);
    }
    return result;
}

std::wstring SkinToneFamilyKey(const std::wstring& glyph) {
    std::wstring key = WithoutSkinTone(glyph);
    key.erase(std::remove(key.begin(), key.end(), static_cast<wchar_t>(0xFE0F)), key.end());
    return key;
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

struct TokenMatch {
    int tier{};
    int detail{};
    bool fromName{};
};

struct MatchScore {
    int tier{};
    int detail{};
};

TokenMatch ScoreToken(const Emoji& emoji, const std::wstring& token) {
    for (const auto& word : emoji.nameWords) if (word == token) return {5, 100, true};
    const std::wstring normalizedToken = NormalizeSearchWord(token);
    for (const auto& word : emoji.nameWords) {
        if (NormalizeSearchWord(word) == normalizedToken) return {5, 90, true};
    }
    for (const auto& word : emoji.keywordWords) if (word == token) return {4, 100, false};
    for (const auto& word : emoji.keywordWords) {
        if (NormalizeSearchWord(word) == normalizedToken) return {4, 90, false};
    }
    for (const auto& word : emoji.nameWords) if (StartsWith(word, token)) return {3, 80, true};
    for (const auto& word : emoji.keywordWords) if (StartsWith(word, token)) return {2, 70, false};
    if (emoji.lowerName.find(token) != std::wstring::npos) return {2, 60, true};
    if (emoji.lowerKeywords.find(token) != std::wstring::npos) return {1, 50, false};
    return {};
}

std::wstring JoinWords(const std::vector<std::wstring>& words) {
    std::wstring result;
    for (const auto& word : words) {
        if (!result.empty()) result += L' ';
        result += word;
    }
    return result;
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

std::wstring EmojiCatalogPath() {
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
    std::wstring executable(path);
    const size_t slash = executable.find_last_of(L"\\/");
    return (slash == std::wstring::npos ? L"" : executable.substr(0, slash + 1)) + L"emojis.txt";
}

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

bool LoadEmojis() {
    const std::wstring path = EmojiCatalogPath();
    std::ifstream file(path.c_str(), std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::string glyphUtf8;
        std::string nameUtf8;
        std::string keywordsUtf8;
        const size_t firstTab = line.find('\t');
        if (firstTab != std::string::npos) {
            const size_t secondTab = line.find('\t', firstTab + 1);
            glyphUtf8 = line.substr(0, firstTab);
            nameUtf8 = line.substr(firstTab + 1, secondTab - firstTab - 1);
            if (secondTab != std::string::npos) keywordsUtf8 = line.substr(secondTab + 1);
        } else {
            const size_t split = line.find(' ');
            if (split == std::string::npos) continue;
            glyphUtf8 = line.substr(0, split);
            nameUtf8 = line.substr(split + 1);
        }
        const std::wstring glyph = Utf8ToWide(glyphUtf8);
        const std::wstring name = Utf8ToWide(nameUtf8);
        std::wstring keywords = Utf8ToWide(keywordsUtf8);
        if (keywords.empty()) keywords = name;
        if (!glyph.empty() && !name.empty()) {
            const std::wstring lowerName = Lower(name);
            const std::wstring lowerKeywords = Lower(keywords);
            g_emojis.push_back({glyph, name, keywords, lowerName, lowerKeywords,
                                SplitWords(lowerName), SplitWords(lowerKeywords)});
        }
    }
    // Some CLDR families contain only toned variants (for example, waving hand).
    // Add an in-memory untoned entry so they can be presented once and changed
    // with Alt+I like families that already have a default glyph in the catalog.
    std::unordered_set<std::wstring> untonedFamilies;
    for (const auto& emoji : g_emojis) {
        if (!SkinToneIndex(emoji.glyph)) untonedFamilies.insert(SkinToneFamilyKey(emoji.glyph));
    }
    const size_t catalogSize = g_emojis.size();
    for (size_t index = 0; index < catalogSize; ++index) {
        const Emoji& emoji = g_emojis[index];
        if (!SkinToneIndex(emoji.glyph)) continue;
        if (!UsesOnlySkinTone(emoji.glyph, SkinToneIndex(emoji.glyph))) continue;
        const std::wstring base = WithoutSkinTone(emoji.glyph);
        if (!untonedFamilies.insert(SkinToneFamilyKey(emoji.glyph)).second) continue;
        Emoji synthetic = emoji;
        synthetic.glyph = base;
        g_emojis.push_back(std::move(synthetic));
    }
    return !g_emojis.empty();
}

std::wstring DataPath(const wchar_t* fileName) {
    wchar_t path[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) return fileName;
    const std::wstring appData(path);
    const std::wstring folder = appData + L"\\SwashMoji";
    CreateDirectoryW(folder.c_str(), nullptr);
    const std::wstring target = folder + L"\\" + fileName;
    if (GetFileAttributesW(target.c_str()) == INVALID_FILE_ATTRIBUTES) {
        static const wchar_t* legacyFolders[]{L"WinMoji"};
        for (const wchar_t* legacyFolder : legacyFolders) {
            const std::wstring legacy = appData + L"\\" + legacyFolder + L"\\" + fileName;
            if (CopyFileW(legacy.c_str(), target.c_str(), TRUE)) break;
        }
    }
    return target;
}

std::wstring HistoryPath() {
    return DataPath(L"history.txt");
}

std::wstring SettingsPath() {
    return DataPath(L"settings.txt");
}

void LoadSettings() {
    std::ifstream file(SettingsPath().c_str(), std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line == "position_above_text_field=1") g_positionAboveTextField = true;
        if (line == "sort_by_usage=1") g_sortByUsage = true;
        if (line == "emoji_rows=2") g_emojiRows = 2;
        if (line == "emoji_rows=3") g_emojiRows = 3;
        if (line == "skin_tone=1") g_skinToneIndex = 1;
        if (line == "skin_tone=2") g_skinToneIndex = 2;
        if (line == "skin_tone=3") g_skinToneIndex = 3;
        if (line == "skin_tone=4") g_skinToneIndex = 4;
        if (line == "skin_tone=5") g_skinToneIndex = 5;
    }
}

void SaveSettings() {
    std::ofstream file(SettingsPath().c_str(), std::ios::binary | std::ios::trunc);
    file << "position_above_text_field=" << (g_positionAboveTextField ? '1' : '0') << '\n';
    file << "sort_by_usage=" << (g_sortByUsage ? '1' : '0') << '\n';
    file << "emoji_rows=" << g_emojiRows << '\n';
    file << "skin_tone=" << g_skinToneIndex << '\n';
}

std::wstring UsagePath() {
    return DataPath(L"usage.txt");
}

void LoadUsage() {
    std::ifstream file(UsagePath().c_str(), std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const std::wstring glyph = Utf8ToWide(line.substr(0, tab));
        try {
            const unsigned long count = std::stoul(line.substr(tab + 1));
            if (!glyph.empty() && count) g_usageCounts[glyph] = static_cast<unsigned int>(count);
        } catch (...) {
        }
    }
    for (const auto& recent : g_history) {
        if (!g_usageCounts.count(recent)) g_usageCounts[recent] = 1;
    }
}

void SaveUsage() {
    std::ofstream file(UsagePath().c_str(), std::ios::binary | std::ios::trunc);
    for (const auto& emoji : g_emojis) {
        const auto found = g_usageCounts.find(emoji.glyph);
        if (found != g_usageCounts.end() && found->second) {
            file << WideToUtf8(emoji.glyph) << '\t' << found->second << '\n';
        }
    }
}

void LoadHistory() {
    const std::wstring path = HistoryPath();
    std::ifstream file(path.c_str(), std::ios::binary);
    std::string line;
    while (std::getline(file, line) && g_history.size() < kMaxHistory) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::wstring emoji = Utf8ToWide(line);
        if (!emoji.empty()) g_history.push_back(emoji);
    }
}

void SaveHistory() {
    const std::wstring path = HistoryPath();
    std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);
    for (const auto& item : g_history) file << WideToUtf8(item) << '\n';
}

void Remember(const wchar_t* glyph) {
    std::wstring value(glyph);
    g_history.erase(std::remove(g_history.begin(), g_history.end(), value), g_history.end());
    g_history.insert(g_history.begin(), value);
    if (g_history.size() > kMaxHistory) g_history.resize(kMaxHistory);
    SaveHistory();
    ++g_usageCounts[value];
    SaveUsage();
}

int HistoryBoost(const std::wstring& glyph) {
    const auto found = std::find(g_history.begin(), g_history.end(), glyph);
    if (found == g_history.end()) return 0;
    const int position = static_cast<int>(std::distance(g_history.begin(), found));
    return std::max(1, 20 - position);
}

unsigned int UsageCount(const std::wstring& glyph) {
    const auto found = g_usageCounts.find(glyph);
    return found == g_usageCounts.end() ? 0 : found->second;
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

const Emoji* PreferredSkinToneVariant(const Emoji* emoji) {
    if (!emoji || !g_skinToneIndex) return emoji;
    const std::wstring family = SkinToneFamilyKey(emoji->glyph);
    for (const auto& candidate : g_emojis) {
        if (UsesOnlySkinTone(candidate.glyph, g_skinToneIndex) &&
            SkinToneFamilyKey(candidate.glyph) == family) {
            return &candidate;
        }
    }
    return emoji;
}

void RefreshList() {
    wchar_t input[256]{};
    GetWindowTextW(g_edit, input, static_cast<int>(std::size(input)));
    const std::wstring query = Lower(input);
    const auto queryWords = SplitWords(query);
    SendMessageW(g_list, LB_RESETCONTENT, 0, 0);
    g_visible.clear();
    g_displayVisible.clear();
    std::unordered_set<std::wstring> added;

    // Empty searches lead with either strict recency or lifetime usage, according
    // to the user's Alt+T preference.
    if (queryWords.empty()) {
        if (g_sortByUsage) {
            std::vector<const Emoji*> used;
            for (const auto& emoji : g_emojis) {
                if (UsageCount(emoji.glyph)) used.push_back(&emoji);
            }
            std::stable_sort(used.begin(), used.end(), [](const Emoji* left, const Emoji* right) {
                const unsigned int leftUsage = UsageCount(left->glyph);
                const unsigned int rightUsage = UsageCount(right->glyph);
                return SwashMojiRanking::ComparePreference(true,
                                                           leftUsage, HistoryBoost(left->glyph),
                                                           rightUsage, HistoryBoost(right->glyph)) > 0;
            });
            for (const Emoji* emoji : used) {
                if (added.insert(emoji->glyph).second) g_visible.push_back(emoji);
            }
        } else {
            for (const auto& recent : g_history) {
                for (const auto& emoji : g_emojis) {
                    if (recent == emoji.glyph && added.insert(recent).second) {
                        g_visible.push_back(&emoji);
                        break;
                    }
                }
            }
        }
        for (const auto& emoji : g_emojis) {
            if (added.insert(emoji.glyph).second) g_visible.push_back(&emoji);
        }
    } else {
        struct SearchResult {
            const Emoji* emoji;
            MatchScore match;
            unsigned int usage;
            int recency;
            int popularity;
        };
        std::vector<SearchResult> results;
        const auto addResult = [&results](const Emoji& emoji, MatchScore match) {
            results.push_back({&emoji, match, UsageCount(emoji.glyph),
                               HistoryBoost(emoji.glyph), PopularityPrior(emoji.glyph)});
        };
        for (const auto& emoji : g_emojis) {
            const MatchScore match = LexicalScore(emoji, queryWords);
            if (match.tier) addResult(emoji, match);
        }

        // Typo tolerance is deliberately a fallback so it never pollutes good exact results.
        if (results.empty()) {
            for (const auto& emoji : g_emojis) {
                const int score = FuzzyScore(emoji, queryWords);
                if (score >= 0) addResult(emoji, {1, score});
            }
        }
        std::stable_sort(results.begin(), results.end(), [](const SearchResult& left, const SearchResult& right) {
            if (left.match.tier != right.match.tier) return left.match.tier > right.match.tier;
            const int preference = SwashMojiRanking::ComparePreference(
                g_sortByUsage, left.usage, left.recency, right.usage, right.recency);
            if (preference) return preference > 0;
            if (left.match.detail != right.match.detail) return left.match.detail > right.match.detail;
            return left.popularity > right.popularity;
        });
        for (const auto& result : results) {
            if (added.insert(result.emoji->glyph).second) g_visible.push_back(result.emoji);
        }
    }
    // Tone variants are selected through Alt+I, not exposed as separate results.
    // Mixed-tone-only sequences are omitted because one global tone cannot
    // represent both independent modifiers.
    g_visible.erase(std::remove_if(g_visible.begin(), g_visible.end(), [](const Emoji* emoji) {
        return SkinToneIndex(emoji->glyph) != 0;
    }), g_visible.end());
    // A multi-column Win32 listbox fills each column top-to-bottom. Reorder the
    // listbox items so it still reads left-to-right: 1–10 on the first row, then
    // 11–20 on the next row.
    const size_t columns = (g_visible.size() + g_emojiRows - 1) / g_emojiRows;
    for (size_t column = 0; column < columns; ++column) {
        const size_t page = column / kEmojiColumns;
        const size_t columnInPage = column % kEmojiColumns;
        for (int row = 0; row < g_emojiRows; ++row) {
            const size_t index = page * g_emojiRows * kEmojiColumns +
                                 static_cast<size_t>(row) * kEmojiColumns + columnInPage;
            if (index < g_visible.size()) {
                g_displayVisible.push_back(PreferredSkinToneVariant(g_visible[index]));
            }
        }
    }
    for (const Emoji* emoji : g_displayVisible) {
        SendMessageW(g_list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(emoji->glyph.c_str()));
    }
    if (!g_displayVisible.empty()) SendMessageW(g_list, LB_SETCURSEL, 0, 0);
}

void CopySelection() {
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_displayVisible.size())) return;
    const std::wstring value = g_displayVisible[selected]->glyph;
    if (!OpenClipboard(g_window)) return;
    EmptyClipboard();
    const size_t bytes = (value.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
        auto* output = static_cast<wchar_t*>(GlobalLock(memory));
        if (output) {
            memcpy(output, value.c_str(), bytes);
            GlobalUnlock(memory);
            if (SetClipboardData(CF_UNICODETEXT, memory)) { // Clipboard owns memory after success.
                memory = nullptr;
                Remember(value.c_str());
            }
        }
        if (memory) GlobalFree(memory);
    }
    CloseClipboard();
    ShowWindow(g_window, SW_HIDE);
}

void InsertSelection(bool keepOpen = false, bool controlHeld = false) {
    const int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(g_displayVisible.size())) return;
    const std::wstring value = g_displayVisible[selected]->glyph;
    const HWND target = g_targetWindow;
    if (!keepOpen) ShowWindow(g_window, SW_HIDE);
    if (!target || !IsWindow(target) || value.empty()) return;

    SetForegroundWindow(target);
    std::vector<INPUT> inputs;
    inputs.reserve(value.size() * 2 + (controlHeld ? 2 : 0));
    if (controlHeld) {
        // Ctrl is still held when Ctrl+Enter reaches this handler. Release its logical
        // state before typing so the target receives Unicode text, not a shortcut.
        INPUT controlUp{};
        controlUp.type = INPUT_KEYBOARD;
        controlUp.ki.wVk = VK_CONTROL;
        controlUp.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(controlUp);
    }
    for (const wchar_t codeUnit : value) {
        INPUT down{};
        down.type = INPUT_KEYBOARD;
        down.ki.wScan = codeUnit;
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(down);
        INPUT up = down;
        up.ki.dwFlags |= KEYEVENTF_KEYUP;
        inputs.push_back(up);
    }
    if (controlHeld) {
        // Restore Ctrl's logical state so another Enter works while the user keeps
        // holding the physical key. The eventual physical key-up releases it normally.
        INPUT controlDown{};
        controlDown.type = INPUT_KEYBOARD;
        controlDown.ki.wVk = VK_CONTROL;
        inputs.push_back(controlDown);
    }
    if (SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)) == inputs.size()) {
        Remember(value.c_str());
    }
    if (keepOpen) {
        // Leave the target focused long enough for its input queue to consume the
        // injected text before returning keyboard focus to the still-visible picker.
        SetTimer(g_window, kRestorePickerFocusTimerId, 75, nullptr);
    }
}

bool TryGetAutomationTextFieldAnchor(RECT& anchor) {
    if (!g_uiAutomation) return false;
    IUIAutomationElement* focused{};
    if (FAILED(g_uiAutomation->GetFocusedElement(&focused)) || !focused) return false;
    CONTROLTYPEID type{};
    RECT bounds{};
    const bool isTextField = SUCCEEDED(focused->get_CurrentControlType(&type)) &&
        (type == UIA_EditControlTypeId || type == UIA_DocumentControlTypeId ||
         type == UIA_ComboBoxControlTypeId);
    const bool hasBounds = SUCCEEDED(focused->get_CurrentBoundingRectangle(&bounds)) &&
        bounds.right > bounds.left && bounds.bottom > bounds.top;
    focused->Release();
    if (!isTextField || !hasBounds) return false;
    anchor = bounds;
    return true;
}

bool TryGetTextFieldAnchor(HWND active, RECT& anchor) {
    if (TryGetAutomationTextFieldAnchor(anchor)) return true;

    GUITHREADINFO info{sizeof(info)};
    const DWORD thread = GetWindowThreadProcessId(active, nullptr);
    if (!thread || !GetGUIThreadInfo(thread, &info)) return false;
    if (info.hwndCaret) {
        anchor = info.rcCaret;
        MapWindowPoints(info.hwndCaret, nullptr, reinterpret_cast<POINT*>(&anchor), 2);
        if (anchor.right <= anchor.left) anchor.right = anchor.left + 1;
        if (anchor.bottom <= anchor.top) anchor.bottom = anchor.top + 1;
        return true;
    }
    if (!info.hwndFocus || GetAncestor(info.hwndFocus, GA_ROOT) == info.hwndFocus) return false;
    return GetWindowRect(info.hwndFocus, &anchor);
}

int PickerHeight() {
    const int listHeight = g_emojiRows * kResultSize;
    return kPickerHeight + listHeight - kResultSize - (g_statusVisible ? 0 : kStatusHeight + 5);
}

void CenterOnActiveMonitor() {
    HWND active = GetForegroundWindow();
    if (active && active != g_window) g_targetWindow = GetAncestor(active, GA_ROOT);
    RECT anchor{};
    const bool hasAnchor = g_positionAboveTextField && TryGetTextFieldAnchor(active, anchor);
    HMONITOR monitor = hasAnchor
        ? MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST)
        : MonitorFromWindow(active, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    const RECT& area = info.rcWork;
    int x = area.left + ((area.right - area.left) - kPickerWidth) / 2;
    const int pickerHeight = PickerHeight();
    int y = area.top + ((area.bottom - area.top) - pickerHeight) / 2;
    if (hasAnchor) {
        x = anchor.left + (anchor.right - anchor.left) / 2 - kPickerWidth / 2;
        y = anchor.top - pickerHeight - 8;
        if (y < area.top) y = anchor.bottom + 8;
        const int minX = static_cast<int>(area.left);
        const int minY = static_cast<int>(area.top);
        const int maxX = std::max(minX, static_cast<int>(area.right) - kPickerWidth);
        const int maxY = std::max(minY, static_cast<int>(area.bottom) - pickerHeight);
        x = std::clamp(x, minX, maxX);
        y = std::clamp(y, minY, maxY);
    }
    SetWindowPos(g_window, HWND_TOPMOST, x, y, kPickerWidth, pickerHeight,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    SetForegroundWindow(g_window);
    SetFocus(g_edit);
    SendMessageW(g_edit, EM_SETSEL, 0, -1);
}

void UpdateStatusLine() {
    if (!g_status || g_emojiFonts.empty()) return;
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    const std::wstring label = font.name + (font.color ? L" · Color" : L" · Monochrome");
    const std::wstring text = label + L"    Tab: font    Alt+I: skin tone    Alt+1-3: rows    F1: help";
    SetWindowTextW(g_status, text.c_str());
}

void ShowFontToast() {
    if (!g_status || g_emojiFonts.empty()) return;
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    const std::wstring text = std::wstring(L"Font: ") + font.name +
                              (font.color ? L" · Color" : L" · Monochrome");
    SetWindowTextW(g_status, text.c_str());
    KillTimer(g_window, kStatusTimerId);
    SetTimer(g_window, kStatusTimerId, 800, nullptr);
}

void ToggleStatusLine() {
    g_statusVisible = !g_statusVisible;
    KillTimer(g_window, kStatusTimerId);
    ShowWindow(g_status, g_statusVisible ? SW_SHOW : SW_HIDE);
    if (g_statusVisible) UpdateStatusLine();
    SetWindowPos(g_window, nullptr, 0, 0, kPickerWidth, PickerHeight(),
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void SetEmojiRows(int rows) {
    rows = std::clamp(rows, kMinEmojiRows, kMaxEmojiRows);
    if (rows == g_emojiRows) return;
    RECT bounds{};
    GetWindowRect(g_window, &bounds);
    g_emojiRows = rows;
    SaveSettings();
    RefreshList();
    const int height = PickerHeight();
    const HMONITOR monitor = MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor, &info);
    const int y = std::max(static_cast<int>(info.rcWork.top), static_cast<int>(bounds.bottom) - height);
    SetWindowPos(g_window, nullptr, bounds.left, y, kPickerWidth, height,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void CycleSkinTone() {
    static const wchar_t* names[]{L"Default", L"Light", L"Medium-light", L"Medium",
                                  L"Medium-dark", L"Dark"};
    g_skinToneIndex = (g_skinToneIndex + 1) % 6;
    SaveSettings();
    RefreshList();
    if (g_statusVisible) {
        const std::wstring text = std::wstring(L"Skin tone: ") + names[g_skinToneIndex];
        SetWindowTextW(g_status, text.c_str());
        KillTimer(g_window, kStatusTimerId);
        SetTimer(g_window, kStatusTimerId, 800, nullptr);
    }
}

void SelectEmojiFont(size_t index) {
    if (g_emojiFonts.empty()) return;
    g_emojiFontIndex = index % g_emojiFonts.size();
    const EmojiFont& font = g_emojiFonts[g_emojiFontIndex];
    if (g_emojiFont) DeleteObject(g_emojiFont);
    g_emojiFont = CreateFontW(-30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH, font.name.c_str());
    if (g_emojiFormat) {
        g_emojiFormat->Release();
        g_emojiFormat = nullptr;
    }
    if (g_dwriteFactory) {
        g_dwriteFactory->CreateTextFormat(font.name.c_str(), nullptr,
                                          DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL, 30.0f, L"",
                                          &g_emojiFormat);
        if (g_emojiFormat) {
            g_emojiFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            g_emojiFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }
    if (g_window) {
        const std::wstring title = std::wstring(L"SwashMoji — ") + font.name +
                                   (font.color ? L" · Color" : L" · Monochrome");
        SetWindowTextW(g_window, title.c_str());
    }
    if (g_list) InvalidateRect(g_list, nullptr, TRUE);
}

void CycleEmojiFont() {
    SelectEmojiFont(g_emojiFontIndex + 1);
    ShowFontToast();
}

void InitializeColorEmojiDrawing() {
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_d2dFactory))) return;
    D2D1_RENDER_TARGET_PROPERTIES properties{};
    properties.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    properties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
    if (FAILED(g_d2dFactory->CreateDCRenderTarget(&properties, &g_d2dTarget))) return;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3),
                                   reinterpret_cast<IUnknown**>(&g_dwriteFactory)))) return;
}

void LoadInstalledColorEmojiFonts() {
    if (!g_dwriteFactory) {
        g_emojiFonts.push_back({L"Segoe UI Emoji", false});
        return;
    }
    IDWriteFontCollection* collection{};
    IDWriteFactory* baseFactory = g_dwriteFactory;
    if (FAILED(baseFactory->GetSystemFontCollection(&collection, FALSE))) return;
    for (UINT32 i = 0; i < collection->GetFontFamilyCount(); ++i) {
        IDWriteFontFamily* family{};
        IDWriteFont* font{};
        IDWriteFontFace* face{};
        IDWriteFontFace4* colorFace{};
        IDWriteLocalizedStrings* names{};
        if (FAILED(collection->GetFontFamily(i, &family)) ||
            FAILED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                                DWRITE_FONT_STYLE_NORMAL, &font)) ||
            FAILED(font->CreateFontFace(&face)) ||
            FAILED(face->QueryInterface(&colorFace)) || !colorFace->IsColorFont()) {
            if (colorFace) colorFace->Release();
            if (face) face->Release();
            if (font) font->Release();
            if (family) family->Release();
            continue;
        }
        const BOOL containsGrinningFace = colorFace->HasCharacter(0x1F600);
        if (containsGrinningFace && SUCCEEDED(family->GetFamilyNames(&names))) {
            UINT32 locale = 0;
            BOOL foundLocale = FALSE;
            names->FindLocaleName(L"en-us", &locale, &foundLocale);
            if (!foundLocale) locale = 0;
            UINT32 length = 0;
            names->GetStringLength(locale, &length);
            std::wstring name(length + 1, L'\0');
            names->GetString(locale, name.data(), length + 1);
            name.resize(length);
            g_emojiFonts.push_back({name, true});
            names->Release();
        }
        colorFace->Release();
        face->Release();
        font->Release();
        family->Release();
    }
    collection->Release();
    // Keep genuine GDI-style monochrome rendering available after the color choices.
    g_emojiFonts.push_back({L"Segoe UI Emoji", false});
    g_emojiFonts.push_back({L"Segoe UI Symbol", false});
}

void DrawColorEmoji(HDC dc, const RECT& bounds, const std::wstring& glyph) {
    if (!g_d2dTarget || !g_emojiFormat || FAILED(g_d2dTarget->BindDC(dc, &bounds))) {
        const HFONT previous = static_cast<HFONT>(SelectObject(dc, g_emojiFont));
        DrawTextW(dc, glyph.c_str(), -1, const_cast<RECT*>(&bounds),
                  DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        SelectObject(dc, previous);
        return;
    }
    g_d2dTarget->BeginDraw();
    ID2D1SolidColorBrush* brush{};
    const D2D1_COLOR_F textColor{0.92f, 0.92f, 0.92f, 1.0f};
    if (SUCCEEDED(g_d2dTarget->CreateSolidColorBrush(textColor, &brush))) {
        // A DCRenderTarget's coordinate system starts at its current BindDC clip rectangle.
        const D2D1_RECT_F textBounds{0.0f, 0.0f,
                                     static_cast<float>(bounds.right - bounds.left),
                                     static_cast<float>(bounds.bottom - bounds.top)};
        const auto options = g_emojiFonts[g_emojiFontIndex].color
                                 ? D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
                                 : D2D1_DRAW_TEXT_OPTIONS_NONE;
        g_d2dTarget->DrawTextW(glyph.c_str(), static_cast<UINT32>(glyph.size()), g_emojiFormat,
                                textBounds, brush, options,
                                DWRITE_MEASURING_MODE_NATURAL);
        brush->Release();
    }
    g_d2dTarget->EndDraw();
}

void AddTrayIcon(HWND window) {
    g_tray = {};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = window;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_tray.uCallbackMessage = kTrayMessage;
    g_tray.hIcon = g_appIcon;
    lstrcpyW(g_tray.szTip, L"SwashMoji — Alt+E");
    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

void UpdateSortIndicator() {
    const wchar_t* cue = g_sortByUsage
                             ? L"Search — most used first (Alt+T)"
                             : L"Search — most recent first (Alt+T)";
    SendMessageW(g_edit, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(cue));
    lstrcpynW(g_tray.szTip,
              g_sortByUsage ? L"SwashMoji — most used — Alt+E" : L"SwashMoji — most recent — Alt+E",
              static_cast<int>(std::size(g_tray.szTip)));
    Shell_NotifyIconW(NIM_MODIFY, &g_tray);
}

void SetSortMode(bool sortByUsage) {
    g_sortByUsage = sortByUsage;
    SaveSettings();
    UpdateSortIndicator();
    RefreshList();
}

void ToggleSortMode() {
    SetSortMode(!g_sortByUsage);
}

void ConfirmAndClearUsageHistory() {
    const int result = MessageBoxW(
        g_window,
        L"Forget all recently used emoji and reset how often each emoji was chosen?\n\nThis cannot be undone.",
        L"Clear remembered emoji",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
    if (result != IDYES) return;

    g_history.clear();
    g_usageCounts.clear();
    SaveHistory();
    SaveUsage();
    RefreshList();
}

void DrawHelpText(HDC dc, const wchar_t* text, RECT area, HFONT font, COLORREF color,
                  UINT format = DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX) {
    const HFONT previous = static_cast<HFONT>(SelectObject(dc, font));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextW(dc, text, -1, &area, format);
    SelectObject(dc, previous);
}

void DrawHelpRow(HDC dc, int x, int y, int width, int height,
                 const wchar_t* key, const wchar_t* description) {
    constexpr int keyWidth = 94;
    RECT keyArea{x, y, x + keyWidth, y + 28};
    HBRUSH keyBrush = CreateSolidBrush(RGB(42, 42, 42));
    HPEN keyPen = CreatePen(PS_SOLID, 1, RGB(67, 67, 67));
    const HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(dc, keyBrush));
    const HPEN oldPen = static_cast<HPEN>(SelectObject(dc, keyPen));
    RoundRect(dc, keyArea.left, keyArea.top, keyArea.right, keyArea.bottom, 7, 7);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(keyPen);
    DeleteObject(keyBrush);

    DrawHelpText(dc, key, keyArea, g_helpHeadingFont, kText,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    RECT descriptionArea{x + keyWidth + 14, y + 3, x + width, y + height};
    DrawHelpText(dc, description, descriptionArea, g_helpBodyFont, kText);
}

LRESULT CALLBACK HelpWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        BOOL dark = TRUE;
        DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
        SetWindowTheme(window, L"DarkMode_Explorer", nullptr);
        SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIcon));
        SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIcon));
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        FillRect(dc, &client, g_backgroundBrush);

        DrawHelpText(dc, L"SwashMoji", RECT{28, 22, 350, 58}, g_helpTitleFont, kText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        DrawHelpText(dc, L"Keyboard-first emoji picker", RECT{29, 58, 350, 81},
                     g_helpBodyFont, kMutedText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        HBRUSH accentBrush = CreateSolidBrush(RGB(75, 151, 222));
        RECT accent{28, 91, client.right - 28, 93};
        FillRect(dc, &accent, accentBrush);
        DeleteObject(accentBrush);

        constexpr COLORREF heading = RGB(112, 184, 246);
        DrawHelpText(dc, L"ESSENTIALS", RECT{28, 112, 330, 136},
                     g_helpHeadingFont, heading);
        int leftY = 143;
        DrawHelpRow(dc, 28, leftY, 315, 42, L"Alt+E", L"Open from any app.");
        leftY += 44;
        DrawHelpRow(dc, 28, leftY, 315, 70, L"Type", L"Search names, Unicode keywords, and aliases. Word order does not matter; close spelling is a fallback.");
        leftY += 72;
        DrawHelpRow(dc, 28, leftY, 315, 46, L"Arrow keys", L"Move through matching emoji.");
        leftY += 48;
        DrawHelpRow(dc, 28, leftY, 315, 46, L"Click", L"Insert and return to SwashMoji.");
        leftY += 48;
        DrawHelpRow(dc, 28, leftY, 315, 46, L"Enter", L"Insert into the active text field.");
        leftY += 48;
        DrawHelpRow(dc, 28, leftY, 315, 54, L"Shift+Enter", L"Copy to the clipboard instead.");
        leftY += 56;
        DrawHelpRow(dc, 28, leftY, 315, 54, L"Ctrl+Enter", L"Insert and keep SwashMoji open.");
        leftY += 56;
        DrawHelpRow(dc, 28, leftY, 315, 42, L"Esc", L"Close the picker.");

        constexpr int rightX = 374;
        DrawHelpText(dc, L"CUSTOMIZE", RECT{rightX, 112, 690, 136},
                     g_helpHeadingFont, heading);
        int rightY = 143;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Tab", L"Cycle color and monochrome fonts.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+T", L"Toggle recent / most-used sorting.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+S", L"Hide or show the status line.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+I", L"Cycle skin tone for all compatible emoji.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 46, L"Alt+1 / 2 / 3", L"Show one, two, or three emoji rows.");
        rightY += 48;
        DrawHelpRow(dc, rightX, rightY, 318, 42, L"F1", L"Open this guide.");
        rightY += 57;

        DrawHelpText(dc, L"RECENTS & TRAY", RECT{rightX, rightY, 690, rightY + 24},
                     g_helpHeadingFont, heading);
        rightY += 31;
        DrawHelpText(dc,
                     L"SwashMoji remembers which emoji you choose. Recent mode puts your latest choices first; most-used mode puts your frequent choices first. This stays on your PC.",
                     RECT{rightX, rightY, 692, rightY + 90}, g_helpBodyFont, kText);
        rightY += 96;
        DrawHelpText(dc,
                     L"Left-click the tray icon to open. Right-click for sorting, text-field positioning, clearing remembered emoji, and Exit.",
                     RECT{rightX, rightY, 692, rightY + 90}, g_helpBodyFont, kText);

        DrawHelpText(dc, L"All usage data is stored locally in %LOCALAPPDATA%\\SwashMoji",
                     RECT{28, client.bottom - 37, client.right - 28, client.bottom - 17},
                     g_statusFont, kMutedText,
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        g_helpWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowHelp() {
    if (g_helpWindow) {
        ShowWindow(g_helpWindow, SW_SHOWNORMAL);
        SetForegroundWindow(g_helpWindow);
        return;
    }

    RECT windowRect{0, 0, kHelpWidth, kHelpHeight};
    AdjustWindowRectEx(&windowRect, WS_CAPTION | WS_SYSMENU, FALSE, WS_EX_TOOLWINDOW);
    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    HMONITOR monitor = MonitorFromWindow(g_window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(monitor, &monitorInfo);
    const RECT& area = monitorInfo.rcWork;
    const int x = area.left + ((area.right - area.left) - width) / 2;
    const int y = area.top + ((area.bottom - area.top) - height) / 2;

    g_helpWindow = CreateWindowExW(
        WS_EX_TOOLWINDOW, kHelpClassName, L"SwashMoji Help", WS_CAPTION | WS_SYSMENU,
        x, y, width, height, g_window, nullptr,
        reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(g_window, GWLP_HINSTANCE)), nullptr);
    if (!g_helpWindow) return;
    ShowWindow(g_helpWindow, SW_SHOW);
    UpdateWindow(g_helpWindow);
    SetForegroundWindow(g_helpWindow);
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    const UINT positionFlags = MF_STRING | (g_positionAboveTextField ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(menu, positionFlags, kPositionAboveTextFieldId, L"Try positioning above active text field");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kSortRecentId, L"Sort: Most recent");
    AppendMenuW(menu, MF_STRING, kSortMostUsedId, L"Sort: Most used");
    CheckMenuRadioItem(menu, kSortRecentId, kSortMostUsedId,
                       g_sortByUsage ? kSortMostUsedId : kSortRecentId, MF_BYCOMMAND);
    AppendMenuW(menu, MF_STRING, kClearUsageHistoryId, L"Clear remembered emoji...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kExitId, L"Exit");
    POINT point{};
    GetCursorPos(&point);
    SetForegroundWindow(g_window);
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                        point.x, point.y, 0, g_window, nullptr);
    DestroyMenu(menu);
    // Lets the taskbar dismiss the menu correctly after a tray interaction.
    PostMessageW(g_window, WM_NULL, 0, 0);
    if (command == kPositionAboveTextFieldId) {
        g_positionAboveTextField = !g_positionAboveTextField;
        SaveSettings();
    } else if (command == kSortRecentId) {
        SetSortMode(false);
    } else if (command == kSortMostUsedId) {
        SetSortMode(true);
    } else if (command == kClearUsageHistoryId) {
        ConfirmAndClearUsageHistory();
    } else if (command == kExitId) {
        DestroyWindow(g_window);
    }
}

void LayoutChildren(HWND window) {
    RECT area{};
    GetClientRect(window, &area);
    constexpr int margin = 12;
    MoveWindow(g_edit, margin, margin, area.right - margin * 2, kInputHeight, TRUE);
    const int listY = margin + kInputHeight + 8;
    const int listHeight = g_emojiRows * kResultSize;
    MoveWindow(g_list, margin, listY, area.right - margin * 2, listHeight, TRUE);
    MoveWindow(g_status, margin, listY + listHeight + 5, area.right - margin * 2,
               kStatusHeight, TRUE);
}

void MoveSelection(int direction) {
    if (g_displayVisible.empty()) return;
    int selected = static_cast<int>(SendMessageW(g_list, LB_GETCURSEL, 0, 0));
    if (selected == LB_ERR) selected = 0;
    selected = std::clamp(selected + direction, 0, static_cast<int>(g_displayVisible.size()) - 1);
    SendMessageW(g_list, LB_SETCURSEL, selected, 0);
}

LRESULT CALLBACK InputProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam) {
    const WNDPROC original = control == g_edit ? g_editProc : g_listProc;
    if (control == g_list && message == WM_LBUTTONUP) {
        const LRESULT result = CallWindowProcW(original, control, message, wParam, lParam);
        InsertSelection(true);
        return result;
    }
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        if (wParam == 'I' && (GetKeyState(VK_MENU) & 0x8000)) {
            CycleSkinTone();
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            ShowWindow(g_window, SW_HIDE);
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (GetKeyState(VK_SHIFT) & 0x8000) CopySelection();
            else if (GetKeyState(VK_CONTROL) & 0x8000) InsertSelection(true, true);
            else InsertSelection();
            return 0;
        }
        if (wParam == VK_UP) {
            MoveSelection(-1);
            return 0;
        }
        if (wParam == VK_DOWN) {
            MoveSelection(1);
            return 0;
        }
        if (wParam == VK_LEFT) {
            MoveSelection(-g_emojiRows);
            return 0;
        }
        if (wParam == VK_RIGHT) {
            MoveSelection(g_emojiRows);
            return 0;
        }
    }
    return CallWindowProcW(original, control, message, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        g_edit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(kEditId)), nullptr, nullptr);
        g_list = CreateWindowExW(0, L"LISTBOX", L"",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT |
                                 LBS_OWNERDRAWFIXED | LBS_MULTICOLUMN | LBS_HASSTRINGS,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(kListId)), nullptr, nullptr);
        g_status = CreateWindowExW(0, L"STATIC", L"",
                                   WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
                                   0, 0, 0, 0, window,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatusId)), nullptr, nullptr);
        SendMessageW(g_edit, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        SendMessageW(g_edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(8, 8));
        SendMessageW(g_list, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
        SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_statusFont), TRUE);
        SendMessageW(g_list, LB_SETCOLUMNWIDTH, kResultSize, 0);
        SetWindowTheme(g_edit, L"DarkMode_Explorer", nullptr);
        SetWindowTheme(g_list, L"DarkMode_Explorer", nullptr);
        g_editProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_edit, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        g_listProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(g_list, GWLP_WNDPROC,
                                                                  reinterpret_cast<LONG_PTR>(InputProc)));
        LayoutChildren(window);
        RefreshList();
        AddTrayIcon(window);
        UpdateSortIndicator();
        UpdateStatusLine();
        return 0;
    }
    case WM_SIZE: LayoutChildren(window); return 0;
    case WM_MEASUREITEM:
        if (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlID == kListId) {
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = kResultSize;
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemWidth = kResultSize;
            return TRUE;
        }
        break;
    case WM_DRAWITEM: {
        auto* item = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (item->CtlID != kListId || item->itemID >= g_displayVisible.size()) break;
        const bool selected = (item->itemState & ODS_SELECTED) != 0;
        HBRUSH fill = CreateSolidBrush(selected ? kSelected : kBackground);
        FillRect(item->hDC, &item->rcItem, fill);
        DeleteObject(fill);
        SetBkMode(item->hDC, TRANSPARENT);
        SetTextColor(item->hDC, kText);
        const HFONT previousFont = static_cast<HFONT>(SelectObject(item->hDC, g_uiFont));
        const Emoji& emoji = *g_displayVisible[item->itemID];
        RECT glyph = item->rcItem;
        InflateRect(&glyph, -3, -3);
        DrawColorEmoji(item->hDC, glyph, emoji.glyph);
        if (item->itemState & ODS_FOCUS) DrawFocusRect(item->hDC, &item->rcItem);
        SelectObject(item->hDC, previousFont);
        return TRUE;
    }
    case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wParam), kText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kInputBackground);
        return reinterpret_cast<LRESULT>(g_inputBrush);
    case WM_CTLCOLORLISTBOX:
        SetTextColor(reinterpret_cast<HDC>(wParam), kText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kBackground);
        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wParam), kMutedText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kBackground);
        return reinterpret_cast<LRESULT>(g_backgroundBrush);
    case WM_COMMAND:
        if (LOWORD(wParam) == kEditId && HIWORD(wParam) == EN_CHANGE) RefreshList();
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam == 'I' && (GetKeyState(VK_MENU) & 0x8000)) {
            CycleSkinTone();
            return 0;
        }
        if (wParam == VK_ESCAPE) { ShowWindow(window, SW_HIDE); return 0; }
        if (wParam == VK_RETURN) {
            if (GetKeyState(VK_SHIFT) & 0x8000) CopySelection();
            else if (GetKeyState(VK_CONTROL) & 0x8000) InsertSelection(true, true);
            else InsertSelection();
            return 0;
        }
        break;
    case WM_HOTKEY:
        if (wParam == kHotkeyId) {
            SetWindowTextW(g_edit, L"");
            CenterOnActiveMonitor();
        }
        return 0;
    case WM_TIMER:
        if (wParam == kStatusTimerId) {
            KillTimer(window, kStatusTimerId);
            UpdateStatusLine();
            return 0;
        }
        if (wParam == kRestorePickerFocusTimerId) {
            KillTimer(window, kRestorePickerFocusTimerId);
            SetForegroundWindow(g_window);
            SetFocus(g_edit);
            return 0;
        }
        break;
    case kShowPickerMessage:
        SetWindowTextW(g_edit, L"");
        CenterOnActiveMonitor();
        return 0;
    case kTrayMessage:
        switch (LOWORD(lParam)) {
        case WM_LBUTTONUP:
        case NIN_SELECT:
        case NIN_KEYSELECT:
            CenterOnActiveMonitor();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu();
            break;
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_CLOSE) { ShowWindow(window, SW_HIDE); return 0; }
        break;
    case WM_DESTROY:
        UnregisterHotKey(window, kHotkeyId);
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    // A repeat launch should surface the existing picker, not create a second
    // process that cannot own the global Alt+E shortcut.
    if (HWND existing = FindWindowW(kClassName, nullptr)) {
        PostMessageW(existing, kShowPickerMessage, 0, 0);
        return 0;
    }
    if (!LoadEmojis()) {
        MessageBoxW(nullptr, L"Kunne ikke lese emojis.txt ved siden av SwashMoji.exe.", L"SwashMoji", MB_ICONERROR);
        return 1;
    }
    LoadHistory();
    LoadUsage();
    LoadSettings();
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    g_comInitialized = SUCCEEDED(comResult);
    CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(&g_uiAutomation));
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    wc.lpfnWndProc = WindowProc;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    g_appIcon = LoadIconW(instance, MAKEINTRESOURCEW(kAppIconId));
    wc.hIcon = g_appIcon;
    g_uiFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                           L"Segoe UI Variable Text");
    g_statusFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                               L"Segoe UI Variable Text");
    g_helpTitleFont = CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                  L"Segoe UI Variable Display");
    g_helpHeadingFont = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                    L"Segoe UI Variable Text");
    g_helpBodyFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                 L"Segoe UI Variable Text");
    g_backgroundBrush = CreateSolidBrush(kBackground);
    g_inputBrush = CreateSolidBrush(kInputBackground);
    InitializeColorEmojiDrawing();
    LoadInstalledColorEmojiFonts();
    // Only the glyph column uses this font; labels retain the regular UI font.
    SelectEmojiFont(0);
    wc.hbrBackground = g_backgroundBrush;
    if (!RegisterClassW(&wc)) return 1;
    WNDCLASSW helpClass{};
    helpClass.hInstance = instance;
    helpClass.lpszClassName = kHelpClassName;
    helpClass.lpfnWndProc = HelpWindowProc;
    helpClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    helpClass.hIcon = g_appIcon;
    helpClass.hbrBackground = g_backgroundBrush;
    if (!RegisterClassW(&helpClass)) return 1;

    g_window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, kClassName, L"SwashMoji",
                               WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                               kPickerWidth, PickerHeight(), nullptr, nullptr, instance, nullptr);
    if (!g_window) return 1;
    SendMessageW(g_window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_appIcon));
    SendMessageW(g_window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_appIcon));
    if (!RegisterHotKey(g_window, kHotkeyId, MOD_ALT | MOD_NOREPEAT, 'E')) {
        MessageBoxW(nullptr, L"Alt+E er allerede i bruk av et annet program.", L"SwashMoji", MB_ICONWARNING);
    }

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0)) {
        // Handle app-local Alt shortcuts before dispatch. Depending on focus and
        // popup state, Windows can address system-key messages to either the
        // child control or the top-level picker.
        const bool altPressed = (GetKeyState(VK_MENU) & 0x8000) ||
                                (message.message == WM_SYSKEYDOWN &&
                                 (message.lParam & (1u << 29)));
        const bool firstKeyPress = !(message.lParam & (1u << 30));
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            firstKeyPress && message.wParam == VK_F1) {
            ShowHelp();
            continue;
        }
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            firstKeyPress && message.wParam == VK_TAB) {
            CycleEmojiFont();
            continue;
        }
        if ((message.message == WM_KEYDOWN || message.message == WM_SYSKEYDOWN) &&
            altPressed && firstKeyPress) {
            if (message.wParam == 'T') {
                ToggleSortMode();
                continue;
            }
            if (message.wParam == 'S') {
                ToggleStatusLine();
                continue;
            }
            if (message.wParam == 'I') {
                CycleSkinTone();
                continue;
            }
            if (message.wParam >= '1' && message.wParam <= '3') {
                SetEmojiRows(static_cast<int>(message.wParam - '0'));
                continue;
            }
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    if (g_uiAutomation) g_uiAutomation->Release();
    if (g_emojiFormat) g_emojiFormat->Release();
    if (g_dwriteFactory) g_dwriteFactory->Release();
    if (g_d2dTarget) g_d2dTarget->Release();
    if (g_d2dFactory) g_d2dFactory->Release();
    DeleteObject(g_emojiFont);
    DeleteObject(g_helpBodyFont);
    DeleteObject(g_helpHeadingFont);
    DeleteObject(g_helpTitleFont);
    DeleteObject(g_statusFont);
    DeleteObject(g_uiFont);
    DeleteObject(g_inputBrush);
    DeleteObject(g_backgroundBrush);
    if (g_comInitialized) CoUninitialize();
    return static_cast<int>(message.wParam);
}
