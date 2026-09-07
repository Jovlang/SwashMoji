#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace SwashMoji {

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

struct MatchScore { int tier{}; int detail{}; };

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
