# M4 preview — selection and display changes

This build is pending desktop acceptance; see [verification limits](selection.md).

## Native editor visual polish — 2026-09-08

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

No product behavior, profile data, interactive control order,
accessibility implementation, or custom rendering changed in this pass. The
native layout regression test now checks workflow ordering, action alignment,
button sizing, status separation, and the Save label. See the verification
records in [selection.md](selection.md) and [combinations.md](combinations.md).

- **Click now inserts and closes.** Use Ctrl+click to keep the picker open.
- **Tab now moves focus.** Use Alt+F to cycle emoji fonts.
- The footer shows the selected emoji name and insertion/copy shortcuts.
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
