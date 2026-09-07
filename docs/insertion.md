# Insertion and recovery (M1)

`insertion.h/.cpp` contain testable input and clipboard operations and a one-shot
focus-return guard. `insertion_win32.h/.cpp` adapt those operations to Windows.
`main.cpp` owns the picker, recovery controls, timers, and foreground event hook.

## Behavior

Enter inserts and closes after full input submission. Ctrl+Enter and clicking
insert while leaving the picker open. Shift+Enter explicitly copies and closes
only after the entire copy operation succeeds. M1 preserves these existing
shortcuts and the current Tab/font behavior.

An insertion failure keeps the query, selected result, and picker available.
The recovery area explains the outcome and offers **Copy instead**, also
available through Alt+C. This area is visible even when the ordinary status line
is hidden. Failed clipboard operations also leave the picker open. Key-repeat
Enter messages cannot automatically repeat an unsuccessful insertion.

The original destination is captured from an external application's root window
when opening the picker. The root window, process ID, and thread ID are checked
again before input submission. SwashMoji's own windows, its help window, and shell
surfaces are never captured as new destinations.

## Input submission

Validate the full UTF-16 payload before touching focus. Empty strings, embedded
NULs, unpaired surrogates, and payloads longer than 4,096 UTF-16 units are rejected.
Prepare a single event batch before requesting activation. After a successful
activation request, send a bounded WM_NULL synchronization message to the target
queue (100 ms maximum), then recheck target identity and foreground ownership.
There are no automatic activation retries. If focus cannot be verified, submit
no text and offer explicit copying.

Input outcomes distinguish missing targets, invalid payloads, held Alt/Windows
modifiers, failed focus, zero events submitted, partial submission, and full
submission. A full SendInput return counts events added to the input stream; it
does not prove acceptance by an arbitrary application. UIPI can block injection
into elevated applications without identifying that cause in the return value.
[Microsoft SendInput documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).

Left/right Ctrl and Shift states are temporarily released and restored within
the same input batch, allowing repeated Ctrl+Enter while Ctrl stays held. Held
Alt or Windows keys produce an instruction to release them first; releasing
those modifiers synthetically could activate menus or the shell. A partial
submission is never retried. Cleanup submits only key-up events, including a
dangling Unicode key-up if needed. It never adds another text or modifier
key-down. The message asks the user to check the destination and release
Ctrl/Shift; cleanup failure is reported separately.

For keep-open insertion, the picker requests focus after 75 ms only if the same
attempt is still current, the picker remains visible, the target identity is
valid, and that target remains foreground. A foreground event hook invalidates
the request if the user switches away, including a switch away and back. Opening
the picker, copying, another insertion, dismissal, opening help/tray menus, or
destruction cancels the pending return. Timer IDs are unique per attempt, so a
queued callback from a killed timer cannot act on a later session. If a hook or
timer cannot be established, the picker stays available without forcing delayed
focus. [Microsoft KillTimer documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-killtimer).

## Clipboard ownership

Allocate, lock, populate, and unlock a movable Unicode buffer before opening or
emptying the clipboard. If allocation or opening fails, the existing clipboard
is untouched. If publishing fails after EmptyClipboard, report that it was
already cleared. If text was published but closing failed, report that text was
copied and retain the picker. Track close failures independently on other error
paths as well. The adapter retries closing when it is destroyed.

Windows owns the allocation only after successful SetClipboardData. Every other
path frees the caller-owned allocation. History is updated only after full input
submission or a completed explicit copy. Direct insertion does not access the
clipboard. [Microsoft SetClipboardData documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setclipboarddata).

## Verification

`.\build.cmd test build-m1` builds the application and runs four CTest suites.
The insertion suite uses fake platform adapters to verify missing/destroyed
targets, denied or changed focus, invalid Unicode, full submission, every partial
prefix of a multi-emoji batch with modifiers, failed cleanup, stale/cancelled
focus callbacks, switch-away-and-back behavior, and all clipboard ownership and
failure paths. It never changes desktop focus or the real clipboard.

`.\build-m1\SwashMojiNativeInputTests.exe` runs separately because it needs an
interactive desktop. It creates a temporary external Win32 edit target, verifies
the exact received UTF-16 string and repeat insertion, checks that the clipboard
sequence number did not change, and rejects internal/stale/destroyed targets.
It exits with 77 when modifiers are held or foreground permission is unavailable.
It writes PASS, FAIL, or SKIPPED to `native-input-result.txt` beside the test
executable. The target has a 15-second crash-safety timeout and is closed by the
driver. No real profile data is used.

Verified on 2026-09-07: all four CTest suites passed; the native test passed on the
interactive desktop. The restricted shell session did not grant foreground
activation; the interactive launch was needed for actual delivery verification.
Broader application testing (Notepad, browsers, desktop chat/editor apps, terminals,
and elevated applications), the complete picker UI walkthrough, and M4's DPI and
accessibility matrix remain outstanding. The controlled edit test establishes
native text delivery, not universal application compatibility.
