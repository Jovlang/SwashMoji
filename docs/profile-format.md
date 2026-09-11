# Local profile format

The runtime uses `%LOCALAPPDATA%\SwashMoji\profile.tsv`. All profile tests pass an
isolated directory explicitly; they never resolve or change the user's profile.

## Version 5

Files are UTF-8 with LF line endings. The reader also accepts a UTF-8 BOM and CRLF.
The header is `SwashMoji<TAB>5`. Following lines contain typed records:

| Record | Fields after the record type |
| --- | --- |
| `setting` | setting name, unsigned integer value |
| `display_languages` | one or two registered locale codes, primary first (for example `en<TAB>nb`) |
| `recent` | target kind, stable target ID; newest records first |
| `usage` | target kind, stable target ID, nonzero unsigned 32-bit count |
| `query` | normalized complete query, target kind, stable target ID, nonzero unsigned 32-bit count; most recently chosen pair first |
| `pin` | target kind, stable target ID; display order |
| `alias` | display phrase, target kind (`emoji` or `combination`), stable target ID |
| `combination` | generated ID, display name/trigger, exact payload, followed by 2–8 pairs of family ID and exact component payload |
| `end` | number of preceding records, excluding the header |

The final `end` record and its terminating newline are mandatory. They detect
truncated writes; they are not a checksum for arbitrary corruption. Files are
limited to 4 MiB, records to 16 KiB, and record counts to 50,000. At most 40 recent
entries are retained. Invalid records within a correctly framed profile are
skipped with a diagnostic. Incomplete framing triggers backup recovery instead
of accepting a partial history.

String fields escape backslash as `\\`, tab as `\t`, newline as `\n`, and carriage
return as `\r`. Unknown escapes, invalid UTF-8, and embedded NULs are rejected.
Counts use decimal digits without signs or suffixes; overflow is rejected.

Settings are `position_above_text_field` and `sort_by_usage` (0–1), `emoji_rows`
(1–3), `skin_tone` (0–5), and `learn_queries` (0–1, default 1). Font and
status-line visibility remain session-only, and are not persisted.

Display languages default to `en`, `nb`, including when migrating versions 1–4.
One locale selects a single display language. Empty, duplicate, unknown or more
than two locales invalidate the entire record; defaults remain unless an earlier
valid record was read. The first valid record wins. Codes come from the shared
locale registry and are written literally (currently `en`, `nb`, `de`, `it` and `fr`). Search locale
selection is independent and is not persisted by this feature. History clearing
retains display languages. Version 5 protects this new record from older writers.

History and usage now store family IDs; tone variants share counts and recency.
Target kinds are `emoji` or `combination`. Up to ten unique pins retain
their explicit order. Up to 1,000 unique `(query, target)` records retain their
most-recently-chosen order for LRU eviction. Queries have at most 256 UTF-16 code
units after normalization; punctuation-only queries are not learned. Reading or
searching does not refresh LRU order. Counts saturate at UINT32_MAX. Duplicate
history, usage, pin, and normalized query records are skipped (first valid wins).

There are up to 500 aliases; phrases have
at most 96 UTF-16 code units and must normalize to at least one letter or digit.
The normalized phrase is the unique lookup key; the original phrase is kept for
display. Duplicate normalized records are skipped (first valid record wins).
Missing catalog targets are retained so they can be repaired/deleted in My
vocabulary; they do not produce search results. New targets must be existing
emoji-family IDs or existing saved combination IDs.

Introduce a new profile version when adding persistent record types so an older
executable cannot silently discard new data. Unknown versions are read-only.
The shared `ResultId` already distinguishes emoji-family IDs from combination IDs;
neither pointer values nor catalog indices are persisted.

Combinations are bounded to 200 records, names to 96 UTF-16 code units, and
sequences to 2–8 entries. Each entry has a catalog family reference (up to 96 units)
and exact Unicode payload (up to 64 units); the complete payload is at most 512
units and must equal the concatenation of its entries. Invalid Unicode and
mismatched payloads are rejected. Missing catalog metadata does not invalidate an
authored sequence. The generated GUID persists through rename and sequence edits.
Normalized combination names and aliases share one unique namespace; first valid
record wins on conflicting decoded records. Deletion removes the combination,
its aliases, pins, history, usage and query counts in one atomic profile save.

## Migration and persistence

Versions 3 and 4 migrate without changing their typed targets, combinations or settings.
Versions 1 and 2 use exact-glyph `recent` and `usage` records; version 2 also has
aliases. On load, the catalog resolves known glyphs to stable families, adds their
usage counts with saturation, and keeps each family's newest history position.
Unknown IDs are preserved, and missing metadata never produces a fabricated base.
The mapping is idempotent and also handles catalogs that later recognize an ID.
Aliases and settings are preserved. The runtime passes its loaded catalog to
`ProfileStorage::Load`; codec-only consumers may omit that catalog.

Migration atomically writes version 5 and backs up the previous complete file.
A failed write leaves the previous format on disk, retains the migrated values
in memory, and reports unsaved state. Retrying does not double counts. Unsupported
future versions remain read-only.

When both the primary and backup profiles are absent, import each legacy
`settings.txt`, `history.txt`, and `usage.txt` from the SwashMoji directory. For a
missing file only, try the corresponding WinMoji directory. Do not copy, move,
or delete the legacy files. Seed missing usage counts for recent glyphs to one,
matching the old loader. An unreadable legacy file blocks persistent migration
rather than committing incomplete data. Successfully writing the versioned
profile is the migration marker; later launches do not reimport or add counts.

The storage adapter validates the serialized profile, writes and flushes a
uniquely named temporary file in the destination directory, closes it, then
replaces the destination with a same-directory write-through rename. Before
replacing a valid primary, it atomically saves that complete primary as
`profile.tsv.bak`. Temporary files left by an interrupted process are ignored on
load. Failed live writes clean up their own temporary files.

A missing or incomplete primary is recovered from a valid backup. The corrupt
primary is never rotated over the good backup. If both profiles are unreadable,
preserve them and run with in-memory defaults and a visible diagnostic. If a
write fails, retain the changed in-memory profile and retry on the next mutation.
No writes occur while the user types a search.

To inspect a broken profile, exit SwashMoji and preserve a copy of both profile
files before editing. To deliberately start fresh, move both profile files and
the three legacy files out of the SwashMoji data directory; also account for any
legacy WinMoji files, which otherwise remain eligible for import. The tray's
clear-history command is the normal way to reset history while retaining settings.

## Validation

Run `.\build.cmd test`, or `.\build.cmd test build-agent` while the normal executable
is running. CTest covers the original preference comparator, search behavior and
catalog identity, UTF-8/escaping, legacy migration, every truncation point of a
sample profile, backup recovery, future-format protection, malformed numeric
records, and real filesystem replacement failures using a locked test file.
