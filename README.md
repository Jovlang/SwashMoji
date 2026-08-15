# EmoPicker

En liten, avhengighetsfri Win32 emoji-velger.

- `Alt+E` åpner velgeren på skjermen til det aktive vinduet.
- Skriv for å filtrere på emoji-navn og nøkkelord (substring-søk).
- `Enter` kopierer valgt emoji til utklippstavlen og lukker vinduet.
- `Esc` lukker vinduet.
- `Ctrl+F` bytter til neste installerte fargefont eller monokrome emoji-font.
- Tidligere valg vises først og lagres i `%LOCALAPPDATA%\EmoPicker\history.txt`.
- Ikonet i systemstatusfeltet åpner velgeren ved venstreklikk og har `Exit` ved høyreklikk.
- Emoji-katalogen leses fra UTF-8-filen `emojis.txt` ved siden av programfilen.

Bygg med den native Visual Studio 2022-verktøykjeden:

```powershell
.\build.cmd
```

Skriptet finner Visual Studio Build Tools automatisk. Ingen installasjon av EmoPicker
er nødvendig; kjør `build\EmoPicker.exe`.
