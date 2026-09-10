# Working on SwashMoji

## Project

SwashMoji is a self-contained Windows emoji picker written in C++17 using native
Win32 controls, Direct2D and DirectWrite. Keep runtime operation offline and avoid
adding dependencies without a concrete need. Source and catalog files are UTF-8.

Read `README.md` for the product overview, `docs/user-guide.md` for user behavior
and commands, and `CONTRIBUTING.md` for general build/test workflow. Consult the
relevant technical document in `docs/` before changing search, learning, storage
or insertion. Localization scope and search policy are documented in
`docs/search.md`; verify current behavior in code before changing it.

## Code map

- `main.cpp`: picker window, tray, shortcuts, rendering and session coordination.
- `vocabulary.cpp/.h`, `vocabulary.rc`, `vocabulary_ids.h`: vocabulary editor UI.
- `language_preferences.h`, `language_preferences.rc`: native **Languages...**
  preferences dialog; `DisplayLanguages` in `models.h` validates the selection.
- `catalog.cpp/.h`, `text.cpp/.h`, `search.cpp/.h`, `ranking.h`: catalog loading,
  locale-keyed names and search caches, shared name formatting, Unicode
  normalization, multilingual matching and ranking.
- `picker.cpp/.h`: testable picker-session state, grid navigation and selection.
- `models.h`, `personalization.cpp/.h`, `storage.cpp/.h`: profile data, learning,
  favorites, saved combinations, persistence and migration.
- `insertion.cpp/.h`: testable insertion and clipboard logic;
  `insertion_win32.cpp/.h`: Windows adapters.
- `edit_controls.cpp/.h`: shared native text-edit behavior.
- `tests/`: C++ regression suites, desktop integration tools and Python generator
  tests. `tools/update_emoji_catalog.py` generates the catalog.
- `emojis.txt`, `intent_phrases.tsv`, `data/catalog_sources.json`, `third_party/`:
  catalog, curated phrases, source provenance and licenses.

Keep reusable behavior in `SwashMojiCore` and UI coordination in the application.
Follow surrounding C++ conventions, including the `SwashMoji` namespace. Preserve
UTF-16 correctness at Windows API boundaries and UTF-8 encoding on disk.

## Build and verification

Use PowerShell from the repository root with native Visual Studio 2022 C++ Build
Tools installed. `build.cmd` discovers the compiler, bundled CMake and Ninja.

```powershell
.\build.cmd
.\build.cmd test
# Use a separate build directory when an existing executable is running:
.\build.cmd test build-agent
```

Run the build and CTest suites for C++ or build-system changes. Add focused
regression coverage for behavior changes in the relevant existing suite. A
documentation-only change does not require compiling. Report checks actually run
and any verification gaps.

Catalog generator tests require Python 3.10 or later:

```powershell
python tests\test_catalog_generator.py
```

Set `$env:SWASHMOJI_PYTHON` to a Python executable before configuring if CMake
cannot discover it. CTest includes generator tests only when Python is found.
The current full suite contains 13 tests with Python enabled; inspect the actual
CTest output rather than assuming generator coverage. Use deterministic fixtures
for result-count/layout checks, since new translations can add valid matches.

`SwashMojiNativeInputTests.exe` and `SwashMojiPickerVocabularyTests.exe` are separate
from CTest: they require an interactive desktop, change focus and submit real
input. Run them when relevant with modifier keys released, using their isolated
profiles. A foreground-activation denial leaves actual insertion unverified even
when editor/state checks pass. `SwashMojiPickerPreview.exe`,
`SwashMojiVocabularyPreview.exe`, and `SwashMojiCombinationPreview.exe` support
visual and keyboard checks. `SwashMojiLanguagePreview.exe` opens language settings;
it shares the isolated `vocabulary-preview-profile` beside the executable with
the vocabulary preview. See `docs/insertion.md`, `docs/search.md`, and
`docs/combinations.md` for details. Do not terminate a user's running picker merely
to unlock a build; use another build directory.

## Behavior to preserve

