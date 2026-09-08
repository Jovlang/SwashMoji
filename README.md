# SwashMoji

En liten, avhengighetsfri Win32 emoji-velger.

- `Alt+E` åpner velgeren på skjermen til det aktive vinduet.
- Høyreklikk systemstatusikonet for plassering, sortering og sletting av lokal brukshistorikk.
- Søk på engelsk og norsk bokmål samtidig: navn, Unicode-nøkkelord og uttrykk som `bra jobbet`, `på vei` og `thank you`. Skrivefeil tolereres når det ikke finnes vanlige treff.
- `Ctrl+Backspace` sletter forrige ord eller markert tekst i søkefeltet og tekstfeltene i **My vocabulary**. `Ctrl+Z` kan angre slettingen.
- Høyreklikk et treff og velg **Add alias**, eller bruk `Alt+A`. Søket fylles inn som forslag til din egen frase. Uten treff kan du velge **Teach this phrase** og finne ønsket emoji.
- **My vocabulary** i systemstatusmenyen lar deg opprette, endre og slette aliaser. **Save alias** lagrer; **Close** forkaster utkastet. Lagrede aliaser beholdes når brukshistorikken slettes.
- `Alt+P` eller **Pin favorite** i treffmenyen fester valgt emoji som favoritt. Opptil ti favoritter vises først uten søketekst; **My vocabulary** lar deg flytte dem opp/ned eller fjerne dem.
- Velgeren lærer hvilke treff du velger for hele søket. Læring kan slås av med **Learn from searches** i systemstatusmenyen. Nye valg påvirker rangeringen neste gang velgeren åpnes, slik at gjentatt innsetting ikke flytter treffene.
- Hudtonevariantene deler brukshistorikk. Bytte av hudtone, font og antall rader beholder markeringen.
- Treff vises i én til tre emoji-rader; bruk `Alt+1`, `Alt+2` eller `Alt+3` for å velge antall rader, og piltastene for å flytte markeringen.
- Valgt radantall er en øvre grense: opptil ti treff samles i én rad, og vinduet krymper automatisk. Flere treff utvider visningen igjen uten å endre innstillingen.
- Med én rad velger pil opp/ned forrige/neste treff. Med flere rader flytter de markeringen opp/ned i samme kolonne.
- Klikk en emoji for å sette den inn og lukke velgeren. `Ctrl+klikk` lar SwashMoji forbli åpen.
- `Enter` setter valgt emoji direkte inn i det aktive programmet uten å endre utklippstavlen.
- `Ctrl+Enter` setter inn valgt emoji og lar SwashMoji forbli åpen.
- `Shift+Enter` kopierer valgt emoji til utklippstavlen og lukker vinduet.
- Ved innsettingsfeil beholdes søket og markeringen. **Copy instead** (`Alt+C`)
  kopierer valget; delvis innsetting prøves aldri automatisk på nytt.
- Ved kopieringsfeil forblir velgeren åpen. Meldingen sier fra hvis utklippstavlen
  allerede ble tømt før feilen oppstod.
- `Esc` lukker vinduet.
- `F1` viser en komplett oversikt over funksjoner og hurtigtaster.
- `Tab` og `Shift+Tab` flytter fokus mellom søk, treff og tilgjengelige handlinger. Piltastene og `Ctrl`+piltaster navigerer treffene direkte, både før og etter at du skriver. Søket beholder fokus, slik at du kan fortsette å skrive. Bruk `Shift`+piltaster eller `Home`/`End` for å redigere søketeksten.
- `Alt+F` bytter til neste installerte fargefont eller monokrome emoji-font (tidligere `Tab`).
- **Details** (`Alt+D` eller treffmenyen) viser en større forhåndsvisning og gyldige varianter fra katalogen, også blandede hudtoner. **Use once** velger varianten for neste vellykkede innsetting eller kopiering uten å endre global hudtone. **Cancel** forkaster utkastet.
- Hold pekeren rolig over et treff i omtrent 350 ms for en forhåndsvisning uten å flytte tastaturmarkeringen.
- `Alt+I` bytter global hudtone for alle kompatible emojier.
- Under treffene vises bare navnet på valgt variant. Søket er uten etikett eller plassholder, og hurtigtastene vises ikke fast i velgeren. Font- og hudtonebytte vises midlertidig i 0,8 sekunder.
- `Alt+T` bytter mellom sortering etter sist brukt og totalt antall ganger brukt.
- Sorteringen kan også velges i systemstatusmenyen, som viser aktiv modus.
- Tidligere valg og bruksteller lagres lokalt i `%LOCALAPPDATA%\SwashMoji`; sist brukt er standard sortering.
- **Clear learned history** sletter nylige valg, brukstellere og lærte søk etter bekreftelse. Aliaser, favoritter og utseende beholdes.
- Ikonet i systemstatusfeltet åpner velgeren ved venstreklikk og har `Exit` ved høyreklikk.
- Emoji-katalogen og uttrykkene leses fra UTF-8-filene `emojis.txt` og `intent_phrases.tsv` ved siden av programfilen. Begge må følge med når programmet flyttes.

Bygg med den native Visual Studio 2022-verktøykjeden:

```powershell
.\build.cmd
```

Skriptet finner Visual Studio Build Tools automatisk. Ingen installasjon av SwashMoji
er nødvendig; kjør `build\SwashMoji.exe`.

M4 har endret klikk- og Tab-oppførselen og lagt til DPI-støtte. Full skrivebords-,
skjermleser- og skjermtesting gjenstår; se [endringsnotater](docs/release-notes.md)
og [teststatus for M4](docs/selection.md). Den siste native innsettingstesten fikk
avslag på fokusbytte til målprogrammet; M4 er ennå ikke ferdig godkjent.

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
M3-regler for læring, favoritter og stabile treff er beskrevet i `docs/learning.md`.

Søkenavn og nøkkelord er basert på [Unicode CLDR 48.2](https://cldr.unicode.org/),
lisensiert under [Unicode License v3](https://www.unicode.org/license.txt). Katalogen kan
oppdateres med `python tools\update_emoji_catalog.py --download --cldr-dir build\cldr`.
Gjenta uten `--download` for å bruke den lokale kildekopien. Se `docs/search.md`
for kildeversjoner, språkregler, aliasgrenser og tester. Kjør generatortestene
med `python tests\test_catalog_generator.py`; CTest inkluderer dem automatisk
når CMake finner Python 3.
