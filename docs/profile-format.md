# Local profile format (M0–M2)

The runtime uses `%LOCALAPPDATA%\SwashMoji\profile.tsv`. All profile tests pass an
isolated directory explicitly; they never resolve or change the user's profile.

## Version 2

Files are UTF-8 with LF line endings. The reader also accepts a UTF-8 BOM and CRLF.
The header is `SwashMoji<TAB>2`. Following lines contain typed records:

| Record | Fields after the record type |
| --- | --- |
| `setting` | setting name, unsigned integer value |
| `recent` | exact emoji glyph, newest records first |
| `usage` | exact emoji glyph, nonzero unsigned 32-bit count |
| `alias` | display phrase, target kind (`emoji` or reserved `combination`), stable target ID |
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
(1–3), and `skin_tone` (0–5). Defaults match the previous application. Font and
status-line visibility remain session-only, as before M0.

Usage still stores exact glyphs. Family aggregation, query learning, pins, and
combinations belong to later milestones. M2 adds up to 500 aliases; phrases have
at most 96 UTF-16 code units and must normalize to at least one letter or digit.
The normalized phrase is the unique lookup key; the original phrase is kept for
display. Duplicate normalized records are skipped (first valid record wins).
Missing catalog targets are retained so they can be repaired/deleted in My
vocabulary; they do not produce search results. New targets must be existing
emoji-family IDs. Combination IDs are reserved for M5.

Introduce a new profile version when adding persistent record types so an older
executable cannot silently discard new data. Unknown versions are read-only.
The shared `ResultId` already distinguishes emoji-family IDs from combination IDs;
neither pointer values nor catalog indices are persisted.

## Migration and persistence

Version 1 profiles are read with settings, exact-glyph history, and counts intact,
then atomically upgraded to version 2. The previous complete version 1 file becomes
the backup. A failed write keeps version 1 on disk and reports unsaved state;
the next launch can retry. Unsupported future versions remain read-only.

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

Run `.\build.cmd test`, or `.\build.cmd test build-m0` while the normal executable
is running. CTest covers the original preference comparator, search behavior and
catalog identity, UTF-8/escaping, legacy migration, every truncation point of a
sample profile, backup recovery, future-format protection, malformed numeric
records, and real filesystem replacement failures using a locked test file.