- English (`en`), Norwegian Bokmål (`nb`), German (`de`) and Italian (`it`) search
  together. Search locale filters are independent of display preferences; an empty
  filter searches all available localizations for non-fuzzy matches. Search uses
  display locales as default preferences, not a hard filter: catalog fuzzy matching
  is restricted to preferred locales, and language preference breaks ties after
  lexical detail. Keep SearchLanguagePolicy independent and unrestricted in length.
  Curated/personal phrases remain locale-neutral until intents have locale metadata.
  Preserve Unicode normalization,
  English-only inflection rules, exact alias/name priority and fuzzy fallback
  rules in `docs/search.md`.
- Keep names, keywords and normalized search caches in the canonical
  `Emoji::names` locale map. Use `SetEmojiLocalization` to build/replace caches,
  `SupportedLocales` for locale metadata and `FormatEmojiDisplayName` for UI names.
  Do not add per-language fields or separate translation mappings in UI code.
- **Languages...** selects one or two distinct registered display locales, primary
  first, with English + Norwegian as the default. Pass the profile's display
  preferences through picker, vocabulary, combination and details views. Omit
  missing/duplicate names and dangling separators; fall back to English when
  neither selected name exists. Matching another locale must not change display
  languages. Localized emoji names do not translate the app's interface strings.
- Query learning stays within match classes and uses the complete normalized
  query. Ranking snapshots keep repeated insertions stable within a session.
  Preserve family IDs across skin tones; see `docs/learning.md`.
- Saved combinations retain their generated IDs across edits and store an exact
  payload of 2–8 catalog emoji variants. Global tone and catalog updates must not
  rewrite that payload. Treat insertion, copying, history and learning as one
  combination result, and cascade deletion through dependent personalization;
  see `docs/combinations.md`.
- Only completed insertion or explicit copying records a choice. Direct insertion
  leaves the clipboard untouched. Never automatically retry partial input.
  Failures preserve the query and selection; see `docs/insertion.md`.
- Preserve profile migration, backup recovery and unsupported-version protection.
  Use isolated directories for tests, never the user's real
  `%LOCALAPPDATA%\SwashMoji` or legacy `%LOCALAPPDATA%\WinMoji` data.
  Profile version 5 stores `display_languages`; versions 1–4 migrate with `en`,
  `nb` defaults. History clearing retains aliases, combinations, favorites,
  appearance settings and display languages.
  Consult `docs/profile-format.md` before changing the file format.

## Catalog and documentation

For catalog regeneration, follow `docs/search.md` and the pinned source manifest:

```powershell
python tools\update_emoji_catalog.py --download --cldr-dir build\cldr
# Reuse downloaded sources offline:
python tools\update_emoji_catalog.py --cldr-dir build\cldr
```

The catalog retains five legacy columns (glyph, English name/keywords, Bokmål
name/keywords), followed by locale/name/keywords triples for additional languages.
Keep older three/five-column loading compatible. To add a language, register its
metadata in `SupportedLocales` and its source in the generator's `EXTRA_LOCALES`;
use the generic loader/search rather than adding language-specific branches.
Missing additional translations stay absent; display fallback handles them.

Use the pinned CLDR release and preserve source hashes and Unicode licensing.
When expanding locales, preserve existing language columns and source hashes,
record the new sources in `data/catalog_sources.json`, and verify byte-identical
offline regeneration. Add generator fixtures, localized search/name coverage and
display-preference round-trips for the new locales.
Distribute `emojis.txt`, `intent_phrases.tsv` and `UNICODE_LICENSE.txt` beside the
executable; CMake copies these files. Treat `build*/` as generated output.

Update relevant documentation when changing shortcuts, visible behavior, profile
formats or workflow. Keep `README.md` a concise project overview; detailed user
instructions belong in `docs/user-guide.md`, general contributor workflow in
`CONTRIBUTING.md`, and technical rules/evidence in the corresponding `docs/` file.
Keep this file focused on agent/developer invariants. Preserve Norwegian in the
README and user guide and the existing language of UI strings. Keep changes
focused and preserve unrelated working-tree edits.
