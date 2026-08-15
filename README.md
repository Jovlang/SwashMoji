# WinMoji

En liten, avhengighetsfri Win32 emoji-velger.

- `Alt+E` åpner velgeren på skjermen til det aktive vinduet.
- Skriv for å filtrere på emoji-navn og nøkkelord (substring-søk).
- `Enter` kopierer valgt emoji til utklippstavlen og lukker vinduet.
- `Esc` lukker vinduet.
- `Ctrl+F` bytter til neste installerte fargefont eller monokrome emoji-font.
- Tidligere valg vises først og lagres i `%LOCALAPPDATA%\WinMoji\history.txt`.
- Ikonet i systemstatusfeltet åpner velgeren ved venstreklikk og har `Exit` ved høyreklikk.
- Emoji-katalogen leses fra UTF-8-filen `emojis.txt` ved siden av programfilen.

Bygg på Windows med enten Visual Studio eller CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Ingen installasjon er nødvendig; kjør `build\Release\WinMoji.exe`.

## Bygg fra WSL

Installer krysskompilatoren én gang:

```bash
sudo apt-get update
sudo apt-get install mingw-w64 ninja-build
```

Bygg en 64-bit Windows-programfil:

```bash
cmake -S . -B build-mingw -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/windows-x64-mingw.cmake
cmake --build build-mingw
```

Resultatet er `build-mingw/WinMoji.exe`. Åpne det fra Windows Explorer eller kjør
`/mnt/c/.../WinMoji.exe` fra WSL; programmet bruker Windows' globale hurtigtast og
utklippstavle, så det må kjøre i Windows (ikke i WSLg).
