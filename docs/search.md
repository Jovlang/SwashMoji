# Bilingual search and personal vocabulary (M2)

Search English and Norwegian Bokmål together, without a language switch or runtime
network access. Exact personal aliases rank first, then exact English/localized
names or a pasted known emoji, exact curated intent phrases, name prefixes,
all-token lexical matches, and finally fuzzy matches only when ordinary results
are absent. Partial aliases use the corresponding prefix/token classes. Existing
query learning and recency/usage break ties within a class; see [learning.md](learning.md).

Text uses Windows NFKC normalization and invariant Unicode lowercase conversion.
Letters and digits form words; punctuation and whitespace become word boundaries.
All words are retained. Æ, ø, å and accents are preserved; composed/decomposed
forms compare equally. English inflection rules apply only to English fields.
Known pasted emoji are recognized before word splitting and preserve the exact
variant. Alias and phrase matches resolve a stable family and use the global tone.

Right-click a result and choose **Add alias**, or press **Alt+A**, to prefill the
current query and target. With no results, **Teach this phrase** opens the same
editor and asks for an emoji. **My vocabulary** in the tray opens the saved list.
Select a saved phrase to edit it, or **New** to start a draft. **Save alias** applies
the draft, **Delete** removes the selected saved alias, and **Close**/Escape discards
an unfinished draft. Replacing an existing normalized phrase requires confirmation.
Changing the saved-list selection or choosing New also discards the current draft.

Aliases are local profile data and survive catalog regeneration, app restarts, and
clearing history. There are at most 500 aliases with phrases of at most 96 UTF-16
code units. Persistence failures are shown inline and in the picker/tray; changes
remain available in the running session. See [profile-format.md](profile-format.md).
The modal editor preserves the original insertion target, query and selected ID;
opening shortcuts and tray callbacks cannot reset the picker while it is open.

## Catalog provenance and regeneration

`emojis.txt` has five tab-separated UTF-8 columns: glyph, English name, English
keywords, Bokmål name, Bokmål keywords. The loader still accepts older three-column
catalogs. `intent_phrases.tsv` holds 51 separately maintained intent mappings (50 phrases).
Both files and `UNICODE_LICENSE.txt` must be distributed beside the executable;
CMake copies them.

Sources are Unicode CLDR 48.2, tag `release-48-2`, under
[Unicode License v3](../third_party/UNICODE_LICENSE.txt). The checked-in
`data/catalog_sources.json` records source paths and SHA-256 hashes. CLDR's
supplemental parent-locale data resolves `nb` to `no`; the pinned release has no
separate `nb` annotation files. Names/keywords inherit from `no` with English
fallback. Hand-authored annotations override derived annotations. Inheritance
markers are respected. Existing English keyword vocabulary is preserved.

```powershell
python tools/update_emoji_catalog.py --download --cldr-dir build/cldr
python tools/update_emoji_catalog.py --cldr-dir build/cldr
python tests/test_catalog_generator.py
.\build.cmd test build-m2
```

Generator tests need Python 3.10 or later. If Python is not on PATH, set
`$env:SWASHMOJI_PYTHON` to its executable path before running `build.cmd test` to
include those tests in CTest.

Use a fresh cache directory when changing the pinned version. Download mode accepts
404 only for optional inherited locale files and records those absences; missing
files in offline mode are errors. Generating twice from the same inputs gives the
same catalog and manifest. Runtime startup reads only the bundled generated files.

## Verification

The checked-in `tests/search_corpus.tsv` has 111 cases: 40 names/keywords, 51 intent
phrases, seven normalization cases, four typos, three glyphs, three aliases and
three intentional misses. All 111 meet the expected top-three/no-result outcome;
the intent subset is 51/51. Tests also enforce the exact-name tier for every
untoned English and Bokmål catalog name, explicit alias priority, duplicate/rename
transactions, limits, codec restart, and version 1 migration with backup retention.
Python fixture tests cover locale inheritance, English fallback, offline missing
sources, curated data references and deterministic regeneration without a network.

`SwashMojiPickerVocabularyTests.exe` separately exercises the real native editor
and picker against an isolated profile under the build directory. It checks
create/edit/delete across disk reload, cancelled drafts/replacements, preserved
query/selection/target, and actual insertion into an external Win32 edit without
changing the clipboard. Run on an interactive desktop with modifiers released.
Results are written to `picker-vocabulary-result.txt`. The separate
`SwashMojiVocabularyPreview.exe` opens an isolated editor for visual/keyboard checks
and uses `vocabulary-preview-profile` beside the executable.
This is separate from CTest because it changes desktop focus and submits real input.

M2 verification on 2026-09-07: all six CTest suites passed, including both offline Python
fixture tests; full-cache regeneration was byte-identical. The native picker/editor
test passed on the interactive desktop, including variation-selector prefilling.
Visual inspection and keyboard Save/Escape passed in the standalone preview, and
UI Automation exposed named phrase/search/result controls. Broader target-app,
multi-monitor/DPI and screen-reader acceptance remains assigned to M4/M6.
For the current eight-suite baseline and remaining M3 visual review, see
[learning verification](learning.md#verification).
