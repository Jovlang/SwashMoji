# Real-application compatibility attempt — 2026-09-15

## Candidate and environment

- Candidate: `build-release-0.3/SwashMoji.exe`, matching the executable hash in
  `build-release-0.3/release-20260914-172848-b3ac1ae7/manifest.json`.
- SHA-256: `CCEA7E930E47721D09F14E1A00AE25A3FD2DEC4F1A107C5257070225B66CABCD`.
- Notepad: 11.2607.14.0. A new empty document was opened and its editor inspected.
- Brave: 153.1.95.101, from the installed executable's product version.
- Tester: Codex through the Windows computer-use tool.

## Outcome: blocked setup, no compatibility pass

The user's existing picker was stopped with permission to test an isolated
candidate and restore the original executable afterward. The shell-launched
candidate did not appear in the computer-use window inventory after Alt+E from
Notepad. This does not establish an application insertion failure.

Inspection also found that `LoadProfile` uses `SHGetFolderPathW` with
`CSIDL_LOCAL_APPDATA`. An overridden `LOCALAPPDATA` environment variable alone
is therefore not sufficient evidence of profile isolation. The candidates were
stopped before any insertion or explicit copying was performed. Future packaged
application tests must verify the resolved profile directory, or use a dedicated
Windows test account.

The existing `SwashMojiPickerPreview.exe`, which explicitly constructs an isolated
profile directory, was then launched through computer use. Its result file read
`RUNNING`, but the tool reported "launched app did not expose a targetable window".
A refreshed window inventory also contained no preview window. No input tests
were attempted through guessed window handles or coordinates.

| Check | Result |
| --- | --- |
| Notepad Enter insertion and exact received Unicode | Not run; picker not targetable |
| Ctrl+Enter repeat and focus return | Not run |
| Clipboard preservation and explicit copy/paste | Not run |
| Browser input, textarea and contenteditable | Not run; offline fixture prepared under the generated compatibility directory |
| Chromium desktop editor round trip | Not run |
| Failure recovery and elevated target boundary | Not run |
| Terminal | Not run; requires a manual tester or a separately permitted test harness |

No target-application acceptance row is closed by this attempt. The earlier
controlled Win32 harness passes remain separate evidence. Next steps are to
establish a targetable isolated picker, then record exact received text, focus,
clipboard preservation, and recovery behavior for each application.
