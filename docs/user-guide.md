# SwashMoji user guide

[Project overview](../README.md) · [Building and contributing](../CONTRIBUTING.md)

Press `Alt+E`, search for an emoji or phrase, and press `Enter` to insert the
selected result directly into the application you came from. Use the arrow keys
to choose another result.

## Startup and tray menu

SwashMoji needs no installation. Launch `SwashMoji.exe`; after a local build,
it is in the `build` directory. When moving it, keep `emojis.txt`,
`intent_phrases.tsv`, `LICENSE`, and `UNICODE_LICENSE.txt` beside the executable.
The catalog and phrases are read from the two UTF-8 files, with no network access.
Settings and history are stored separately in your user profile.

`Alt+E` opens the picker immediately after the active text cursor, falling back
to the mouse pointer when an application does not expose a text cursor.
Left-clicking the tray icon also opens it. Right-click the icon to configure
sorting, languages, **My vocabulary**, learning, and startup/shortcut settings. **Exit** quits the application; `Esc` closes the picker.

## Startup and shortcut settings

Right-click the tray icon and choose **Settings...**.

- **Start SwashMoji when I sign in to Windows** is off by default. Enabling it
  registers the current executable for your Windows user, without administrator
  access. Keep its data files beside it. If you move the folder, run the moved
  copy and save this setting again. Disable it before deleting the app.
- Choose the modifiers and letter, digit, or F1–F11 key for **Open picker shortcut**.
  At least Ctrl, Alt, or Win is required. **Reset to Alt+E** restores the default
  in the dialog; **Save** applies it. Windows-reserved or occupied shortcuts can
  be rejected; the previous working shortcut remains registered.
- **Close** discards changes you have not applied. A failed profile save keeps
  the new shortcut active for this session and the dialog explains how to retry.

The checkbox reflects SwashMoji's Windows startup registration, independently
of your profile. Windows **Startup apps** can separately disable that entry;
re-enable it there if necessary. SwashMoji does not override that Windows control.
Starting with Windows leaves SwashMoji in the tray until you open the picker.
The tray tooltip shows your configured shortcut. If it is unavailable at launch,
a warning explains how to open Settings or use the tray icon instead.

Throughout this guide, `Alt+E` means the default opening shortcut; substitute
your configured shortcut if you change it.

## Search and languages

Search English, Norwegian, German, Italian, French, and Spanish together. You can use names,
Unicode keywords, and phrases such as `bra jobbet` (well done), `på vei` (on my way),
and `thank you`. Both `Rakete` and `razzo` find 🚀. Typos in catalog names and
keywords are tolerated in your display languages when there are no regular matches.
Your own aliases give you more ways to find an emoji.

**Languages...** in the tray menu selects one or two languages for emoji names:

- **Primary** appears first.
- **Secondary** is optional; choose **None** to use only one language.
- You cannot select the same language twice. If you change the primary language
  to the previous secondary language, **Secondary** is set to **None**.
- **Save** saves the selection; **Close** discards unapplied changes.

The same dialog has an independent **Interface language** setting. Choose English,
Norwegian, German, Italian, French, or Spanish for SwashMoji's menus,
dialogs, buttons, and messages. The change is saved with the profile and applies
to subsequently opened UI; it does not restrict search or change the selected
languages used for emoji names. F1 help, application diagnostics and confirmation
messages use this language too. Native Windows file dialogs and standard message-box
buttons use the Windows language. A catalog-loading error before the profile can
be loaded uses the default English interface.

English and Norwegian are the defaults. **Italian**, **German**, **French**, and **Spanish** also have
translated names for all 3598 emoji variants in the catalog. Regular matching
still searches all available languages regardless of your display selection:
`ragazza` finds 👧 even with English and Norwegian selected. Display languages
are preferred when matches are otherwise equal; an exact name in another language
ranks above a weaker match in a selected language. Personal aliases and built-in
phrases retain phrase matching and typo tolerance regardless of the language
selection. Emoji-name preferences are independent of the interface language.

Names in results, previews, and variant choices use the same language preferences.
The selected variant's names appear below the results, separated by “·”. Identical
translations appear only once. Missing names are omitted without leaving a stray
separator; English is used as a fallback if neither selected translation exists.
Long names are visually truncated.

