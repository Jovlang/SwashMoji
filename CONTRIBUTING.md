# Contributing to SwashMoji

For application usage, see the [user guide](docs/user-guide.md).
This page covers the general build and verification workflow. Agent-specific
architecture and behavior invariants remain in [AGENTS.md](AGENTS.md).

## Build on Windows

Install Visual Studio 2022 C++ Build Tools with the native C++ toolchain and its
bundled CMake/Ninja tools. Run PowerShell from the repository root:

```powershell
.\build.cmd
```

The script discovers the tools automatically and builds the C++17 application.
Run `build\SwashMoji.exe`; no SwashMoji installation or separate VC runtime install
is required. Keep `emojis.txt`, `intent_phrases.tsv` and `UNICODE_LICENSE.txt` beside
the executable when moving it. Runtime operation is offline.

## Automated and desktop checks

```powershell
.\build.cmd test
# If the usual executable is running, build and test in a separate directory:
.\build.cmd test build-agent
```

Do not stop a user's picker to unlock a build. Tests use isolated directories
under the build directory and must not touch the real SwashMoji or WinMoji profile.
Run the build and relevant suites for code changes; documentation-only changes
need link and content checks, not compilation.

Catalog generator tests need Python 3.10 or later. If discovery fails, set the
executable path before configuring:

```powershell
$env:SWASHMOJI_PYTHON = 'C:\path\to\python.exe'
.\build.cmd test build-agent
& $env:SWASHMOJI_PYTHON tests\test_catalog_generator.py
```

CTest registers the Python tests only when Python is found. Check the actual
suite list and results; the current full configuration has 13 suites.

`SwashMojiNativeInputTests.exe` and `SwashMojiPickerVocabularyTests.exe` run
separately from CTest because they require an interactive desktop, change focus
and send real input. Release modifier keys first. The native-input test creates
a temporary external text field and checks exact Unicode insertion without
changing the clipboard. Its result is `native-input-result.txt` beside the
executable; the picker/editor harness writes `picker-vocabulary-result.txt`.
Exit 77 means skipped/unverified, not passed. See the
[insertion test instructions](docs/insertion.md#verification) and
[release protocol](docs/release-validation.md) for scope and remaining gaps.

For visual or keyboard checks, use the isolated picker, vocabulary, combination
and language preview executables described in [selection](docs/selection.md),
[search](docs/search.md) and [combinations](docs/combinations.md).

## Technical documentation

`SwashMojiCore` holds reusable catalog, search, personalization, storage and
insertion behavior; application code coordinates the native UI. Consult the
appropriate reference before changing behavior:

| Area | Reference |
| --- | --- |
| Search, locales, aliases, CLDR sources and catalog regeneration | [Search](docs/search.md) |
| Learning, favorites and stable ranking within a session | [Learning](docs/learning.md) |
| Insertion, clipboard handling and recovery | [Insertion](docs/insertion.md) |
| Saved sequences, variants and editor behavior | [Combinations](docs/combinations.md) |
| Profile format, migration and backup recovery | [Profile format](docs/profile-format.md) |
| Keyboard selection, Details, DPI and accessibility | [Selection](docs/selection.md) |
| Release evidence and remaining acceptance checks | [Release validation](docs/release-validation.md) |

The [catalog regeneration instructions](docs/search.md#catalog-provenance-and-regeneration)
cover pinned CLDR downloads, offline cache reuse, source hashes, deterministic
output and Unicode licensing. Keep those procedures in the search reference.

## Release candidates

With `SWASHMOJI_PYTHON` pointing to Python 3.10+, run
`.\tools\verify_release.ps1`. It builds, tests, measures response time and creates
a portable ZIP with checksums in a new subdirectory of `build-m6`; it does not
publish a release. The complete procedure, evidence and outstanding desktop/user
acceptance work live in [release validation](docs/release-validation.md).

## Documentation changes

Keep [README.md](README.md) a short English project overview. Put complete
user instructions in [docs/user-guide.md](docs/user-guide.md), build/development
workflow here, and technical rules or dated evidence in the appropriate reference.
Write documentation in English and retain the UI's actual labels.
Distinguish historical test results from current verification; do not turn an
automated pass into a claim of full desktop or release acceptance.
