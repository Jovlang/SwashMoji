# M6 release validation

M6 now has a repeatable automated release workflow. Release acceptance remains
open: automated and hidden-control checks cannot establish actual delivery to
all target apps, visible opening time, accessibility or user-study outcomes.

## Reproduce the automated evidence

From the repository root, set `SWASHMOJI_PYTHON` to a Python 3.10+ executable and run:

```powershell
$env:SWASHMOJI_PYTHON = 'C:\path\to\python.exe'
.\tools\verify_release.ps1 -BuildDirectory build-m6
```

This runs the Release build and all CTest suites, requiring `catalog_generator`
to be registered. It creates a unique `build-m6/release-*/` directory containing
build/test and install logs, staged and extracted packages, smoke-test logs,
`performance.csv`, `manifest.json` and `SwashMoji-portable.zip`. Failures stop the
workflow; a partial directory without the final manifest is not a verified candidate.
Nothing is published. Existing output directories and running pickers are retained.

The portable payload contains only `SwashMoji.exe`, `emojis.txt`,
`intent_phrases.tsv` and `UNICODE_LICENSE.txt`. ZIP round-trip hashes are checked,
including extraction into a path containing spaces and æøå. The smoke runner loads
the staged catalog and intents and uses separate, unique profile roots to verify
fresh startup data, bilingual search, a saved combination/alias/pin/choice across
restart, v1 migration, backup recovery and future-version write protection.
It calls the shared core; it does **not** launch the packaged application or prove
its foreground, hotkey, rendering or DLL behavior on a clean Windows installation.

The manifest records SHA-256 for runtime files, ZIP and tracked/untracked source
files, commit and working-tree status, CPU, OS build, logical CPU count, compiler
path, build type and Python path. The evidence describes the working tree, including
any pre-existing edits, rather than claiming it is a clean tagged release.

## Performance method

`SwashMojiPerformance.exe` exercises the actual `BeginPickerSession` and synchronous
search-edit/result-list update code in hidden native controls. There are 10 warmups
and 200 samples per metric; p50 and p95 use nearest-rank samples 100 and 190.
The catalog is already loaded. Each query is repeatedly assigned in a warm session;
results are rebuilt on every assignment. This measures repeated-query latency,
not a representative distribution of changing keystrokes.

The fixture contains 500 aliases, 200 two-emoji combinations, 1,000 query pairs,
40 recent entries, ten pins and usage for every catalog family. It is codec-validated
and never saved to the user's profile. Queries cover a broad prefix, English,
Bokmål, intent, alias, combination, typo and miss. Timings include matching,
ranking and native list/status updates, but exclude painting, foreground activation,
monitor placement, cold catalog load, disk writes and human interaction.

Initial measurements on 2026-09-09, Intel Core i5-12400F, MSVC 19.44 x64 Release:
session preparation p95 18.245–19.358 ms; broad `r` update p95 18.150–32.921 ms;
other query p95 values 3.129–9.427 ms. The second run overlapped a native harness,
so these samples establish variability, not a controlled reference-machine pass.
Preserve per-run CSVs rather than selecting only favorable samples. The original
p95 targets (<100 ms visible warm opening and <20 ms query-to-results) remain open
until an idle reference-machine run measures the full visible paths.

## Recorded automated/native results — 2026-09-09

- Release build and all 13 CTest suites passed: release_smoke, combination_editor,
  combinations, picker_keyboard, picker, ranking, core, storage, insertion,
  vocabulary, learning, edit_controls and catalog_generator. The vocabulary suite
  includes the 111-case bilingual corpus; Python uses offline fixtures.
- Staged and extracted ZIP catalog/profile smoke checks passed.
- `SwashMojiNativeInputTests.exe`: exit 77, desktop denied foreground activation.
- `SwashMojiPickerVocabularyTests.exe`: editor/state checks completed; exit 77,
  desktop denied picker foreground activation. No actual insertion pass is claimed.
  The harness now explicitly checks that setup precondition. If setup succeeds,
  insertion/recovery or exact-receipt failures still fail the test; no retry or
  application focus workaround was added.

Run desktop harnesses separately with modifier keys released. Preserve
`native-input-result.txt` and `picker-vocabulary-result.txt` beside the executables.
Exit 77 means unverified, not passed. This task did not resume visual automation.

## Remaining acceptance protocol

Record candidate ZIP hash, date, tester, Windows/app versions, exact steps,
expected/actual text, focus behavior, outcome and evidence path for every row.
Use isolated test profiles and synthetic text; never reset the real user profile.
Do not check a row merely because the corresponding state test passes.

| Gate | Required exercise | Status |
| --- | --- | --- |
| Picker walkthrough | English/Bokmål footer, long/missing names, favorites, hover versus keyboard selection, Details and one-use variants; all shortcuts and 1–3 rows including partial pages | Open |
| Combination walkthrough | Create launch → 🚀✨ and please → 🥺🙏; reorder, tone/ZWJ variants, rename, alias, pin, copy, insert, restart, delete cascade and cancellation | Open |
| Controlled Win32 target | Run both native harnesses in an interactive foreground-capable session; check exact UTF-16 and unchanged clipboard for direct input | Skipped here |
| Target applications | Notepad, browser input and contenteditable, Chromium desktop editor/chat app and terminal: Enter, Ctrl+Enter repeats, copy/paste, editor round trip and failure recovery | Open; record actual versions |
| Elevated target boundary | Normal-integrity picker over elevated synthetic target; record blocked/accepted behavior and copy recovery, without elevating the picker | Open |
| DPI/monitors | 100%, 125%, 150%, 200%, mixed-DPI transitions and screen edges across picker, both editors, help and Details | Open; requires suitable displays |
| Accessibility | High contrast, keyboard focus, Inspect names/selection semantics, Narrator variants, empty state and recovery announcements | Open |
| Visible performance | Idle documented machine; populated fixture; at least 200 warm opens and query updates through first paint; retain raw samples and p95 | Open |
| Portable application | Launch extracted executable on clean Windows with isolated LOCALAPPDATA (both SwashMoji and WinMoji roots); hotkey/tray, fresh profile, restart/migration, backup and future version | Core/data checks passed; application launch open |
| Consented user trials | Baseline and candidate, same device and task set, counterbalanced order; opening-to-insertion time, intended rank and abandonment | Open; no participants recruited |

For trials, obtain participant consent before collecting observations. Use a small
fixed English/Bokmål task set plus favorites, aliases and combinations; record
training, task order, sample size and familiarity. Report paired timings/ranks,
abandonment counts and limitations rather than inferring improvement from synthetic
benchmarks. Store only consented, anonymized results locally; no production telemetry.

Release acceptance closes only when these rows have evidence and any failures
are resolved or explicitly documented as accepted compatibility limitations.
