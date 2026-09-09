#pragma once
#include "models.h"
#include <array>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace SwashMoji {
struct LocalizedEmojiName {
    std::wstring name, keywords, lowerName, lowerKeywords;
    std::vector<std::wstring> nameWords, keywordWords;
    std::vector<std::wstring> normalizedNameWords, normalizedKeywordWords;
};

struct Emoji {
    std::wstring glyph;
    EmojiFamilyId family;
    std::map<std::string, LocalizedEmojiName> names;
    std::vector<std::wstring> intents;
};

struct LocaleMetadata {
    const char* code;
    const wchar_t* label;
    bool englishInflections;
};
inline constexpr wchar_t kEmojiNameSeparator[] = L" · ";
const std::vector<LocaleMetadata>& SupportedLocales();
void SetEmojiLocalization(Emoji& emoji, const std::string& locale,
                          const std::wstring& name, const std::wstring& keywords = L"");
std::wstring GetEmojiName(const Emoji& emoji, const std::string& locale);
std::wstring GetBestEmojiName(const Emoji& emoji, const std::vector<std::string>& preferredLocales = {"en"});
// Two slots by construction; empty secondary means a single display language.
// Duplicate locales and duplicate translated text are rendered only once.
std::wstring FormatEmojiDisplayName(const Emoji& emoji, const std::string& primary = "en",
                                    const std::string& secondary = "nb");

int SkinToneIndex(const std::wstring& glyph);
bool UsesOnlySkinTone(const std::wstring& glyph, int tone);
std::wstring SkinToneFamilyKey(const std::wstring& glyph);

class Catalog {
public:
    bool Load(std::istream& input);
    bool LoadIntents(std::istream& input);
    const std::vector<Emoji>& Entries() const { return entries_; }
    const Emoji* Find(const std::wstring& glyph) const;
    const Emoji* FindFamily(const EmojiFamilyId& family) const;
    const Emoji* PreferredVariant(const Emoji& emoji, int tone) const;
private:
    std::vector<Emoji> entries_;
    std::map<std::wstring, size_t> glyphIndex_;
    std::map<std::wstring, std::array<size_t, 6>> variants_;
};
}
