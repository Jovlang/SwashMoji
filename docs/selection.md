# Selection, Details and display scaling (M4)

Implementation is present; the full M4 acceptance gate is still open. See the
verification record below before treating this as release-ready.

For the complete user workflow and shortcut guide, see the
[Norwegian user guide](user-guide.md). This document describes selection rules,
implementation details and historical verification.

## Keyboard and pointer behavior

The saved row count is a maximum: up to ten matches use one row, eleven to
twenty use up to two, and larger sets use up to three. The window shrinks and
expands with the results, retaining the preference and using the displayed row
count for navigation and hit testing. Empty searches with no results retain one
row for the teaching action.

Click or Enter inserts and closes. Ctrl+click or Ctrl+Enter keeps the picker open.
Shift+Enter copies and closes only after success. A click must start and finish
on the same real result; empty grid space never inserts the previous selection.

Tab and Shift+Tab cycle through search, results and visible teaching or
recovery actions. Plain arrows and Ctrl+arrows immediately navigate results both
before and after typing. Search keeps focus so further typing refines the query. Shift+arrows
and Home/End retain native query-editing behavior.
Result arrows move spatially, Page Up/Down move ten columns, and Home/End reach
the first/last slot. In a one-row layout, Up/Down select the previous/next result.
Alt+F replaces the old Tab font shortcut. Other existing
Alt shortcuts and Ctrl+Backspace/undo remain available. Escape closes an active
dialog or help first; otherwise it dismisses the picker and requests focus for
the still-valid original target. Vocabulary uses Save alias as its default button.

The picker shows only an unlabelled, empty search control, the grid, and a
compact selected-name line using the exact variant's catalog names and the
**Languages...** display preferences (English · Norwegian by default). Missing
or duplicate names are omitted; [search documentation](search.md) defines fallback.
Combination names are unchanged. The line retains its
full accessible text when ellipsized. Alt+S can hide it. Font/tone messages and
failure recovery remain available, but there are no permanent shortcut hints or
Details button. Right-click a result or press Alt+D for Details; F1 retains help.
The normal window is 44 logical pixels shorter without changing grid geometry,
ordering, search, or navigation. All ten CTest suites passed. Visual checking was
attempted, but the preview exposed no targetable window and the user stopped
Computer Use with physical Escape; the visual acceptance check remains open.

`picker.cpp` maps ranked results into the native listbox's column-major slots.
Complete pages read left-to-right across ten columns. Incomplete final pages
balance real items across columns without synthetic empty items; every ranked
result appears exactly once. Navigation uses physical row/column coordinates
instead of assuming adjacent ranking indices are spatial neighbors.

## Preview and variants

A deliberate hover of roughly 350 ms opens a larger, nonactivating preview.
Moving away, scrolling, changing results, opening a dialog or losing activation
cancels it. Hover does not alter selection or the original insertion target.

Details (Alt+D or context menu) opens an accessible native modal dialog.
The variant list contains complete catalog sequences from the selected family,
including available mixed-tone sequences. Selecting a row previews that exact
sequence. Use once commits a pending payload; Cancel discards the draft.

The pending variant survives font/row/tone changes and failed insertion/copy.
It clears after a successful insertion/copy, a changed query or a new/dismissed
session. Global tone is never rewritten by Use once. Learning records the family
once after success, using the existing frozen session preferences. Details does
not directly send input; Enter there confirms Use once.

## DPI and accessibility

`app.manifest` enables Per-Monitor V2 and version 6 common controls. The picker
scales logical dimensions and fonts from 96 DPI. `WM_DPICHANGED` recreates resources,
applies suggested bounds and clamps to the work area. Direct2D uses explicit
96-DPI render-target coordinates with physical-pixel bounds/font sizes to avoid
double scaling. See [Microsoft's DPI-change contract](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged).

Details and vocabulary use the PMv2 native dialog manager. Help now uses a
scrollable, read-only native text control and its own DPI-dependent font instead
of inaccessible painted text. Picker rendering uses system colors in high
contrast and retains the native focus rectangle.

Results store localized names as native listbox strings while drawing glyphs.
Search is an unlabelled native edit; Details variants and recovery messages use native controls.
Selection changes emit an accessibility selection event and recovery emits an
alert event. No custom UI Automation provider has been added; native result
Selection/SelectionItem exposure still needs direct validation with Inspect and
Narrator. See [Microsoft's control-pattern overview](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controlpatternsoverview).

## Verification record — 2026-09-08

- `build.cmd test build-m4`: all nine CTest suites pass, including the offline
  Python generator tests. The new picker suite checks grid permutations for
  0–125 results at 1–3 rows, spatial edges, partial pages and catalog-only mixed
  variants. Existing search, storage, learning and insertion suites still pass.
- The separate `SwashMojiPickerVocabularyTests.exe` reached its final native
  insertion assertion after passing Details cancellation/commit, target/query
  retention, one-use failure retention/consumption, empty-space clicks, search
  caret behavior and layout/item-height checks at 96/120/144/192 DPI. Those are
  controlled layout checks, not proof of real mixed-monitor behavior.
- That final insertion reported **Could not focus the original app**. Actual
  external text delivery is therefore not a passing M4 result. Investigate with
  an interactive foreground launch; do not bypass the production focus guard.
- Computer Use inspected the isolated vocabulary dialog: the visible controls
  fit on the current display, and named editable fields, buttons and selectable
  result items appeared in its accessibility tree. The favorites list was empty
  in that preview, so the outstanding favorites visual review remains open.
- The user stopped Computer Use with physical Escape before picker/Details
  visual review. Computer Use was not resumed after the stop.

Outstanding: picker and Details visual/keyboard review, favorites ordering review,
hover timing/focus observation, Inspect/Narrator result-selection semantics,
high-contrast review, real 100/125/150/200% and mixed-monitor transitions, edge
placement, and actual insertion into the planned Win32/Notepad/browser/Chromium
editor/terminal matrix (including the elevated-target boundary). Record actual
app versions and observed delivery; simulated input success does not satisfy it.

The 2026-09-08 native-editor polish pass did not change picker geometry, drawing,
focus routing, accessibility events or high-contrast handling. Its `build-polish`
run passed all 11 registered CTest suites, and the separate native harness again
passed the picker/editor and simulated 96/120/144/192-DPI checks before failing its
known final external foreground assertion. Computer Use exposed no native-app
surface for the rebuilt previews, so the visual and real-monitor items above remain
open; source inspection and automated geometry checks are not visual acceptance.

`build-m4/SwashMojiPickerPreview.exe` opens an isolated picker seeded with two
favorites; `SwashMojiVocabularyPreview.exe` opens the separate editor preview.
Both use test profiles under the build directory, not the user's profile. Exit
the picker preview through its tray Exit command. Do not run desktop input tests
while a user is typing or holding modifiers.
