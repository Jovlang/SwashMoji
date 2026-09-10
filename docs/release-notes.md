# M6 release candidate — verification and final UI

This build is pending release acceptance; see the [M6 evidence and checklist](release-validation.md).

## Languages — 2026-09-10

- Italian and German CLDR names and keywords now cover all 3,598 catalog variants.
  Choose **Italian** or **German** in Languages, or search directly in
  either language. Existing English/Norwegian catalog data is unchanged.
  All 13 suites pass in `build-i18n-translations`, including exact-name coverage
  for all four languages; Italian/German vocabulary rendering was inspected.

- The tray's **Languages...** dialog selects a primary and optional
  secondary language. Names across picker, vocabulary and combinations share this
  preference; search still uses all available languages. Defaults remain English
  + Norwegian, with missing/duplicate translations omitted and English fallback.
- Profile version 5 persists the selection and migrates earlier profiles with
  backups, preserving vocabulary and combination payloads. Older builds treat
  version 5 as read-only.
- All 13 CTest suites passed in `build-i18n-settings`, including Python generator
  tests. The native preferences dialog was visually inspected. Desktop editor
  checks passed, but Windows denied foreground focus for real insertion; DPI,
  high contrast and Narrator acceptance remain outstanding.

## Final editor and picker layout — 2026-09-09

- My vocabulary has a resizable Library and Alias editor, bilingual results and
  a selected-emoji card. Save alias saves; Close discards an unfinished draft.
- Combinations uses a saved library, bilingual search, integrated variant selection
  and eight horizontal sequence tiles. The tiles are the editor preview. Save
  combination and status sit below sequence actions; Close is in the global footer.
  The previous numbered stages and separate editor Preview section were removed.
- The selected-result footer shows English · Norwegian, with missing names omitted
  and long names visually shortened. It does not show permanent shortcut hints.
  F1 contains the shortcut guide; Details still displays the complete payload.
- Release tooling requires the offline Python suite, stages only the executable
  and three runtime data/license files, verifies ZIP contents and records hashes,
  source state, machine details and populated-profile latency samples.

## Selection shortcuts

- **Click now inserts and closes.** Use Ctrl+click to keep the picker open.
- **Tab now moves focus.** Use Alt+F to cycle emoji fonts.
- Enter inserts; Ctrl+Enter keeps the picker open; Shift+Enter explicitly copies.
  Direct insertion leaves the clipboard untouched. Failures preserve the query and
  selection; Alt+C copies instead. Partial input is never automatically retried.
- Details (Alt+D) offers catalog variants for one successful insertion/copy.
  Plain arrows and Ctrl+arrows navigate results while search retains focus;
  Shift+arrows and Home/End edit the query. Ctrl+Backspace retains native undo.

Profiles remain local and offline. Clear learned history retains aliases,
combinations, pins and appearance. See [backup recovery and deliberate profile
reset](profile-format.md#migration-and-persistence), [insertion limitations](insertion.md)
and [Unicode source attribution](search.md#catalog-provenance-and-regeneration).

## Historical native editor polish — 2026-09-08 (superseded layout)

- My vocabulary now separates its two management lists from a compact, aligned
  alias workflow: Phrase, Find emoji, Matching emoji, preview, and actions.
- Saved aliases and pinned favorites retain independent list/action areas with
  consistent button heights, gaps, and edge alignment.
- Combinations now separates Add emoji from the authored Sequence with subtle
  native dividers and numbered stages. Name, search, results, variant, sequence
  controls, preview, and bottom actions follow a consistent compact rhythm.
- The combination action row no longer competes with the sequence controls or
  status text. The default action now uses the conventional `&Save` resource
  label, correcting the awkward `Sa&ve` mnemonic rendering.
- Details uses the same button height and eight-dialog-unit action gap. Picker
  geometry and behavior are unchanged.
- My vocabulary, Combinations and Details now use the picker's dark background,
  inset-surface and text palette through native control theming. High contrast
  disables the dark theme and falls back to system colors.
- A follow-up reference pass gives Combinations a wider two-pane composition,
  concise muted guidance beneath each stage, a taller saved-items area, stronger
  sequence spacing, a larger payload preview, and a live saved-combination count.
  The reference-only saved-search and drag behavior were not added.
- Emoji-bearing editor controls now use a dedicated DPI-scaled Segoe UI Emoji
  font instead of inheriting the general dialog font. This covers search results,
  variants, sequences, favorites and previews while ordinary labels remain in
  Segoe UI.
- The follow-up color-glyph correction owner-draws only those native emoji-bearing
  controls through DirectWrite with color-font support. Accessible item strings,
  selection, scrolling, keyboard navigation and control types remain native.
- Existing aliases now prefill the editable Find emoji field with the selected
  emoji's searchable name instead of its glyph. This avoids the final monochrome
  GDI rendering site while preserving the native edit control, caret and undo.

These are historical intermediate layouts. The final layout is described above.
See the verification records in [selection.md](selection.md) and
[combinations.md](combinations.md).

- **Click now inserts and closes.** Use Ctrl+click to keep the picker open.
- **Tab now moves focus.** Use Alt+F to cycle emoji fonts.
- The footer shows the selected emoji name; F1 documents insertion/copy shortcuts.
- Short result sets collapse to one row; the picker expands up to the preferred
  row count as more results appear. Arrow navigation follows the displayed layout.
- Details (Alt+D) previews full catalog variants and offers a one-use selection.
  Brief hovering previews a result without changing keyboard selection.
- Plain arrows and Ctrl+arrows navigate results while search retains focus, including after typing.
  Shift+arrows and Home/End edit text. Grid navigation handles partial
  pages without dropping results. Empty-space clicks do not insert.
- Per-Monitor V2 scaling, named result strings, system high-contrast colors and
  scrollable native help improve the display/accessibility foundation.

Aliases, favorites, query learning, Ctrl+Backspace and existing profile format are
preserved. No migration is introduced in M4.

## M5: saved combinations

My vocabulary now supports named sequences of 2–8 explicit catalog variants,
ordered editing, rename and cascading deletion. Search, aliases, favorites,
learning and insertion share stable combination IDs. Profile version 4 preserves
exact Unicode payloads independently of global tone and catalog changes. The
picker retains its compact layout; Details shows the full sequence.

See [M5 behavior and verification](combinations.md). Interactive keyboard,
populated-picker/Details, screen-reader and target-app acceptance remains open.