See [search behavior, language data, and alias rules](search.md) for technical details.

## Insertion, copying, and error handling

| Key or action | Result |
| --- | --- |
| `Enter` or click an emoji | Insert and close the picker |
| `Ctrl+Enter` or `Ctrl+click` | Insert and keep the picker open |
| `Shift+Enter` | Copy to the clipboard and close after successful copying |
| **Copy instead** / `Alt+C` | Copy the selection after an insertion failure |

Direct insertion leaves the clipboard unchanged. If insertion fails, the query
and selection are preserved, and you can choose **Copy instead**. Partial input
is never retried automatically. Check the target application before trying again.

If copying fails, the picker stays open. The message tells you if the clipboard
was already cleared before the failure. Only completed insertion or explicit
copying records a choice in history and learning.

See [insertion and error handling](insertion.md) for limitations and test coverage.

## Keyboard navigation, rows, and appearance

`Tab` and `Shift+Tab` move focus between search, results, and available actions.
Arrow keys and `Ctrl`+arrow keys navigate results directly, both before and after
you type. Search keeps focus so you can continue typing. Use `Shift`+arrow keys
or `Home`/`End` to edit the search text.

| Key | Action |
| --- | --- |
| `Ctrl+Backspace` | Delete the previous word or selected text in search and **My vocabulary** text fields |
| `Ctrl+Z` | Undo text deletion |
| `Alt+1`, `Alt+2`, `Alt+3` | Choose the maximum number of emoji rows |
| `Alt+F` | Cycle through installed color and monochrome emoji fonts |
| `Alt+K` | Cycle the global skin tone |
| `Alt+S` | Show or hide the selected result's text |
| `Esc` | Close the active dialog, help, or picker |
| `F1` | Show the feature and shortcut overview |

Results appear in one to three rows. The selected row count is an upper limit:
up to ten results fit in one row, and the window shrinks automatically. More
results expand the window again without changing the setting. With one row,
Up/Down selects the previous/next result; with multiple rows, they move the
selection within the same column. In the result list, `Page Up`/`Page Down` moves
ten columns, and `Home`/`End` moves to the first/last position.

Changing the skin tone, font, or row count preserves the selection. Search has
no label or placeholder, and shortcuts are not permanently displayed in the
picker. Font and skin-tone changes are shown briefly for 0.8 seconds. `Alt+F`
replaced the previous font shortcut, `Tab`; `Tab` now moves focus.

See [selection, navigation, and display scaling](selection.md) for details.

## Skin tones and variant details

`Alt+K` cycles the global skin tone for all compatible emoji. Skin-tone variants
share usage history, so they are not learned or counted as separate families.

**Details** (`Alt+D` or the result menu) shows a larger preview and valid catalog
variants, including mixed skin tones. **Use once** selects the variant for the
next successful insertion or copy without changing the global skin tone.
**Cancel** discards the draft.

## Aliases and learned phrases

Right-click a result and choose **Add alias**, or press `Alt+A`. The current
query is suggested as your own phrase. With no results, choose **Teach this phrase**
and find the emoji you want.

**My vocabulary** in the tray menu opens one window with tabs for aliases and
favorites, and for saved combinations. Use the first tab to create, edit, and delete aliases.
**Save alias** saves; **Close** discards the draft.

The window is resizable. The left panel contains saved aliases and pinned
favorites. In **Alias editor**, use **Search emoji** and **Choose an emoji** to
search and select from a single list with names in your display languages.
The alias and combination editors use the hint "Search emoji across languages"
because search includes all supported languages.
The selected row identifies the target for **Save alias** and **Pin favorite**.
The shared **Close** action is at the bottom of the window.

## Favorites and combinations

`Alt+P` or **Pin favorite** in the result menu pins the selected emoji as a favorite.
Up to ten favorites appear first when the query is empty. **My vocabulary** lets
you move them up/down or remove them.

The **Saved combinations** tab in **My vocabulary** saves sequences of 2–8 emoji
under a custom name, for example `launch` → 🚀✨. Choose each emoji's skin tone
and the sequence order before saving. Global skin-tone changes do not affect
saved sequences. The variant field appears only when the selected emoji has
multiple variants. Otherwise, **Add** uses the sole variant automatically.

