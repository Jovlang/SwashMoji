#include "picker.h"
#include "test_support.h"
#include <algorithm>
#include <fstream>
#include <numeric>

using namespace SwashMoji;
int main(int argc, char** argv) {
    try {
        CHECK(GridRows(0, 3) == 1);
        CHECK(GridRows(5, 3) == 1);
        CHECK(GridRows(10, 3) == 1);
        CHECK(GridRows(11, 3) == 2);
        CHECK(GridRows(20, 3) == 2);
        CHECK(GridRows(21, 3) == 3);
        CHECK(GridRows(100, 2) == 2);
        CHECK(GridRows(100, 1) == 1);
        for (int rows = 1; rows <= 3; ++rows) {
            for (size_t count = 0; count <= 125; ++count) {
                auto order = GridOrder(count, rows);
                auto sorted = order;
                std::sort(sorted.begin(), sorted.end());
                CHECK(sorted.size() == count);
                for (size_t i = 0; i < count; ++i) CHECK(sorted[i] == i);
                for (size_t slot = 0; slot < count; ++slot) {
                    for (int direction : {-1, 1}) {
                        const auto horizontal = GridMove(slot, count, rows, direction, 0);
                        CHECK(horizontal < count);
                        CHECK(horizontal / rows == static_cast<size_t>(std::clamp(
                            static_cast<int>(slot / rows) + direction, 0, static_cast<int>((count - 1) / rows))));
                        const auto vertical = GridMove(slot, count, rows, 0, direction);
                        CHECK(vertical < count && vertical / rows == slot / rows);
                    }
                }
            }
        }
        CHECK(GridOrder(4, 2) == (std::vector<size_t>{0, 2, 1, 3}));
        CHECK(GridOrder(5, 3) == (std::vector<size_t>{0, 2, 4, 1, 3}));
        CHECK(GridMove(29, 31, 3, 1, 0) == 30); // Incomplete next page.
        CHECK(GridMove(30, 31, 3, -1, 0) == 27);
        CHECK(GridMove(0, 0, 3, 1, 0) == 0);
        CHECK(argc == 2);
        std::ifstream input(argv[1], std::ios::binary);
        Catalog catalog; CHECK(catalog.Load(input));
        const auto variants = CatalogVariants(catalog, {ResultKind::Emoji, L"🤝"});
        CHECK(variants.size() > 6);
        bool mixed = false;
        for (const auto* variant : variants) {
            CHECK(catalog.Find(variant->glyph) == variant);
            CHECK(variant->family.value == L"🤝");
            mixed |= SkinToneIndex(variant->glyph) && !UsesOnlySkinTone(variant->glyph, SkinToneIndex(variant->glyph));
        }
        CHECK(mixed);
        CHECK(CatalogVariants(catalog, {ResultKind::Emoji, L"🚀"}).size() == 1);
        CHECK(CatalogVariants(catalog, {ResultKind::Combination, L"missing"}).empty());
        std::cout << "PASS: grid permutations, spatial edges, partial pages and catalog-only mixed variants\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
