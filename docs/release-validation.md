# Release validation

SwashMoji has a repeatable automated release workflow. Release acceptance remains
open: automated and hidden-control checks cannot establish actual delivery to
all target apps, visible opening time, accessibility or user-study outcomes.

## Help and diagnostic localization — 2026-09-16

The Release build and all 13 CTest suites passed in `build-agent`, including the
offline generator and the new storage, picker-keyboard and combination-editor
localization checks (120.24 seconds for the final full run).

F1 help, insertion/copy recovery, storage and startup diagnostics, profile import
and alias replacement confirmations, combination deletion, file-filter labels,
and picker status text now use the selected interface language. The help no longer
describes the removed hover preview. Importing a profile or changing the interface
language refreshes the picker's action labels and accessible result name.

Automated regression coverage checks every new translation entry, help shortcuts,
joined recovery/save diagnostics, English fallback, double-NUL file-filter framing,
actual hidden picker messages, and localized deletion confirmations without
rewriting the listed personal aliases. Native Windows dialog buttons still follow
the OS language; an error before profile loading uses the default English locale.
Visual fit, linguistic review and screen-reader acceptance remain desktop/manual
checks; no computer-use or real-input test was run for this change.

## Version 0.3 automated candidate — 2026-09-14

The `build-release-0.3` Release build and all 13 CTest suites passed, including
the offline catalog generator. Staged-package and extracted-ZIP smoke checks also
passed. The portable payload contains the expected five files and round-tripped
without hash changes.

Hidden-control performance p95 values were 21.135 ms for empty-session preparation,
29.119 ms for the broad `r` query, and 11.358–17.858 ms for the other recorded
queries. These measurements do not close the visible-performance gate below.
Desktop insertion, compatibility, DPI, accessibility and user acceptance remain
open as documented in the acceptance protocol.

## Localization follow-up — 2026-09-15

The Release build and all 13 CTest suites passed in `build-agent`, including
the new localization regressions and offline catalog generator. No interactive
desktop harness was run for this follow-up.

Dialog localization now retains resource keyboard mnemonics, preserves literal
ampersands, and excludes editable/list controls from label translation. Additional
Settings, Details and combination-message translations cover all five non-English
interface locales. Hidden-control regression checks exercise translated Save
mnemonics and ensure an edit value equal to a translation key remains unchanged.
At this stage, help, some diagnostics and confirmations remained English; see the
September 16 follow-up above. Translation layout and screen-reader acceptance
are still open.

A fresh hidden-control performance run is retained in
`build-agent/performance-20260915-localization.csv`: session preparation p95
22.589 ms, broad `r` query 33.744 ms, and other queries 12.008–20.327 ms.
This run overlapped the automated search suite, so it is diagnostic evidence,
not an idle-machine performance acceptance result. No performance improvement
or visible-latency pass is claimed.

## Version 0.3 desktop follow-up — 2026-09-15

Both interactive harnesses passed when run in a foreground-capable desktop session.
`SwashMojiNativeInputTests.exe` verified exact multi-code-point emoji and repeat
input into a native Win32 edit, an unchanged clipboard, and rejection of stale and
internal targets. `SwashMojiPickerVocabularyTests.exe` verified persisted alias and
favorite edits, failure recovery without learning, one-time learning after successful
insert/copy, stable selection across repeat insertion, rows, tone and font changes,
next-session ranking, exact delivery to the original target, and an unchanged clipboard.

The executable from the extracted 0.3 ZIP also remained running after launch from
the release workflow's path containing spaces and `æøå`, using an isolated
`LOCALAPPDATA`; the test process was then stopped. This verifies packaged executable
startup on the release machine, but not hotkey/tray behavior or clean-Windows startup.

## Historical: startup and shortcut settings — 2026-09-13

`build-agent` builds successfully with the new Settings dialog. All 12 C++ suites
passed across the full run and a targeted storage rerun. The sandboxed storage
run could not write its isolated registry fixture; running that suite outside
the sandbox passed. The fixture never accesses the real Windows Run key or user
profile. Tests cover shortcut validation, conflict retention and cancellation
with an injected registration adapter, profile v5-to-v6 migration and round trips,
hidden dialog controls, and actual registry enable/update/disable operations under
a disposable test key, including paths with spaces and Unicode.

