# SwashMoji

En liten, avhengighetsfri Win32 emoji-velger.

**`Alt+E` → søk etter en emoji eller frase → `Enter` setter den direkte inn.**
Skriv for eksempel `bra jobbet`, `på vei` eller `razzo`. Bruk piltastene for å
velge treff, uten å slippe tastaturet.

SwashMoji er et portabelt Windows-program med native Win32-grensesnitt.
Det virker uten nett, installasjon eller ekstra kjøretidsavhengigheter.

## Finn og gjenbruk det du mener

- **Fire søkespråk samtidig:** engelsk, norsk bokmål, tysk og italiensk.
  Naturlige fraser og toleranse for skrivefeil gjør det lettere å finne riktig emoji.
- **Dine egne uttrykk:** lag aliaser eller lær velgeren hva en frase skal bety.
- **Favoritter og kombinasjoner:** fest vanlige valg, eller lagre en sekvens som
  `launch` → 🚀✨ under ett navn.
- **Lærer av valgene dine:** tidligere bruk hjelper med å rangere treffene.
- **Hudtoner og varianter:** velg global hudtone eller se og velg en variant i
  **Details**. **Languages...** velger ett eller to språk for navnene som vises.

## Viktigste hurtigtaster

| Tast | Handling |
| --- | --- |
| `Alt+E` | Åpne velgeren |
| `Enter` | Sett inn uten å endre utklippstavlen |
| `Ctrl+Enter` | Sett inn og behold velgeren åpen |
| `Shift+Enter` | Kopier til utklippstavlen |
| `Esc` / `F1` | Lukk / vis hjelp |

## Bygg selv

Med Visual Studio 2022 C++ Build Tools installert, kjør fra prosjektmappen:

```powershell
.\build.cmd
```

Start `build\SwashMoji.exe`. Skriptet finner verktøykjeden automatisk.
Se [brukerveiledningen](docs/user-guide.md#start-og-systemstatusmeny) før du flytter programmet.

## Dokumentasjon

- [Brukerveiledning](docs/user-guide.md) – alle funksjoner, hurtigtaster og innstillinger.
- [Søk og språk](docs/search.md), [læring](docs/learning.md) og [kombinasjoner](docs/combinations.md) – detaljer og regler.
- [Bidra og utvikle](CONTRIBUTING.md) – bygging, tester og teknisk dokumentasjon.
- [Endringsnotater](docs/release-notes.md) og [utgivelsesstatus](docs/release-validation.md) – verifiserte resultater og gjenstående testing.

Søkenavn og nøkkelord bygger på Unicode CLDR, under
[Unicode License v3](third_party/UNICODE_LICENSE.txt).
