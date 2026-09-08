# Saved combinations (M5)

Open **My vocabulary → New combination / edit...**. Enter a name/trigger, search
for a catalog emoji, choose its explicit variant, and select **Add**. Build a
sequence of 2–8 entries with **Remove**, **Move left**, and **Move right**. The
ordered list and preview show insertion order. **Save** applies the draft;
**Close**/Escape, **New**, or selecting another saved item discards unsaved edits.
All controls use native Tab navigation and label mnemonics.

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
