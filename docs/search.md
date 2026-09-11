# Multilingual search and personal vocabulary

## Locale model and display preferences

The catalog now stores names, keywords and their normalized search caches by
locale code in `Emoji::names`. `SetEmojiLocalization` builds/replaces one locale's
data; `SupportedLocales` registers display metadata and English inflection policy.
The existing three/five-column files are still accepted and map to `en`/`nb`.
The bundled dataset also includes German (`de`), Italian (`it`), French (`fr`) and Spanish (`es`) names and
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
all available localizations; aliases and curated intents remain available.
`SearchLanguagePolicy` separately supplies preferred locales, without a two-locale
limit. When omitted, Search uses the profile's display locales as preferences,
not as a hard filter. Exact names, keywords, prefixes and all-token lexical
matches continue across the filter's locales. Keeping non-fuzzy fallback preserves
the existing multilingual behavior, including mixed-language token queries.

Catalog fuzzy matching runs only in the intersection of preferred locales and
the filter, and only if there are no ordinary results anywhere. An explicit empty
preference or empty intersection disables catalog fuzzy matching; it must not be
passed as an empty low-level scorer filter (which means all locales). Missing
preferred translations do not expand fuzzy matching to other languages. Both
selected languages have equal search preference; primary/secondary orders labels.
The low-level `LexicalScore` and `FuzzyScore` retain their explicit filter semantics.

Match classes, query learning, usage/recency and lexical detail retain their
priority. A preferred-language match breaks ties after lexical detail and before
default popularity/stable ID. It must achieve the winning tier and detail using
preferred locales alone; a weak preferred match cannot promote a stronger fallback
match. Exact aliases/names retain their existing priority. English inflections and
the English-name-length tie-break remain unchanged. Display labels use only display
preferences, even with an independent search-policy override.

Curated intents currently have no locale tags: they are a small, separately
maintained phrase list, not all CLDR translations. Exact, partial and fuzzy phrase
matching remains locale-neutral, as do personal aliases and combination names.
If curated phrases eventually expand into a large multilingual dataset, add locale
provenance before applying preferred/fallback policy to those phrases; do not infer
their language from spelling. No new user setting or profile format is introduced.

`SwashMojiLanguagePreview.exe` opens settings against an isolated
`vocabulary-preview-profile` beside the executable, shared with the vocabulary
preview. See [contributor workflow](../CONTRIBUTING.md) for building the previews.

## Existing behavior

Search English, Norwegian Bokmål, German, Italian, French and Spanish together, without a language switch or runtime
network access. Exact personal aliases rank first, then exact English/localized
names or a pasted known emoji, exact curated intent phrases, name prefixes,
all-token lexical matches, and finally preferred-locale catalog fuzzy matches only
when ordinary results are absent. Curated/personal phrases retain their locale-neutral
fallback. Partial aliases use the corresponding prefix/token classes. Existing
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
appends `de`, `it`, `fr` and `es` triples. The loader accepts arbitrary locale triples and older
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

German, Italian, French and Spanish provide names for all 3,598 catalog variants. Their addition
preserved the original five columns and source hashes; new source hashes are in
the manifest. Missing additional translations stay absent in the catalog; the
shared display formatter supplies fallback. Tests cover generic locale triples,
localized exact names, preference round-trips and deterministic offline generation.

```powershell
python tools/update_emoji_catalog.py --download --cldr-dir build/cldr
python tools/update_emoji_catalog.py --cldr-dir build/cldr
python tests/test_catalog_generator.py
.\build.cmd test build-agent
```

Generator tests need Python 3.10 or later. If Python is not on PATH, set
`$env:SWASHMOJI_PYTHON` to its executable path before running `build.cmd test` to
include those tests in CTest.

Use a fresh cache directory when changing the pinned version. Download mode accepts
404 only for optional inherited locale files and records those absences; missing
files in offline mode are errors. Generating twice from the same inputs gives the
same catalog and manifest. Runtime startup reads only the bundled generated files.

## Verification

Preferred-language policy regression fixtures in `tests/core_tests.cpp` cover
unselected exact names/keywords and prefixes, primary/secondary fuzzy matching,
blocked unselected typos, exact fallback versus preferred prefixes, locale ties
and usage precedence, independent display labels/search overrides, hard filters,
empty preference intersections, and curated/personal phrase priority.

Policy verification (2026-09-10): `.\build.cmd test build-search-policy` built the
application and passed all 13 CTest suites with the bundled Python runtime enabled,
including the existing multilingual corpus and catalog generator. No catalog or
profile migration was needed. Separate real-input desktop harnesses were not run
for this search-only change.

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

For current automated results and outstanding desktop, accessibility and release
checks, see [release validation](release-validation.md).
