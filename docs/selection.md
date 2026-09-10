# Selection, Details and display scaling

For the complete workflow and shortcuts, see the [user guide](user-guide.md).
This document describes selection and rendering rules; remaining desktop acceptance
checks are tracked in [release validation](release-validation.md).

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

## Verification

The picker suite checks grid permutations for 0–125 results at 1–3 rows, spatial
edges, partial pages and catalog-only mixed variants. The separate picker/vocabulary
harness covers Details cancellation/commit, target/query retention, one-use failure
retention/consumption, empty-space clicks, caret behavior and controlled layout at
96/120/144/192 DPI. Simulated layout checks do not establish mixed-monitor behavior.

`SwashMojiPickerPreview.exe` opens an isolated picker seeded with two favorites;
`SwashMojiVocabularyPreview.exe` opens the editor preview. Both use test profiles
beside the executable. Exit the picker preview through its tray Exit command.
Do not run desktop input tests while a user is typing or holding modifiers.

See [contributor workflow](../CONTRIBUTING.md) for builds and
[release validation](release-validation.md) for current results and the remaining
visual, keyboard, hover, target-app, DPI and accessibility acceptance checklist.
