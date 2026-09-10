# Saved combinations (M5)

Open **My vocabulary → New combination / edit...**. Enter a name/trigger, search
for a catalog emoji, choose its explicit variant, and select **Add**. Build a
sequence of 2–8 entries with **Remove**, **Move left**, and **Move right**. The
ordered tiles serve as both editor and visual preview. **Save combination** applies the draft;
**Close**/Escape, **New**, or selecting another saved item discards unsaved edits.
All controls use native Tab navigation and label mnemonics.

Emoji names in results, variant choices and component details follow the tray's
**Languages...** preference. Authored combination names and exact payloads
are preserved; searching still uses all available languages.

The resizable editor shares the vocabulary window's dark surfaces, rounded
controls and focus outlines. The library remains a stable width while results
expand. Sequence tiles retain native list selection and full accessible names;
the arrow buttons move the selected emoji. Close sits in the global footer.
Empty lists have explicit empty states. Validation and
save failures remain inline, and draft edits show a muted unsaved status.

For example, save `launch` as 🚀✨ or `please` as 🥺🙏. Search their names in the
picker, then use the existing insertion/copy shortcuts. Names share the normalized
alias namespace. Up to 200 combinations are stored, with names up to 96 UTF-16
units. Validation and persistence errors appear inline. Failed disk writes retain
session changes and can be retried with Save.

Combination results use the unchanged grid: two small component previews, with an
ellipsis for longer sequences. The selected-name line identifies the combination;
Details (Alt+D or right-click) exposes its complete payload and ordered components.
Names remain available to accessibility through the native result list.

The authored payload is exact. Global tone changes and catalog updates never
rewrite it. Missing metadata falls back to stored glyphs and the combination name.
Renaming or editing retains the generated identity, aliases, favorites, usage and
query learning. Combinations join the existing ranking classes and session
snapshots; clearing learned history retains their definitions.

Insertion submits the entire payload in one attempt; explicit copying uses the
same payload. Only a completed operation learns one combination choice. Failure
retains the query, selected ID and complete payload; partial input is never retried
automatically. Deletion first shows dependent aliases in a scrollable confirmation.
Cancel changes nothing. Confirming deletes all dependent personalization in the
same profile save. See [profile format](profile-format.md) for version 4 migration.

## Verification

Visual redesign, 2026-09-09: `.\build.cmd test build-combination-final` passed all
11 registered CTest suites. Added native regression checks cover eight horizontal
tiles without clipping, resize growth/alignment, separated Save/Close actions,
empty search results and disabled Add/movement states. Existing tests still cover
validation, variants, ordering, renaming and discarded drafts.

The actual empty window was inspected, followed by a second polish pass for the
variant dropdown, placeholder contrast and library empty state. The populated
eight-emoji preview was then inspected at default and maximized sizes, including
the native dropdown popup, scrollbars and focus indicators. Real 125%, 150% and
200% monitor scaling, high-contrast visual inspection and Narrator remain manual
acceptance checks. Python generator tests were not registered in this environment.

`SwashMojiCombinationPreview.exe` is a dedicated isolated visual harness. It uses
`combination-preview-profile` beside the executable and starts with an in-memory
sample when no combinations exist. Pass `--empty` to omit that sample. Changes are
persisted only through the editor's explicit actions, never to the real profile.

Build and all 12 automated suites passed on 2026-09-08 with
`.\build.cmd test build-m5-check`. The added core and native editor suites
cover create/search/rename, normalized name collisions, bounds, ordered edits,
explicit variants, codec reload, missing metadata, tone isolation and cascading
deletion. The picker keyboard suite additionally exercises actual picker insertion
and copying with injected platforms: full payload, partial failure without retry,
recovery retention and one-result learning. Existing storage/insertion suites
continue to cover atomic persistence, migration and every partial event prefix.
These tests use isolated profiles and no real input or clipboard submission.

Computer Use visually inspected the native vocabulary and combination editor on
2026-09-08 and exposed named controls in UI Automation. It was stopped by the user
with physical Escape before the keyboard walkthrough, populated-picker preview,
and Details review. Those interactive checks, Narrator and actual target-app
insertion remain open for M6; automated adapter success is not delivery acceptance.

### Visual-polish verification — 2026-09-08

The editor resource now uses consistent compact margins, gaps, button heights and
aligned edges. Subtle native dividers distinguish the saved list, Add emoji flow,
and authored Sequence without changing control types or interaction semantics.
The right-hand workflow uses numbered native headings matching the supplied
design reference. Dialogs share the picker's dark background, inset surface and
text colors while retaining system-color fallback in high contrast.
The follow-up reference pass widens the two-pane layout, adds short muted workflow
hints, enlarges the saved list and payload preview, and includes the saved count in
the left heading. Search-within-saved and drag reordering shown by the reference
remain intentionally absent because they are not existing product behaviors.
`.\build.cmd test build-reference-dark` passed all 11 registered CTest suites.
The subsequent emoji-font correction applies DPI-scaled Segoe UI Emoji to result,
variant, sequence and preview controls; `.\build.cmd test build-reference-emoji`
also passed all 11 registered suites.
Because GDI still rendered that font monochrome, `build-color-panels` moves the
emoji-bearing rows and previews to native owner-draw controls backed by the same
DirectWrite color-font option as the picker. All 11 registered suites pass; the
test also verifies the required owner-draw styles are present.
Preview is a single-line read-only payload field beside the bottom actions; the
status line has its own row below it. The default button label was normalized from
`Sa&ve` to `&Save`.

`.\build.cmd test build-polish` passed all 11 CTest suites discovered by that
configuration. The native editor test additionally checks section ordering,
non-overlap, aligned actions, equal button heights, status separation, the Save
label, and the existing results-to-Save alias Tab transition. The separate picker
and vocabulary harness passed its dialog and 96/120/144/192-DPI layout assertions,
then reached the known final foreground check and failed with **Could not focus the
original app**; actual external insertion is not accepted.

The desktop connector exposed no native-app surface, so the rebuilt My vocabulary,
Combinations, picker and Details windows could not be visually inspected in this
pass. Real monitor transitions at 100%, 125%, 150% and 200%, high contrast,
keyboard focus indicators, clipping/ellipsis, Inspect/Narrator, and target-app
insertion therefore remain manual acceptance items. The standalone Python catalog
test was also unavailable because no `python` command is installed; CTest did not
register that optional twelfth suite.
