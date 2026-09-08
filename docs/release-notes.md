# M4 preview — selection and display changes

This build is pending desktop acceptance; see [verification limits](selection.md).

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
