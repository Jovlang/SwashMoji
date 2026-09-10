# Saved combinations

Open **My vocabulary → New combination / edit...**. Enter a name/trigger, search
for a catalog emoji, choose its explicit variant, and select **Add**. Build a
sequence of 2–8 entries with **Remove**, **Move left**, and **Move right**. The
ordered tiles serve as both editor and visual preview. **Save combination** applies the draft;
**Close**/Escape, **New**, or selecting another saved item discards unsaved edits.
All controls use native Tab navigation and label mnemonics.
The variant field appears only when the selected emoji has multiple variants;
**Add** uses the sole variant automatically otherwise.

Emoji names in results, variant choices and component details follow the tray's
**Languages...** preference. Authored combination names and exact payloads
are preserved; search follows the preferred-language and fallback policy in
[search.md](search.md).

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

The core and native editor suites cover create/search/rename, normalized name
collisions, bounds, ordered edits, explicit variants, codec reload, missing
metadata, tone isolation, cascading deletion, discarded drafts and layout.
The picker keyboard suite checks complete payloads, partial failure without retry,
recovery retention and one-result learning through injected platforms. These tests
use isolated profiles and do not submit real input or clipboard content.

`SwashMojiCombinationPreview.exe` is an isolated visual harness. It uses
`combination-preview-profile` beside the executable and starts with an in-memory
sample when no combinations exist. Pass `--empty` to omit that sample. Changes
are persisted only through explicit editor actions, never to the real profile.

See [contributor workflow](../CONTRIBUTING.md) for builds and
[release validation](release-validation.md) for current results and outstanding
interactive, target-app, DPI and accessibility checks.
