# SwashMoji: implementation plan for all five improvements

Status updated 2026-09-08: M0–M3 implemented. M4 is next; M5 and M6 remain planned.
The M3 implementation and Ctrl+Backspace support are committed and pushed in
[`4dba231`](https://github.com/Jovlang/SwashMoji/commit/4dba2318e3fab21761f93a0b2a93bf1953eaecf1).
Automated and controlled native checks pass; the final M3 favorites visual review
was interrupted when Computer Use was stopped and remains outstanding for M4.

M0 delivery: shared catalog/text/search/personalization/storage modules, stable
family/result identities and picker-session state, version 1 profile migration,
atomic replacement with backup recovery, and three passing CTest suites via
`.\build.cmd test build-m0`. Existing insertion and shortcut handlers remain in
`main.cpp` for their later milestones; the proposed file table below describes
the eventual module boundaries. Profile v1 deliberately retains exact-glyph
history at that milestone; M3 subsequently migrated it to family IDs in profile v3.
The catalog now uses existing valid bases rather than fabricating an empty or
unsupported base from tone modifiers. See `docs/profile-format.md` for details.

M1 delivery: platform-independent insertion/copy outcomes and failure tests,
Win32 adapters, preserved query/selection with a Copy instead action, bounded
foreground synchronization, and guarded one-shot focus restoration. Four CTest
suites pass via `.\build.cmd test build-m1`. The separate native input test passed
on the interactive desktop, verifying actual multi-code-point emoji and repeat
input, clipboard preservation, and stale/internal target rejection. The wider
application, DPI, and accessibility matrix remains part of M4/M6. See
`docs/insertion.md` for the tested scope and remaining compatibility limits.

M2 delivery (2026-09-07): pinned CLDR English/Bokmål catalog with Norwegian
inheritance and English fallback, separately maintained curated intent phrases,
Unicode normalization, priority-ranked personal aliases, and a native My vocabulary
editor accessible from results, empty search and the tray. Profile version 2
introduced aliases while preserving settings/history. The milestone passed its
109-case corpus and native alias CRUD/reload, cancellation, target preservation,
and external text delivery checks. See [search documentation](docs/search.md).

M3 delivery (2026-09-08): query-specific learning with 1,000-pair LRU eviction,
saturating counts, a Learn from searches toggle, and Clear learned history;
up to ten ordered favorites with `Alt+P`, context-menu actions, and vocabulary
Up/Down/Unpin controls; family-based history and usage with profile v3 migration;
and frozen ranking preferences during an open picker session. Selection survives
font, tone and row changes. Curated `nice` matches now include both 👌 and 👍.
See [learning documentation](docs/learning.md) and [profile format](docs/profile-format.md).

Additional delivered shortcut: `Ctrl+Backspace` deletes selected text or the
previous whitespace-delimited word in search and vocabulary text fields, with
native undo support. Tests cover caret positions, selections, whitespace,
Norwegian text, emoji sequences, surrogate pairs, repeated deletion and undo.

Latest verification: `.\build.cmd test build-m3-edit` passes all eight CTest
suites: ranking, core, storage, insertion, vocabulary, learning, edit controls,
and the offline catalog generator. The corpus passes 111/111 cases, including
51/51 intent cases. The M3 native picker/editor test separately passed favorite
editing/persistence, no learning on failures, once-per-success insertion/copy
learning, stable session order/selection, next-session ranking, and actual
delivery to the original external Win32 edit without changing the clipboard.
The latest full build includes Ctrl+Backspace; the broader app/DPI/accessibility
matrix and interrupted favorites visual review are not claimed as completed.

Product goal: make the intended emoji or saved combination easy to find, predictable to select, and reliable to insert. Preserve the native C++17/Win32 application, portable distribution, offline runtime, and local personalization.

## Delivery order

The five product steps are covered below; implementation starts with shared foundations and insertion recovery because the other features depend on them.

| Milestone | Status | Scope | Depends on | Completion gate |
| --- | --- | --- | --- | --- |
| M0 | Implemented | Extract shared models, persistence, and test seams | Existing app | Current search and shortcuts preserved; migration fixtures pass |
| M1 | Implemented | Step 4: insertion and clipboard recovery | M0 | Failure paths preserve the selection; controlled target tests pass |
| M2 | Implemented | Step 1: bilingual intent search and personal aliases | M0 | Search acceptance corpus and alias workflows pass |
| M3 | Implemented; visual follow-up noted above | Step 2: query learning, stable favorites, tone-family history | M2 | Deterministic ranking, migration, and session stability pass |
| M4 | Next | Step 3 plus remaining step 4: selection UI, variants, DPI, accessibility | M1, M3 | Keyboard, pointer, screen-reader, and monitor matrix pass |
| M5 | Planned | Step 5: saved combinations | M4 | Create, edit, search, pin, insert, and copy sequences end to end |
| M6 | Planned | Integrated release verification and documentation | M1–M5 | All automated checks and documented manual acceptance pass |

Each milestone should be a reviewable change or a small series of changes. Keep the application buildable throughout. These are dependency boundaries, not calendar estimates; target-app compatibility and accessibility are the largest uncertainty.

## M0: shared foundations

Extract responsibilities incrementally from `main.cpp`, rather than redesigning the whole application first:

| Proposed files | Responsibility |
| --- | --- |
| `catalog.h/.cpp` | Catalog loading, localized annotations, family and variant indexes |
| `search.h/.cpp`, `ranking.h` | Query normalization, matching, deterministic ordering |
| `personalization.h/.cpp` | Aliases, query preferences, favorites, history |
| `storage.h/.cpp` | Versioned local data, validation, atomic replacement, migration |
| `insertion.h/.cpp` | Target capture, insertion attempt state, clipboard operations |
| `picker.h/.cpp` | Result selection, preview, keyboard navigation, layout |
| `vocabulary.h/.cpp`, `vocabulary.rc` | Implemented native alias/favorites editor; combinations extend it in M5 |
| `edit_controls.h/.cpp` | Shared Ctrl+Backspace behavior for native text fields |
| `main.cpp` | Process lifecycle, tray integration, dispatch and coordination |

Introduce stable IDs before adding more state:

- `EmojiFamilyId`: catalog-backed identity shared by compatible skin-tone variants. Build explicit variant mappings; do not assume removing modifiers always produces a valid base sequence. Preserve joiners and gender distinctions. Fix synthetic base entries retaining tone-specific labels.
- `ResultId`: discriminated identity for an emoji family or a saved combination. Never persist vector indices or pointers.
- `SearchResult`: result ID, display label, preview payload, match class, match quality, and explanation such as a matched alias.
- `PickerSession`: original target, query, selected result ID, ranking snapshot, and insertion attempt token. Keep this separate from persistent state.

Use one versioned UTF-8 `profile.tsv` under `%LOCALAPPDATA%\SwashMoji` for settings, usage, recency, query choices, aliases, pins, and later combinations. Define typed records and escaping for tabs, newlines, and backslashes. Use numeric bounds and record-size limits; skip malformed records with a recoverable diagnostic. Refuse to overwrite an unsupported newer format.

Write a complete temporary file in the same directory, flush/close it, then replace the profile atomically with a last-good backup. On first migration, import the existing settings/history/usage files and preserve them. Commit the profile version only after a successful write; rerunning migration must not double counts. Keep preferences usable in memory if persistence fails and surface the unsaved state.

Precompute normalized search fields and family indexes at load time. Keep storage writes out of the keystroke-to-results path. Update CMake to share the extracted logic with tests and expose a repeatable CTest invocation through the existing build workflow.

Acceptance: extraction preserves existing behavior, old profiles migrate once, corrupt/truncated input does not crash startup, and interrupted writes recover the last complete profile.

## Step 1 / M2: search by intent and personal vocabulary

### User experience

- Search English and Norwegian Bokmål together without switching modes: `takk`, `thanks`, and `thank you` all return useful choices.
- Support a curated set of feelings and situations such as `awkward`, `finally`, `well done`, and `oops`.
- A result context menu offers **Add alias** with the current query prefilled. The empty state offers **Teach this phrase**, opening an editor that lets the user choose the intended emoji and save the mapping.
- A tray command opens **My vocabulary** for editing and removing aliases. An alias points to a family or combination ID and may contain multiple words.
- Normalized alias phrases are unique. Editing an existing phrase makes replacement explicit; cancel leaves the old mapping intact.

### Implementation

Extend `tools/update_emoji_catalog.py` to merge English and Bokmål annotations from the pinned CLDR release, following locale inheritance and English fallback. Keep source versions, license information, and deterministic output. Add curated phrase mappings in a separate tracked data file so a catalog regeneration cannot erase them. Bundle the generated data; runtime search requires no network.

Normalize text consistently with Unicode-aware case conversion and normalization, collapsed whitespace, and defined punctuation handling. Preserve meaningful Norwegian characters such as æ, ø, and å. Test composed/decomposed input. Keep phrases intact as well as tokenized; apply English word-form rules only to English fields. Recognize a pasted known emoji directly before text tokenization.

Use this initial ordering of match classes:

1. Exact personal alias (an explicit instruction by the user).
2. Exact canonical or localized name, or exact known emoji glyph.
3. Exact curated intent phrase.
4. Name phrase prefix.
5. All-token name/keyword matches, keeping current name-versus-keyword quality distinctions.
6. Fuzzy fallback when there are no ordinary results.

Partial aliases and combination names use corresponding prefix/token classes. Avoid generic stop-word removal that destroys phrases. Empty searches use favorites/history, and unmatched searches show the teaching action without irrelevant filler results.

Acceptance: a checked-in corpus covers bilingual names, phrases, case, Norwegian letters, typos, punctuation, aliases, and ambiguous queries. Exact names remain reachable and prioritized except where the user deliberately assigns the same phrase as an alias. Creating, editing, and deleting an alias survives restart and preserves the original insertion target.

## Step 2 / M3: query learning and stable favorites

Implemented. The following policy is the current behavior; saved combinations
remain reserved for M5. Query records store most-recently-chosen pairs first,
with a maximum normalized query length of 256 UTF-16 code units. Reading/searching
does not refresh LRU order. Disabling learning stops both collecting and applying
query counts while retaining them for re-enabling. Recency/usage continues to work.
Explicit alias/favorite edits and sort/learning settings apply immediately;
clearing learned history also resets the open session's preference snapshot.

### Ranking policy

Within each match class, order by query-specific choice count, existing recency/usage preference, lexical detail, default popularity, and stable result ID. Query learning cannot promote an unrelated item into the results or cross match-class boundaries. An explicit alias is how users teach a relationship the catalog does not know.

Store bounded counts for `(normalized complete query, ResultId)`. Increase a count once for a fully submitted insertion or successful explicit copy. Do not learn from hovering, cancelled searches, failed insertion, or intermediate query prefixes. Saturate counters; initially retain up to 1,000 query records using least-recently-used eviction. Provide a setting to disable query learning and a command to clear learned history.

Snapshot preferences when the picker opens. Recompute matching when the query changes, but apply new usage/learning to ordering only in the next picker session. Preserve the selected result ID across font, tone, and layout changes when that result remains available.

### Favorites and tone families

- Allow up to ten pinned results. On an empty query they occupy a stable ordered prefix of the grid, followed by deduplicated recent/most-used results. Pins do not force unrelated results into a nonempty search.
- Use context-menu and keyboard-accessible pin/unpin actions; reorder through the vocabulary dialog with Move up/Move down controls.
- Aggregate legacy usage across mapped tone variants and retain the latest family recency. Global tone affects emitted glyphs; changing it must not reset ranking or duplicate history entries.
- Clearing learned history removes recency, counts, and query preferences. User-authored aliases, combinations, pins, and appearance settings remain independently editable.

Acceptance: repeated `nice` → 👌 choices improve its position within the eligible class; query-specific counts do not affect other queries, while ordinary recency/usage remains global; exact-name results stay ahead of learned keyword matches; pins remain stable; repeated insertions do not reorder an open picker; toned selections contribute to the same family history after restart. Automated and native integration checks pass; the interrupted visual check of favorites remains a follow-up.

## Step 3 / M4: confident selection

Next milestone. Resume the interrupted favorites visual/keyboard review alongside
the selection UI work. Preserve M3's learning boundaries, family IDs, ordered pins,
session snapshots, and the delivered Ctrl+Backspace behavior throughout.

### Visible behavior

- Replace the default font-heavy footer with the selected result's name and the relevant insertion/copy hints. Keep full text accessible when visual space requires ellipsis. Font and tone changes still show short temporary status messages.
- Show a larger preview after approximately 350 ms of deliberate hover. Keyboard selection updates the label immediately; a Details button/context-menu command opens the same preview explicitly. A hover preview does not steal focus or change the committed keyboard selection.
- Provide a keyboard-accessible variant chooser inside Details. A one-use variant selection overrides the global tone for that insertion; the global setting remains available through Alt+I.
- Offer only valid catalog variants, including mixed-tone sequences when available. Show supported full sequences rather than synthesizing arbitrary combinations.
- Keep empty-state actions, preview controls, and vocabulary editing within a predictable focus order.

### Proposed interaction changes

| Action | Behavior |
| --- | --- |
| Click or Enter | Insert selected result and close |
| Ctrl+click or Ctrl+Enter | Insert selected result and keep picker open |
| Shift+Enter | Copy selected result and close after successful copy |
| Tab / Shift+Tab | Move focus between search, results, and visible actions |
| Alt+F | Cycle emoji font, replacing the current Tab binding |
| Arrow keys in results | Move spatially through the visible grid |
| Left / Right in search | Edit the query normally |
| Ctrl+Backspace in text fields | Delete selected text or the previous word; retain undo (already implemented) |
| Down in search | Move focus into results |
| Esc | Close the active detail/editor first; otherwise dismiss picker and restore target |

Enter from the search field still inserts the current first/selected result. Preserve Alt+E, Alt+A, Alt+P, Alt+I, Alt+T, Alt+1–3, Alt+S, Ctrl+Backspace, and F1. Route shortcuts by focused control: an editor's Enter must save its form rather than insert into another application. Document the click and Tab changes visibly in release notes and help.

Use a logical row/column selection model independent of the Win32 multi-column listbox storage order. Hit-test clicks so empty grid space cannot insert the previous selection. Cover partial rows and page boundaries. Start by retaining the native owner-drawn listbox and supplying names through its item strings while drawing the corresponding glyph; assess accessibility before deciding whether a custom provider is required.

Acceptance: every workflow works with keyboard alone; preview and editor opening preserve the original external target; labels identify the exact emitted variant; font/tone changes preserve selection; results never shift during a click; 1–3 rows navigate spatially at incomplete-page boundaries.

## Step 4 / M1 and M4: reliability, DPI, and accessibility

### Insertion and copy recovery (M1)

Replace the boolean-style insertion flow with explicit outcomes: no target, focus failure, no input submitted, partial input submitted, and full input submitted. Capture the target when opening from an external application; internal windows must never become the destination.

Validate the target, prepare the full UTF-16 payload, and request/verify foreground activation before sending. If activation needs a deferred retry, use a bounded attempt tied to the session token and cancel when the user dismisses the picker or switches elsewhere. Handle relevant modifier states without leaving synthetic keys stuck. Use attempt tokens to cancel stale restore-focus timers and prevent an old callback from reopening or focusing the picker after the user has moved on.

On detectable failure, retain/reopen the picker with the same query and selection and offer **Copy instead**. Never retry a partially submitted payload automatically; it can duplicate text. Offer copy for a partial result with a clear message to check the destination first. Do not silently replace the clipboard.

For explicit copy, allocate and populate the payload before opening/emptying the clipboard. Check each operation and only close the picker or update history after success. Clipboard contention and allocation/set-data failures leave the selection available. A failure after emptying may already have changed the clipboard; report it accurately rather than claiming preservation.

Full `SendInput` submission means events entered the input stream, not proof that an application accepted text. UIPI can block input, and its cause is not identified by the return value. Use accurate internal outcome names and avoid a false delivery confirmation. Compatibility testing must validate actual target text. [Microsoft SendInput documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).

### Display and accessibility (M4)

Add a Per-Monitor V2 application manifest and scale layout, fonts, hit targets, and preview geometry from a consistent logical unit system. Handle `WM_DPICHANGED`, recreate DPI-dependent resources, and honor the suggested window bounds before clamping to the work area. Keep DirectWrite/GDI coordinate conversion explicit to prevent double scaling. Apply the same treatment to help and vocabulary windows. [Microsoft DPI-change documentation](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged).

Expose search labels, result names, selected state, variant names, and actionable recovery messages to accessibility clients. Validate native listbox exposure with Inspect and Narrator first; add a narrowly scoped UI Automation provider only for behavior the native control cannot expose. The results container needs selection semantics and its items need selectable-item semantics. Respect system high-contrast colors and provide visible focus. [Microsoft control patterns](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controlpatternsoverview).

Acceptance: automated state tests cover missing/destroyed targets, foreground denial, zero/partial input, clipboard failures, dismissal during a pending callback, and modifier restoration. Manual tests cover a controlled Win32 edit target, Notepad, browser text inputs/contenteditable, a Chromium-based desktop editor/chat app, and a terminal, recording actual versions and limitations. Test elevated targets as a known compatibility boundary without requiring the picker to run elevated. Validate 100%, 125%, 150%, and 200% scaling, mixed-DPI monitors, screen edges, Narrator, and high contrast.

## Step 5 / M5: saved combinations

Add a **New combination** action to My vocabulary. The native editor accepts a name/trigger and a sequence of 2–8 catalog emoji entries, with an ordered preview, Add, Remove, Move left, and Move right controls. Users choose explicit variants for each entry. Saving resolves and stores the exact emitted Unicode payload plus catalog references; global tone changes do not silently rewrite an authored combination.

Use a persistent generated combination ID so renaming does not lose history, pins, or aliases. Keep trigger uniqueness consistent with personal aliases. Reject empty names, invalid sequences, and over-limit entries with inline errors. The editor must remain usable with the keyboard and screen reader.

Results display a compact sequence preview and its name; Details shows the complete sequence. Insert the full stored payload in a single insertion attempt through the same recovery path. Record one selection of the combination, not artificial usage of every component. Copy follows the same rule. Combinations participate in aliases, query learning, favorites, recency, and usage ordering through `ResultId`.

Deleting a combination removes its dependent pins, aliases, and learned counts in the same profile transaction and explains affected aliases before deletion. Cancel makes no changes. Catalog updates must not alter a saved payload; missing component metadata falls back to the stored sequence and label.

Acceptance: `launch` → 🚀✨ and `please` → 🥺🙏 can be created, found, renamed, pinned, inserted, copied, and deleted. Ordering, tone variants, joiners, variation selectors, and surrogate pairs survive save/reload. A failed insertion preserves the entire combination and never automatically sends a second copy.

## M6: integrated validation and release

Existing baseline: eight passing suites, 111 corpus cases with 51 intent cases,
and controlled native insertion/editor checks. Extend this coverage for M4/M5;
do not treat it as completion of the release, application, monitor, accessibility,
or performance acceptance work below.

- Add focused CTest executables for search ranking, family mapping, personalization/storage migration, logical grid navigation, combinations, and insertion state transitions. Use injected platform-operation seams for deterministic failures and a small native target harness for real input delivery. Retain existing ranking tests.
- Add Python standard-library tests for deterministic catalog generation using local source fixtures, locale fallback, and curated phrase merging. Tests should not depend on a network download.
- Check a curated corpus of at least 100 queries spanning both languages, intent phrases, exact names, aliases, typos, and deliberate no-match cases. Label acceptable results explicitly. Require all exact-name/alias invariants and target at least 90% top-three coverage on the intent subset, reporting subset sizes.
- Establish a local baseline before tuning. Proposed budgets on a documented reference machine: p95 warm opening to interactive under 100 ms and query-to-results under 20 ms using the bundled catalog and populated bounded profile. These are targets, not claims about current speed.
- In scripted user trials, compare time from opening to accepted insertion, intended-result rank, and abandonment against the baseline. Define abandonment as dismissal without insertion/copy; inspect reasons so accidental openings do not distort the conclusion. Use test fixtures and consented trial notes rather than adding production telemetry.
- Update Norwegian README, F1 help, release notes, and catalog attribution together. Document changed click/Tab behavior, aliases, learning controls, pins, combinations, copy recovery, migration, and known target limitations. Ensure profile reset and recovery instructions are accurate.
- Build the portable release and verify it from a clean directory with bundled catalog resources, both fresh-profile and migrated-profile runs. Run CTest with failure output and the catalog tests. Record manual app/monitor/accessibility results separately from automated passes.

## Completion definition

All five steps are complete when the full path works together: open over an external text field, find by English/Norwegian intent or personal phrase, choose a stable favorite or learned result, inspect/select a variant or saved combination, insert/copy with predictable focus and recoverable failures, and retain the intended preferences after restart. Release only after each milestone's acceptance gate has evidence; simulated insertion success alone does not establish target-app compatibility.
