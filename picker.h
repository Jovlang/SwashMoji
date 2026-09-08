#pragma once
#include "catalog.h"

namespace SwashMoji {
int GridRows(size_t count, int preferredRows, int columns = 10);
// Maps native column-major slots to ranked results. Final pages contain no
// placeholder items, so accessibility and pointer hit testing share real slots.
std::vector<size_t> GridOrder(size_t count, int rows, int columns = 10);
size_t GridMove(size_t slot, size_t count, int rows, int dx, int dy);
std::vector<const Emoji*> CatalogVariants(const Catalog& catalog, const ResultId& id);
}
