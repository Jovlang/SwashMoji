#pragma once
#include <windows.h>
#include "search.h"
#include <functional>

namespace SwashMoji {
void ShowCombinationDetails(HWND owner, HINSTANCE instance, const Catalog& catalog, const Combination& combination, const DisplayLanguages& languages = {});
// Edits are applied only on Save/Delete; Close discards an unfinished draft.
// The owner keeps its insertion target and picker session throughout the modal loop.
bool ShowVocabulary(HWND owner, HINSTANCE instance, const Catalog& catalog, Profile& profile,
                    const std::wstring& phrase, const ResultId& target,
                    const std::function<bool()>& persist);
}