The Python generator suite still fails on the pre-existing duplicate `bien` → 👍
in `intent_phrases.tsv`. Actual global shortcut activation, visible Settings
layout/accessibility, sign-in startup, and Windows Startup apps overrides still
need desktop acceptance. No real startup entry was enabled during verification.

## Historical: search-policy verification — 2026-09-10

`build-search-policy` is the retained build. The application build and all 13 CTest
suites passed, including the 111-case search corpus, multilingual exact-name
coverage, preferred-language search regressions and offline Python generator tests.
No separate real-input desktop harness was run for the latest search-policy change.

Older build directories and their local artifacts have been removed. Dated results
below are historical summaries, not retained release artifacts or verification of
the latest build. Re-run the release workflow to produce a current package and evidence.

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
`intent_phrases.tsv`, `LICENSE` and `UNICODE_LICENSE.txt`. ZIP round-trip hashes are checked,
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
Norwegian, intent, alias, combination, typo and miss. Timings include matching,
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
Exit 77 means unverified, not passed. These historical runs did not complete visual acceptance.

## Remaining acceptance protocol

The [2026-09-15 real-application attempt](compatibility-2026-09-15.md) was blocked
by desktop test setup and closes no target-application gate. It also identifies
why an environment-only `LOCALAPPDATA` override must not be assumed to isolate
the packaged application's profile; verify the resolved directory or use a
dedicated Windows test account.

Record candidate ZIP hash, date, tester, Windows/app versions, exact steps,
expected/actual text, focus behavior, outcome and evidence path for every row.
Use isolated test profiles and synthetic text; never reset the real user profile.
Do not check a row merely because the corresponding state test passes.

| Gate | Required exercise | Status |
| --- | --- | --- |
| Picker walkthrough | Selected-language footer, long/missing names, favorites, pointer versus keyboard selection, Details and one-use variants; all shortcuts and 1–3 rows including partial pages | Open |
| Combination walkthrough | Create launch → 🚀✨ and please → 🥺🙏; reorder, tone/ZWJ variants, rename, alias, pin, copy, insert, restart, delete cascade and cancellation | Open |
| Controlled Win32 target | Run both native harnesses in an interactive foreground-capable session; check exact UTF-16 and unchanged clipboard for direct input | Passed 2026-09-15 |
| Target applications | Notepad, browser input and contenteditable, Chromium desktop editor/chat app and terminal: Enter, Ctrl+Enter repeats, copy/paste, editor round trip and failure recovery | Open; record actual versions |
| Elevated target boundary | Normal-integrity picker over elevated synthetic target; record blocked/accepted behavior and copy recovery, without elevating the picker | Open |
| DPI/monitors | 100%, 125%, 150%, 200%, mixed-DPI transitions and screen edges across picker, both editors, help and Details | Open; requires suitable displays |
| Accessibility | High contrast, keyboard focus, Inspect names/selection semantics, Narrator variants, empty state and recovery announcements | Open |
| Visible performance | Idle documented machine; populated fixture; at least 200 warm opens and query updates through first paint; retain raw samples and p95 | Open |
| Portable application | Launch extracted executable on clean Windows with isolated LOCALAPPDATA (both SwashMoji and WinMoji roots); hotkey/tray, fresh profile, restart/migration, backup and future version | Extracted app launch passed on release machine; core/data checks passed; clean-Windows and visible behavior open |
| Consented user trials | Baseline and candidate, same device and task set, counterbalanced order; opening-to-insertion time, intended rank and abandonment | Open; no participants recruited |

For trials, obtain participant consent before collecting observations. Use a small
fixed English/Norwegian task set plus favorites, aliases and combinations; record
training, task order, sample size and familiarity. Report paired timings/ranks,
abandonment counts and limitations rather than inferring improvement from synthetic
benchmarks. Store only consented, anonymized results locally; no production telemetry.

Release acceptance closes only when these rows have evidence and any failures
are resolved or explicitly documented as accepted compatibility limitations.
