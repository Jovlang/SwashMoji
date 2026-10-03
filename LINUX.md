# Linux port plan

Build a separate Linux frontend that shares SwashMoji's core, preserving the
Windows application and its behavior. Prove desktop integration before building
the full Linux UI. Runtime operation must remain offline.

This is a proposed implementation plan, not a claim of Linux support. No Linux
build or desktop compatibility checks have been performed for this plan.

## 1. Prove activation and insertion

- Test KDE Plasma and GNOME under Wayland, plus an X11 session. Record the exact
  distribution, desktop, compositor, and portal versions used.
- Prototype `Alt+E` and `Alt+I`, opening the picker, returning focus, explicit
  copying, and delivering Unicode text.
- On Wayland, evaluate the [Global Shortcuts portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.GlobalShortcuts.html)
  and permission-based [Remote Desktop input APIs](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.RemoteDesktop.html).
  Check capabilities at runtime. These APIs do not establish universal text
  insertion or destination identification.
- Evaluate an X11 backend separately, including shortcut conflicts, keyboard
  layouts, target identity, focus changes, and Unicode delivery.
- Provide a launcher or command that desktop shortcuts can invoke when global
  shortcut registration is unavailable.
- Never silently replace direct insertion with clipboard paste. Unsupported
  insertion must offer explicit copying and explain the limitation.

**Completion criterion:** a compatibility matrix showing which environments
support clipboard-preserving insertion, which require explicit copying, and
which aspects of focus restoration and picker positioning are available.

## 2. Make SwashMojiCore portable

The current CMake configuration rejects Linux and includes Windows adapters in
`SwashMojiCore`. Unicode normalization, profile writes, localization UI helpers,
and combination-ID generation also contain Windows dependencies.

- Separate platform adapters from reusable catalog, search, ranking, learning,
  picker-session state, localization tables, and profile logic. Keep UI
  coordination in the respective application frontend.
- Resolve Unicode representation explicitly. Linux `wchar_t` is typically
  32-bit, while existing code assumes UTF-16 in several places. Prefer explicit
  UTF-16 internally, preserving current length limits and conversions at Windows
  API boundaries. Keep disk formats UTF-8.
- Evaluate ICU for NFKC normalization, invariant lowercase conversion, and
  Unicode character classification. Verify compatibility against existing
  Windows behavior; do not substitute case folding without reviewing its effect
  on search and persisted normalized keys. See the
  [ICU normalization documentation](https://unicode-org.github.io/icu/userguide/transforms/normalization/).
- Abstract atomic file replacement and stable combination-ID generation.
- Split CMake targets into shared code, Windows adapters/frontend, and Linux
  adapters/frontend. Preserve the existing Windows build workflow.

**Completion criterion:** shared behavior tests pass on Linux and Windows with
identical catalog identities and representative search/ranking results. Preserve
the multilingual search policy, profile limits, and family identities described
in the existing technical documentation.

## 3. Build the Linux picker

Use Qt 6 Widgets as the proposed Linux UI dependency. It offers C++ integration,
standard controls, accessibility support, and a UI without a browser engine.
Validate dependency size, emoji rendering, and deployment requirements before
committing to the toolkit. Keep the Windows frontend native Win32.

- Implement emoji and letter pickers first, preserving keyboard navigation,
  compact rows, selection, stable session ranking, skin tones, and display
  languages.
- Preserve the letter picker's lack of a placeholder, ordinary status line, or
  reserved footer space. Insertion errors and **Copy instead** remain available.
- Preserve learning controls, separate emoji and letter histories, and immediate
  sort changes that retain selection.
- Add Details, aliases, favorites, saved combinations, languages, settings,
  profile import/export, and translated help.
- Reuse `Emoji::names`, locale metadata, and shared display-name formatting.
  Keep interface language independent of display languages and search scope.
- Include launcher and settings access independent of the tray. Tray behavior
  depends on the desktop, including GNOME limitations documented by
  [Qt](https://doc.qt.io/qt-6/qsystemtrayicon.html).

**Completion criterion:** feature and keyboard parity, with explicit platform
exceptions for activation, positioning, focus restoration, and insertion.

## 4. Add Linux platform services

- Use separate X11 and Wayland activation/input backends behind capability-based
  interfaces. The existing input abstraction models Windows targets and key
  events; do not assume it can represent every Linux backend unchanged.
- Preserve insertion failure semantics: retain query and selection, never retry
  partial input automatically, and record a choice only after completed
  submission or explicit copying. Distinguish submission from verified receipt
  by the destination application.
- Implement Linux clipboard ownership and lifetime behavior explicitly, keeping
  copied text available after the picker closes. Direct insertion must leave
  the clipboard untouched.
- Store profiles under `$XDG_DATA_HOME/SwashMoji`, falling back to
  `~/.local/share/SwashMoji`.
- Implement same-directory temporary writes, file synchronization, atomic rename,
  directory synchronization, backup recovery, and single-instance coordination.
  Preserve unsupported-version protection and failed-write diagnostics.
- Preserve version 9 profile compatibility where possible. Translate existing
  shortcut values at the platform boundary, including the Win/Super modifier.
  Account for compositor-managed shortcut bindings separately from requested
  profile preferences. Introduce a new profile version only when persistent
  semantics require it.
- Preserve combination IDs, exact payloads, and all learning data during profile
  import/export. Do not automatically import Windows directories on Linux.
- Make startup registration a separate per-user Linux setting, outside the
  profile, using the appropriate desktop integration.

**Completion criterion:** isolated storage and platform tests demonstrate safe
recovery, portable profile round-trips, and clear behavior when capabilities or
permissions are unavailable.

## 5. Verify and package

- Add Linux GCC/Clang CI while retaining Windows build and regression checks.
- Port shared tests and add focused coverage for Unicode boundaries,
  normalization parity, profile migration, backups, stable IDs, and filesystem
  failures. Keep desktop tests separate from ordinary CTest runs.
- Use isolated profiles for every test. Never access the user's real profile.
- Run desktop acceptance checks for exact emoji sequences, letter variants,
  keyboard layouts, held modifiers, focus changes, repeated insertion, clipboard
  preservation, mixed DPI, emoji fonts, and accessibility.
- Test representative native, browser, and Electron applications in each claimed
  desktop/session environment. Record denied or unavailable integration as
  unverified or unsupported rather than passed.
- Begin with one distribution package and a defined dependency baseline.
  Evaluate AppImage or Flatpak after desktop integration works; do not assume
  packaging removes compositor or permission limitations.
- Include the catalog, curated phrases, application license, Unicode license,
  and required dependency notices. Preserve deterministic offline catalog
  regeneration.
- Update `README.md`, `CONTRIBUTING.md`, the user guide, relevant technical
  documents, and release evidence with Linux commands and verified limitations.

**Completion criterion:** a reproducible Linux package, passing shared tests on
both platforms, and a published acceptance matrix for supported Linux desktops.

## First implementation milestone

Produce a small Linux activation/insertion prototype and its compatibility
matrix. This determines whether the Linux release can deliver SwashMoji's
central "open, choose, insert" experience before committing to the full frontend.
Only then finalize the support baseline and estimate the remaining work.
