#include "picker.h"
#include <algorithm>
#include <numeric>

namespace SwashMoji {
PickerPlacement PlacePickerNearAnchor(int anchorLeft, int anchorTop, int anchorRight, int anchorBottom,
                                      int workLeft, int workTop, int workRight, int workBottom,
                                      int width, int height, int gap) {
    const int minX = workLeft;
    const int minY = workTop;
    const int maxX = std::max(minX, workRight - width);
    const int maxY = std::max(minY, workBottom - height);
    PickerPlacement result;
    result.x = std::clamp(anchorLeft + (anchorRight - anchorLeft) / 2 - width / 2, minX, maxX);
    result.aboveAnchor = anchorTop - gap - height >= workTop;
    result.y = result.aboveAnchor ? anchorTop - gap - height : anchorBottom + gap;
    result.y = std::clamp(result.y, minY, maxY);
    return result;
}

int GridRows(size_t count, int preferredRows, int columns) {
    columns = std::max(1, columns);
    preferredRows = std::clamp(preferredRows, 1, 3);
    if (!count) return 1;
    return static_cast<int>(std::min(static_cast<size_t>(preferredRows), 1 + (count - 1) / columns));
}

std::vector<size_t> GridOrder(size_t count, int rows, int columns) {
    rows = std::max(1, rows);
    columns = std::max(1, columns);
    std::vector<size_t> result(count);
    const size_t pageSize = static_cast<size_t>(rows) * columns;
    for (size_t start = 0; start < count; start += pageSize) {
        const auto length = std::min(pageSize, count - start);
        size_t rank = start;
        for (int row = 0; row < rows; ++row)
            for (size_t slot = row; slot < length; slot += rows)
                result[start + slot] = rank++;
    }
    return result;
}

size_t GridMove(size_t slot, size_t count, int rows, int dx, int dy) {
    if (!count) return 0;
    rows = std::max(1, rows);
    slot = std::min(slot, count - 1);
    const auto column = static_cast<int>(slot / rows);
    const auto nextColumn = std::clamp(column + dx, 0, static_cast<int>((count - 1) / rows));
    const auto lastRow = static_cast<int>(std::min(static_cast<size_t>(rows), count - nextColumn * rows)) - 1;
    const auto row = std::clamp(static_cast<int>(slot % rows) + dy, 0, lastRow);
    return static_cast<size_t>(nextColumn * rows + row);
}

std::vector<const Emoji*> CatalogVariants(const Catalog& catalog, const ResultId& id) {
    std::vector<const Emoji*> variants;
    if (id.kind != ResultKind::Emoji) return variants;
    for (const auto& emoji : catalog.Entries())
        if (emoji.family.value == id.value) variants.push_back(&emoji);
    return variants;
}
}