The combination editor displays the sequence as horizontal emoji tiles. Arrow
buttons move the selected emoji, and **Save combination** saves. The shared **Close**
button closes the window and discards the draft. The saved-combination and result
lists adapt to the window size.

See [combinations](combinations.md) for editing, limits, and test status.

## Learning, sorting, and history

The picker learns which results you choose for the complete query. Turn learning
off with **Learn from searches** in the tray menu. New choices affect ranking
the next time the picker opens, so repeated insertion does not move results.

`Alt+T` switches between sorting by most recent use and total usage count.
You can also choose sorting in the tray menu, which shows the active mode.
Most recent use is the default. Previous choices and usage counts are stored locally.

See [learning, favorites, and stable results](learning.md) for ranking rules.

## Local data and backups

Settings and usage history are stored in
`%LOCALAPPDATA%\SwashMoji\profile.tsv`. On first launch, existing `settings.txt`,
`history.txt`, and `usage.txt` are imported automatically; the originals are kept.
Missing older files can be imported from `%LOCALAPPDATA%\WinMoji`.

Use **Settings... > Export profile...** to save a portable `.tsv` copy containing
settings, display languages, aliases, combinations, favorites, learning data, and
history. Windows startup registration is specific to the current computer and is
not included. Use **Import profile...** to replace the current profile after a
confirmation. SwashMoji validates the complete file before changing anything,
rejects damaged or unsupported profiles, and applies the imported activation
shortcut only if Windows can register it. A successful import uses the normal
atomic save and backup process.

The application saves through a temporary file and keeps the previous complete
profile as `profile.tsv.bak`. If the profile is incomplete, it attempts recovery
from the backup. Save failures appear in the status line and tray icon text;
your choices remain in memory. Unknown profile versions are not overwritten.

See [profile format and migration](profile-format.md) before editing the files
manually. Search names and keywords are based on Unicode CLDR 48.2 under the
[Unicode License v3](../third_party/UNICODE_LICENSE.txt).
[Catalog sources and updates](search.md#catalog-provenance-and-regeneration)
are documented separately.

## Compatibility and test status

Click/Tab behavior and DPI support are implemented, but full desktop, screen-reader,
and multi-monitor testing remains. Both controlled native insertion harnesses
passed on September 15, 2026. Real-application compatibility remains unverified.
Overall release acceptance is still open.

See [selection and DPI](selection.md) and the [release protocol](release-validation.md)
for documented results and remaining checks. Automated tests do not replace these checks.

## Letter variants

Press `Alt+I`, type a base letter, select a variant with the arrow keys, and press
`Enter` to insert it. For example, `Alt+I`, `u`, `Enter` inserts `ü` on a fresh
profile. `e` offers `é è ê ë ē ė ę ě ĕ`; `s` includes `ß`, and `a` includes `æ`.
Use Shift or Caps Lock for uppercase variants. Typing another letter replaces
the current base; Backspace clears it. Letters without variants show no results.

The letter picker has no placeholder or ordinary status line. Insertion errors
and **Copy instead** still appear when needed. `Alt+S` applies only to the emoji
picker.

Opening `Alt+I` with an empty field shows all supported variants, including
uppercase forms. `Alt+T` switches between **most recent** and **most used**, using
the same saved setting as the emoji picker. Typing `e` filters this list to `e`
variants without changing the ranking rules; clearing the field restores all
variants. Your most recent/frequent choice can become `Alt+I`, `Enter`.

Successful insertion or explicit copying updates the same local usage counts and
recent history in both views. Case is preserved. Most recent mode compares
recency first, then frequency; most used reverses that priority. Remaining ties
keep the built-in order (lowercase bases a–z, then uppercase A–Z globally).
Choices affect ranking next session, keeping repeat insertion stable. `Alt+T`
applies immediately while preserving the selected variant. **Learn from searches**
controls recording and applying variant history. Emoji history is separate.
`Shift+Enter`, `Ctrl+Enter`, and insertion recovery behave as in the emoji picker.
This first version has no variant pins.

`Alt+I` is reserved for this picker; global skin-tone cycling has moved to `Alt+K`.
If another program owns `Alt+I`, startup reports the registration failure.
