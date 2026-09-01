#include "ranking.h"

#include <algorithm>
#include <vector>

struct Item {
    int id;
    unsigned int usage;
    int recency;
};

std::vector<int> SortedIds(std::vector<Item> items, bool sortByUsage) {
    std::stable_sort(items.begin(), items.end(), [sortByUsage](const Item& left, const Item& right) {
        return SwashMojiRanking::ComparePreference(sortByUsage,
                                                   left.usage, left.recency,
                                                   right.usage, right.recency) > 0;
    });
    std::vector<int> ids;
    for (const Item& item : items) ids.push_back(item.id);
    return ids;
}

int main() {
    const std::vector<Item> items{
        {1, 2, 30},
        {2, 5, 10},
        {3, 2, 20},
    };

    if (SortedIds(items, false) != std::vector<int>{1, 3, 2}) return 1;
    if (SortedIds(items, true) != std::vector<int>{2, 1, 3}) return 2;
    if (SwashMojiRanking::ComparePreference(true, 2, 20, 2, 20) != 0) return 3;
    return 0;
}
