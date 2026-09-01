# SwashMoji

En liten, avhengighetsfri Win32 emoji-velger.

- `Alt+E` åpner velgeren på skjermen til det aktive vinduet.
- Høyreklikk systemstatusikonet for plassering, sortering og sletting av lokal brukshistorikk.
- Skriv for å søke på emoji-navn, Unicode-nøkkelord og vanlige aliaser; treff rangeres etter navn, relevans og brukshistorikk, og skrivefeil tolereres når det ikke finnes vanlige treff.
- Treff vises i én til tre emoji-rader; bruk `Alt+1`, `Alt+2` eller `Alt+3` for å velge antall rader, og piltastene for å flytte markeringen.
- Klikk en emoji for å sette den inn og gå tilbake til SwashMoji.
- `Enter` setter valgt emoji direkte inn i det aktive programmet uten å endre utklippstavlen.
- `Ctrl+Enter` setter inn valgt emoji og lar SwashMoji forbli åpen.
- `Shift+Enter` kopierer valgt emoji til utklippstavlen og lukker vinduet.
- `Esc` lukker vinduet.
- `F1` viser en komplett oversikt over funksjoner og hurtigtaster.
- `Tab` bytter til neste installerte fargefont eller monokrome emoji-font.
- `Alt+I` bytter global hudtone for alle kompatible emojier.
- En diskré statuslinje viser aktiv font og de viktigste tastene; etter `Tab` vises den nye fonten alene i 0,8 sekunder.
- `Alt+T` bytter mellom sortering etter sist brukt og totalt antall ganger brukt.
- Sorteringen kan også velges i systemstatusmenyen, som viser aktiv modus.
- Tidligere valg og bruksteller lagres lokalt i `%LOCALAPPDATA%\SwashMoji`; sist brukt er standard sortering.
- Brukshistorikken kan slettes fra systemstatusmenyen etter en bekreftelse.
- Ikonet i systemstatusfeltet åpner velgeren ved venstreklikk og har `Exit` ved høyreklikk.
- Emoji-katalogen leses fra UTF-8-filen `emojis.txt` ved siden av programfilen.

Bygg med den native Visual Studio 2022-verktøykjeden:

```powershell
.\build.cmd
```

Skriptet finner Visual Studio Build Tools automatisk. Ingen installasjon av SwashMoji
er nødvendig; kjør `build\SwashMoji.exe`.

Søkenavn og nøkkelord er basert på [Unicode CLDR 48.2](https://cldr.unicode.org/),
lisensiert under [Unicode License v3](https://www.unicode.org/license.txt). Katalogen kan
oppdateres reproducerbart med `tools\update_emoji_catalog.py`.
