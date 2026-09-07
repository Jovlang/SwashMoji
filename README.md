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
- Ved innsettingsfeil beholdes søket og markeringen. **Copy instead** (`Alt+C`)
  kopierer valget; delvis innsetting prøves aldri automatisk på nytt.
- Ved kopieringsfeil forblir velgeren åpen. Meldingen sier fra hvis utklippstavlen
  allerede ble tømt før feilen oppstod.
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

Bygg og kjør automatiske tester med `.\build.cmd test`.
Hvis programfilen i `build` allerede kjører, bruk en separat
byggemappe: `.\build.cmd test build-m1`. Testene bruker egne midlertidige mapper
i byggemappen og berører ikke din lokale brukshistorikk.

Den separate testen `.\build-m1\SwashMojiNativeInputTests.exe` åpner et midlertidig
tekstfelt i en egen prosess og kontrollerer faktisk Unicode-innsetting. Den krever
et interaktivt skrivebord, endrer ikke utklippstavlen og lagrer resultatet i
`build-m1\native-input-result.txt`. Slipp modifikatortastene før testen kjøres.

Innstillinger og brukshistorikk lagres nå samlet i
`%LOCALAPPDATA%\SwashMoji\profile.tsv`. Ved første oppstart importeres eksisterende
`settings.txt`, `history.txt` og `usage.txt` automatisk; originalfilene beholdes.
Manglende eldre filer kan importeres fra `%LOCALAPPDATA%\WinMoji`.
Programmet lagrer gjennom en midlertidig fil og beholder forrige komplette profil
som `profile.tsv.bak`. Hvis profilen er ufullstendig, forsøkes gjenoppretting fra
sikkerhetskopien. Lagringsfeil vises i statuslinjen og i systemstatusikonets tekst;
valgene beholdes i minnet. Ukjente profilversjoner overskrives ikke.

Katalog, søk, personalisering og lagring ligger i det delte C++-biblioteket
`SwashMojiCore`. Se `docs/profile-format.md` for filformat og migreringsregler,
og `docs/insertion.md` for innsetting, feilhåndtering og testdekning.

Søkenavn og nøkkelord er basert på [Unicode CLDR 48.2](https://cldr.unicode.org/),
lisensiert under [Unicode License v3](https://www.unicode.org/license.txt). Katalogen kan
oppdateres reproducerbart med `tools\update_emoji_catalog.py`.
