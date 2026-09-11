#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace SwashMoji {

class DisplayLanguages {
public:
    const std::vector<std::string>& Locales() const { return locales_; }
    // Reject unsupported, duplicate or more than two locales without changing state.
    bool Set(const std::vector<std::string>& locales);
private:
    std::vector<std::string> locales_{"en", "nb"};
};

struct EmojiFamilyId {
    std::wstring value;
    bool operator==(const EmojiFamilyId& other) const { return value == other.value; }
};

enum class ResultKind { Emoji, Combination };

struct ResultId {
    ResultKind kind{ResultKind::Emoji};
    std::wstring value;
    bool operator==(const ResultId& other) const { return kind == other.kind && value == other.value; }
    bool operator<(const ResultId& other) const {
        return std::tie(kind, value) < std::tie(other.kind, other.value);
    }
};

struct MatchScore { int tier{}; int detail{}; bool preferredLanguage{}; };

struct SearchResult {
    ResultId id;
    std::wstring label;
    std::wstring payload;
    MatchScore match;
    std::wstring explanation;
};

// HWND is kept in the platform adapter; this token never goes into the profile.
struct PickerSession {
    std::uintptr_t originalTarget{};
    std::wstring query;
    ResultId selected;
    std::vector<ResultId> rankingSnapshot;
    std::uint64_t insertionAttempt{};
};

} // namespace SwashMoji
