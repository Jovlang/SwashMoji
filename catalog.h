#pragma once
#include "models.h"
#include <array>
#include <istream>
#include <map>
#include <string>
#include <vector>

namespace SwashMoji {
struct Emoji {
    std::wstring glyph, name, keywords, lowerName, lowerKeywords;
    std::vector<std::wstring> nameWords, keywordWords;
    std::vector<std::wstring> normalizedNameWords, normalizedKeywordWords;
    EmojiFamilyId family;
};

int SkinToneIndex(const std::wstring& glyph);
bool UsesOnlySkinTone(const std::wstring& glyph, int tone);
std::wstring SkinToneFamilyKey(const std::wstring& glyph);

class Catalog {
public:
    bool Load(std::istream& input);
    const std::vector<Emoji>& Entries() const { return entries_; }
    const Emoji* Find(const std::wstring& glyph) const;
    const Emoji* PreferredVariant(const Emoji& emoji, int tone) const;
private:
    std::vector<Emoji> entries_;
    std::map<std::wstring, size_t> glyphIndex_;
    std::map<std::wstring, std::array<size_t, 6>> variants_;
};
}
