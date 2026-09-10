# Multilingual search and personal vocabulary

## Locale model and display preferences

The catalog now stores names, keywords and their normalized search caches by
locale code in `Emoji::names`. `SetEmojiLocalization` builds/replaces one locale's
data; `SupportedLocales` registers display metadata and English inflection policy.
The existing three/five-column files are still accepted and map to `en`/`nb`.
The bundled dataset also includes German (`de`) and Italian (`it`) names and
keywords from the same pinned CLDR release. No runtime dependency is added.

`GetEmojiName`, `GetBestEmojiName` and `FormatEmojiDisplayName` provide shared
lookup/fallback and formatting. The formatter accepts primary and optional
secondary locales, omits missing/duplicate names and falls back to English when
neither selected translation exists. **Languages...** in the tray opens a
compact native dialog with primary and optional secondary dropdowns. Secondary
excludes the primary language; selecting its current language as primary clears
secondary to None. Save applies and persists; Close/Escape discards unapplied
changes. A save failure keeps the applied preference in memory and offers retry.
English + Norwegian remains the default for existing users. Picker labels,
variant details, vocabulary and combination editors share these preferences.
Vocabulary results retain their two-line presentation using the shared separator.

Search accepts an independent locale filter of any length. An empty filter uses
all available localizations; aliases and curated intents remain available. Exact,
prefix, token and fuzzy matching iterate locale data. English inflections and the
existing English-name-length tie-break remain unchanged. Display labels do not
switch to the query's language.

The I18N foundation passed all 12 registered CTest suites in `build-i18n` on
2026-09-09, including synthetic third-locale tests and the bilingual corpus.
Python generator tests were not registered (Python unavailable). One isolated
vocabulary visual pass confirmed bilingual result/preview rendering. The desktop
harness completed editor/state checks but Windows denied foreground activation;
actual insertion was skipped.

Settings verification on 2026-09-10: `build-i18n-settings` passed all 13 CTest
suites, including the Python catalog generator, profile migration and language
selection tests. One visual pass confirmed the native language dialog and its
accessible dropdown/button names. The desktop harness completed editor/state
checks but actual insertion was skipped because Windows denied foreground focus.
DPI transitions, high contrast and Narrator remain manual acceptance checks.

`build-i18n-settings\SwashMojiLanguagePreview.exe` opens settings against an
isolated `vocabulary-preview-profile` beside the executable. Open
`SwashMojiVocabularyPreview.exe` to inspect vocabulary using those preferences.

## Existing behavior

Search English, Norwegian Bokmål, German and Italian together, without a language switch or runtime
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

Saved combinations use their normalized names as personal triggers and can also
be alias targets. Their authored payload is independent of global tone. See
[combinations](combinations.md) for editing, limits and verification.

## Catalog provenance and regeneration

`emojis.txt` starts with five tab-separated UTF-8 columns: glyph, English name,
English keywords, Bokmål name, Bokmål keywords. Additional languages use repeated
triples of locale code, localized name and localized keywords. The current catalog
appends `de` and `it` triples. The loader accepts arbitrary locale triples and older
three/five-column catalogs; incomplete triples or duplicate locale codes invalidate
the row. `intent_phrases.tsv` holds 51 separately maintained intent mappings (50 phrases).
Both files and `UNICODE_LICENSE.txt` must be distributed beside the executable;
CMake copies them.

Sources are Unicode CLDR 48.2, tag `release-48-2`, under
[Unicode License v3](../third_party/UNICODE_LICENSE.txt). The checked-in
`data/catalog_sources.json` records source paths and SHA-256 hashes. CLDR's
supplemental parent-locale data resolves `nb` to `no`; the pinned release has no
separate `nb` annotation files. Names/keywords inherit from `no` with English
fallback. Hand-authored annotations override derived annotations. Inheritance
markers are respected. Existing English keyword vocabulary is preserved.

Italian/German expansion (2026-09-10): both languages provide names for all 3,598
catalog variants. The original five columns and original source hashes remain
unchanged. Newly downloaded annotation/derived-annotation hashes and all four
locale codes are recorded in the manifest. Offline regeneration is byte-identical.
Missing additional translations are omitted rather than replaced with English in
the catalog; the shared display formatter supplies fallback when needed.

`.\build.cmd test build-i18n-translations` passes all 13 suites, including the
111-case existing search corpus, exact-name checks for all four locales, Italian/
German preference round-trips, generic catalog-triple parsing, and offline Python
generator fixtures. A vocabulary preview with Italian primary/German secondary
confirmed translated results and selected-name rendering. The compact-grid test
uses three fixed aliases so language additions cannot change its result-count
assumption. English/Norwegian defaults and authored combination payloads remain
unchanged.

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

### Vocabulary visual redesign — 2026-09-09

My vocabulary now groups saved aliases and pinned favorites in a Library panel,
with a separate Alias editor. Results and the selected preview distinguish the
English name from its Bokmål translation. Save alias is the primary action;
Close and New combination / edit remain global footer actions. Native controls
retain their mnemonics, Tab navigation, accessible names and selection behavior.
The resizable layout expands both columns and lists, uses DPI-scaled geometry,
and provides explicit focus outlines, subdued destructive actions and empty states.
Status messages are local to the editor; draft changes replace permanent instructions.

The implementation uses a reusable native presentation layer in vocabulary_style.h
and opt-in single-line DirectWrite ellipsis. No runtime dependencies or profile
format changes were introduced. The first rendered preview was inspected, followed
by a second pass correcting input alignment, preview spacing, footer surfaces,
focus outlines and clipping. The final preview was inspected with empty and saved
aliases; Save and native Tab focus were exercised with its isolated profile.
Build and all 11 registered CTest suites passed with
`.\build.cmd test build-vocabulary-polish`, including layout growth, action alignment,
empty results and draft-status regression checks. Python generator tests were not
registered in this environment. Real multi-monitor DPI transitions and Narrator
remain manual acceptance checks.
The separate picker/vocabulary harness passed its preceding editor and layout
checks but failed at external insertion with "Could not focus the original app"
(`picker-vocabulary-result.txt`); actual target-app insertion remains unverified.
The final button repaint/high-contrast focus correction was verified by a fresh
`.\build.cmd test build-vocabulary-final`: all 11 registered suites passed.
