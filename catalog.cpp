#include "catalog.h"
#include "text.h"
#include <algorithm>
#include <limits>

namespace SwashMoji {
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

bool Catalog::Load(std::istream& input) {
    entries_.clear();
    glyphIndex_.clear();
    variants_.clear();
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (entries_.empty() && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        const size_t first = line.find('\t');
        const size_t separator = first == std::string::npos ? line.find(' ') : first;
        if (separator == std::string::npos) continue;
        const size_t second = first == std::string::npos ? std::string::npos : line.find('\t', first + 1);
        const size_t third = second == std::string::npos ? std::string::npos : line.find('\t', second + 1);
        const size_t fourth = third == std::string::npos ? std::string::npos : line.find('\t', third + 1);
        Emoji emoji;
        emoji.glyph = Utf8ToWide(line.substr(0, separator));
        emoji.name = Utf8ToWide(line.substr(separator + 1, second - separator - 1));
        if (second != std::string::npos) emoji.keywords = Utf8ToWide(line.substr(second + 1, third - second - 1));
        if (third != std::string::npos) emoji.nbName = Utf8ToWide(line.substr(third + 1, fourth - third - 1));
        if (fourth != std::string::npos) emoji.nbKeywords = Utf8ToWide(line.substr(fourth + 1));
        if (emoji.glyph.empty() || emoji.name.empty() || glyphIndex_.count(emoji.glyph)) continue;
        if (emoji.keywords.empty()) emoji.keywords = emoji.name;
        emoji.lowerName = NormalizePhrase(emoji.name);
        emoji.lowerKeywords = NormalizePhrase(emoji.keywords);
        emoji.lowerNbName = NormalizePhrase(emoji.nbName);
        emoji.lowerNbKeywords = NormalizePhrase(emoji.nbKeywords);
        emoji.nbNameWords = SplitWords(emoji.lowerNbName);
        emoji.nbKeywordWords = SplitWords(emoji.lowerNbKeywords);
        emoji.nameWords = SplitWords(emoji.lowerName);
        emoji.keywordWords = SplitWords(emoji.lowerKeywords);
        for (const auto& word : emoji.nameWords) emoji.normalizedNameWords.push_back(NormalizeSearchWord(word));
        for (const auto& word : emoji.keywordWords) emoji.normalizedKeywordWords.push_back(NormalizeSearchWord(word));
        glyphIndex_.emplace(emoji.glyph, entries_.size());
        entries_.push_back(std::move(emoji));
    }

    // Only associate a stripped sequence with a base that actually exists in the
    // catalog. In particular, a standalone skin-tone modifier has no empty base.
    std::map<std::wstring, std::wstring> bases;
    for (const auto& emoji : entries_) {
        if (!SkinToneIndex(emoji.glyph)) bases.emplace(SkinToneFamilyKey(emoji.glyph), emoji.glyph);
    }
    // These multi-person sequences have established single-glyph base forms.
    const std::map<std::wstring, std::wstring> alternateBases{
        {L"👩‍🤝‍👨", L"👫"}, {L"🧑‍❤‍💋‍🧑", L"💏"},
        {L"🧑‍❤‍🧑", L"💑"}, {L"🫱‍🫲", L"🤝"}
    };
    constexpr size_t missing = std::numeric_limits<size_t>::max();
    for (size_t index = 0; index < entries_.size(); ++index) {
        auto& emoji = entries_[index];
        std::wstring key = SkinToneFamilyKey(emoji.glyph);
        const auto alternate = alternateBases.find(key);
        if (!bases.count(key) && alternate != alternateBases.end() && Find(alternate->second)) {
            key = SkinToneFamilyKey(alternate->second);
        }
        emoji.family.value = bases.count(key) ? key : emoji.glyph;
        auto inserted = variants_.try_emplace(emoji.family.value);
        if (inserted.second) inserted.first->second.fill(missing);
        const int tone = SkinToneIndex(emoji.glyph);
        if (!tone || UsesOnlySkinTone(emoji.glyph, tone)) {
            auto& slot = inserted.first->second[tone];
            if (slot == missing) slot = index;
        }
    }
    return !input.bad() && !entries_.empty();
}

const Emoji* Catalog::Find(const std::wstring& glyph) const {
    const auto found = glyphIndex_.find(glyph);
    return found == glyphIndex_.end() ? nullptr : &entries_[found->second];
}

const Emoji* Catalog::FindFamily(const EmojiFamilyId& family) const {
    const auto found = variants_.find(family.value);
    if (found == variants_.end() || found->second[0] == std::numeric_limits<size_t>::max()) return nullptr;
    return &entries_[found->second[0]];
}

bool Catalog::LoadIntents(std::istream& input) {
    if (!input) return false;
    std::vector<std::vector<std::wstring>> pending(entries_.size());
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) return false;
        const auto phrase = NormalizePhrase(Utf8ToWide(line.substr(0, tab)));
        const auto* emoji = Find(Utf8ToWide(line.substr(tab + 1)));
        if (phrase.empty() || !emoji) return false;
        const auto* base = FindFamily(emoji->family);
        if (!base) return false;
        auto& intents = pending[static_cast<size_t>(base - entries_.data())];
        if (std::find(intents.begin(), intents.end(), phrase) == intents.end()) intents.push_back(phrase);
    }
    if (input.bad()) return false;
    for (size_t i = 0; i < entries_.size(); ++i) entries_[i].intents = std::move(pending[i]);
    return true;
}

const Emoji* Catalog::PreferredVariant(const Emoji& emoji, int tone) const {
    if (tone < 1 || tone > 5) return &emoji;
    const auto found = variants_.find(emoji.family.value);
    if (found == variants_.end() || found->second[tone] == std::numeric_limits<size_t>::max()) return &emoji;
    return &entries_[found->second[tone]];
}

}
