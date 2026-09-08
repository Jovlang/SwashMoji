# Working on SwashMoji

## Project

SwashMoji is a self-contained Windows emoji picker written in C++17 using native
Win32 controls, Direct2D and DirectWrite. Keep runtime operation offline and avoid
adding dependencies without a concrete need. Source and catalog files are UTF-8.

Read `README.md` for user behavior and commands. Consult the relevant document in
`docs/` before changing search, learning, storage or insertion. The
`IMPLEMENTATION_PLAN.md` describes planned work; verify current behavior in code.

## Code map

- `main.cpp`: picker window, tray, shortcuts, rendering and session coordination.
- `vocabulary.cpp/.h`, `vocabulary.rc`, `vocabulary_ids.h`: vocabulary editor UI.
- `catalog.cpp/.h`, `text.cpp/.h`, `search.cpp/.h`, `ranking.h`: catalog loading,
  Unicode normalization, bilingual matching and ranking.
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

`SwashMojiNativeInputTests.exe` and `SwashMojiPickerVocabularyTests.exe` are separate
from CTest: they require an interactive desktop, change focus and submit real
input. Run them when relevant with modifier keys released, using their isolated
profiles. `SwashMojiPickerPreview.exe` and `SwashMojiVocabularyPreview.exe` support
visual and keyboard checks. See `docs/insertion.md`, `docs/search.md`, and
`docs/combinations.md` for details. Do not terminate a user's running picker merely
to unlock a build; use another build directory.

## Behavior to preserve

- English and Norwegian Bokmal search together. Preserve Unicode normalization,
  exact alias/name priority and fuzzy fallback rules in `docs/search.md`.
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
  History clearing retains aliases, combinations, favorites and appearance settings.
  Consult `docs/profile-format.md` before changing the file format.

## Catalog and documentation

For catalog regeneration, follow `docs/search.md` and the pinned source manifest:

```powershell
python tools\update_emoji_catalog.py --download --cldr-dir build\cldr
# Reuse downloaded sources offline:
python tools\update_emoji_catalog.py --cldr-dir build\cldr
```

Keep generation deterministic and preserve source hashes and Unicode licensing.
Distribute `emojis.txt`, `intent_phrases.tsv` and `UNICODE_LICENSE.txt` beside the
executable; CMake copies these files. Treat `build*/` as generated output.

Update relevant documentation when changing shortcuts, visible behavior, profile
formats or workflow. Preserve the Norwegian language of `README.md` and the
existing language of UI strings. Keep changes focused and preserve unrelated
working-tree edits.
