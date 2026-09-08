# Learning, favorites and stable picker sessions (M3)

Successful insertion or explicit copy records one choice of a stable result ID.
For a nonempty normalized search, it also records the `(complete query, result)`
pair. Typing, hovering, cancelling, failed/partial insertion, and incomplete copy
operations do not learn. `nice` has two curated choices, 👌 and 👍, so repeated
choices can improve either within the eligible match class.

Within each match class, search compares query-specific counts, the selected
recent/most-used preference, lexical detail, default popularity, and stable ID.
Learning cannot introduce an unrelated result or cross a match-class boundary.
Exact names still lead keyword/intent matches; an explicit personal alias retains
the highest priority. Query counts affect only the normalized whole query, not
its prefixes or unrelated searches. Ordinary usage/recency still applies globally.

`Learn from searches` in the tray is enabled by default. Turning it off stops
collecting and applying query counts; existing counts remain stored for re-enabling.
Recency and usage continue working. `Clear learned history` removes recency, usage,
and all query counts after confirmation, preserving aliases, favorites and settings.
There is no network service: the profile remains on this PC.

## Stable sessions

Opening the picker takes a copy of ranking preferences. Search recomputes matching
as the query changes but reads usage, recency and query choices from that snapshot.
Successful choices are saved immediately and take effect in the next session.
Repeated keep-open insertion and the delayed return callback do not start a new
session. Font, tone and row changes preserve the selected ID while it remains
available. A changed query starts at its best result.

Explicit edits to aliases, favorites and sort/learning settings take effect
immediately. Clearing learned history also clears the open session's snapshot.
Opening the vocabulary dialog preserves the original external insertion target.

## Favorites and families

Use `Alt+P` or the result context menu to pin/unpin the selection. Up to ten pins
form an ordered prefix on an empty search. The rest of the list follows the
recent/most-used preference, deduplicated by family. Pins do not boost nonempty
searches or force unrelated emoji into them.

`My vocabulary` has a favorites list with Up, Down and Unpin controls. The target
chooser also has Pin favorite/Unpin favorite, independent of saving an alias.
Pin edits are saved immediately. Unavailable stored targets can be unpinned.

Emoji history and usage use catalog family IDs, so selecting a different supported
skin tone does not create another history entry or reset its rank. The global tone
changes emitted glyphs for compatible families. A pasted exact variant retains
that payload while recording usage against its family. Profile version 3 migrates
older exact-glyph counts and recency without changing aliases or appearance settings.
See [profile-format.md](profile-format.md) for bounds, LRU ordering and recovery.

## Verification

Run `.\build.cmd test build-m3-edit`. Eight CTest suites cover the bilingual corpus,
exact-name invariants, query-class boundaries, full-query isolation, disabled
learning, 1,000-pair LRU eviction, saturation, snapshot stability, pin limits and
ordering, family aggregation, restart, and locked-file migration retries. The
edit-control suite verifies Ctrl+Backspace deletion, selection/caret handling,
Unicode safety, repeat behavior and undo in native text fields. All eight suites
passed on 2026-09-08. The corpus passes 111/111 top-three/no-match cases, including
51/51 intent cases.

The separate `SwashMojiPickerVocabularyTests.exe` uses an isolated profile and
the real picker/editor code. It verifies favorite ordering and persistence,
cancelled alias edits, rejected/partial input and failed clipboard outcomes,
once-per-success learning, stable selection across repeat insertion and layout
changes, and next-session ranking. Its injected clipboard tests do not access
the system clipboard. A final real insertion into a temporary external Win32 edit
checks the original target and unchanged clipboard sequence number. The executable
writes `picker-vocabulary-result.txt` next to itself and then exits.

`SwashMojiVocabularyPreview.exe` opens a separate test profile for keyboard and
visual checks. These native tools need an interactive desktop; ordinary CTest does
not change focus or submit input. The final M3 favorites visual review was
interrupted when Computer Use was stopped and remains outstanding for M4.
Broader application compatibility, DPI/monitor and screen-reader verification
remains in M4/M6.
