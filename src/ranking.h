#pragma once

namespace SwashMojiRanking {

// Returns a positive value when the left item should sort first, a negative
// value when the right item should sort first, and zero for an exact tie.
inline int ComparePreference(bool sortByUsage,
                             unsigned int leftUsage, int leftRecency,
                             unsigned int rightUsage, int rightRecency) {
    if (sortByUsage) {
        if (leftUsage != rightUsage) return leftUsage > rightUsage ? 1 : -1;
        if (leftRecency != rightRecency) return leftRecency > rightRecency ? 1 : -1;
    } else {
        if (leftRecency != rightRecency) return leftRecency > rightRecency ? 1 : -1;
        if (leftUsage != rightUsage) return leftUsage > rightUsage ? 1 : -1;
    }
    return 0;
}

} // namespace SwashMojiRanking
