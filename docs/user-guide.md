# Brukerveiledning for SwashMoji

[Til prosjektoversikten](../README.md) · [Bygging og utvikling](../CONTRIBUTING.md)

Trykk `Alt+E`, søk etter en emoji eller frase, og trykk `Enter` for å sette inn
valgt treff direkte i programmet du kom fra. Piltastene velger et annet treff.

## Start og systemstatusmeny

Ingen installasjon av SwashMoji er nødvendig. Start `SwashMoji.exe`; etter lokal
bygging ligger den i `build`-mappen. Ved flytting må `emojis.txt`,
`intent_phrases.tsv` og `UNICODE_LICENSE.txt` følge med ved siden av programfilen.
Katalogen og uttrykkene leses fra de to UTF-8-filene, uten nettforbindelse.
Innstillinger og historikk lagres separat i brukerprofilen, ikke ved programfilen.

`Alt+E` åpner velgeren på skjermen til det aktive vinduet. Venstreklikk på ikonet
i systemstatusfeltet åpner den også. Høyreklikk ikonet for plassering ved det
aktive tekstfeltet, sortering, språkvalg, **My vocabulary**, læring og sletting
av lokal brukshistorikk. **Exit** avslutter programmet; `Esc` lukker velgeren.

## Søk og språk

Søk på engelsk, norsk bokmål, tysk og italiensk samtidig. Du kan bruke navn,
Unicode-nøkkelord og uttrykk som `bra jobbet`, `på vei` og `thank you`.
`Rakete` og `razzo` finner begge 🚀. Skrivefeil tolereres når det ikke finnes
vanlige treff. Egne aliaser gir deg flere måter å finne en emoji på.

**Languages...** i systemstatusmenyen velger ett eller to språk for emojinavn:

- **Primary** vises først.
- **Secondary** er valgfritt; velg **None** for bare ett språk.
- Samme språk kan ikke velges to ganger. Hvis primærspråket settes til det
  tidligere sekundærspråket, blir **Secondary** satt til **None**.
- **Save** lagrer valget; **Close** forkaster endringer som ikke er brukt.

Engelsk og norsk er standard. **Italian** og **German** har også oversatte navn
for alle 3598 emojivariantene i katalogen. Søket bruker fortsatt alle tilgjengelige
språk, uansett hvilke du viser. Språkvalget gjelder emojinavn, ikke selve
grensesnittets menyer og knapper.

Navn i treff, forhåndsvisning og variantvalg bruker samme språkvalg. Under treffene
vises navnene på valgt variant, atskilt med «·». Like oversettelser vises bare én
gang. Manglende navn utelates uten et løst skilletegn; engelsk brukes som reserve
hvis ingen av de valgte oversettelsene finnes. Lange navn forkortes visuelt.

Se [søkemodell, språkdata og aliasregler](search.md) for tekniske detaljer.

## Sett inn, kopier og håndter feil

| Tast eller handling | Resultat |
| --- | --- |
| `Enter` eller klikk på en emoji | Sett inn og lukk velgeren |
| `Ctrl+Enter` eller `Ctrl+klikk` | Sett inn og behold velgeren åpen |
| `Shift+Enter` | Kopier til utklippstavlen og lukk etter vellykket kopiering |
| **Copy instead** / `Alt+C` | Kopier valget etter en innsettingsfeil |

Direkte innsetting endrer ikke utklippstavlen. Ved innsettingsfeil beholdes søket
og markeringen, og du kan velge **Copy instead**. Delvis innsetting prøves aldri
automatisk på nytt. Kontroller målprogrammet før du eventuelt prøver igjen.

Ved kopieringsfeil forblir velgeren åpen. Meldingen sier fra hvis utklippstavlen
allerede ble tømt før feilen oppstod. Bare fullført innsetting eller eksplisitt
kopiering registrerer et valg i historikk og læring.

Se [innsetting og feilhåndtering](insertion.md) for begrensninger og testdekning.

## Tastatur, rader og utseende

`Tab` og `Shift+Tab` flytter fokus mellom søk, treff og tilgjengelige handlinger.
Piltastene og `Ctrl`+piltaster navigerer treffene direkte både før og etter at du
skriver. Søket beholder fokus, slik at du kan fortsette å skrive. Bruk
`Shift`+piltaster eller `Home`/`End` for å redigere søketeksten.

| Tast | Handling |
| --- | --- |
| `Ctrl+Backspace` | Slett forrige ord eller markert tekst i søk og tekstfeltene i **My vocabulary** |
| `Ctrl+Z` | Angre tekstsletting |
| `Alt+1`, `Alt+2`, `Alt+3` | Velg maksimalt antall emoji-rader |
| `Alt+F` | Bytt til neste installerte fargefont eller monokrome emoji-font |
| `Alt+I` | Bytt global hudtone |
| `Alt+S` | Vis eller skjul teksten for valgt treff |
| `Esc` | Lukk aktiv dialog/hjelp eller velgeren |
| `F1` | Vis oversikt over funksjoner og hurtigtaster |

Treffene vises i én til tre rader. Valgt radantall er en øvre grense: opptil ti
treff samles i én rad, og vinduet krymper automatisk. Flere treff utvider vinduet
igjen uten å endre innstillingen. Med én rad velger pil opp/ned forrige/neste
treff; med flere rader flytter de markeringen i samme kolonne. I trefflisten
flytter `Page Up`/`Page Down` ti kolonner og `Home`/`End` til første/siste plass.

Bytte av hudtone, font og antall rader beholder markeringen. Søket har ingen
etikett eller plassholder, og hurtigtastene vises ikke fast i velgeren. Font- og
hudtonebytte vises midlertidig i 0,8 sekunder. `Alt+F` har erstattet den tidligere
fontsnarveien `Tab`; `Tab` brukes nå til fokusflytting.

Se [valg, navigasjon og skjermskalering](selection.md) for detaljene.

## Hudtoner og variantdetaljer

`Alt+I` bytter global hudtone for alle kompatible emojier. Hudtonevariantene deler
brukshistorikk, slik at de ikke lærer eller teller som separate familier.

**Details** (`Alt+D` eller treffmenyen) viser en større forhåndsvisning og gyldige
varianter fra katalogen, også blandede hudtoner. **Use once** velger varianten for
neste vellykkede innsetting eller kopiering uten å endre global hudtone.
**Cancel** forkaster utkastet. Hold pekeren rolig over et treff i omtrent 350 ms
for en forhåndsvisning uten å flytte tastaturmarkeringen.

## Aliaser og lærte fraser

Høyreklikk et treff og velg **Add alias**, eller bruk `Alt+A`. Søket fylles inn
som forslag til din egen frase. Uten treff kan du velge **Teach this phrase** og
finne ønsket emoji.

**My vocabulary** i systemstatusmenyen lar deg opprette, endre og slette aliaser.
**Save alias** lagrer; **Close** forkaster utkastet. Lagrede aliaser beholdes når
brukshistorikken slettes.

Vinduet kan endre størrelse. **Library** samler aliaser og festede favoritter,
mens **Alias editor** viser søketreff og valgt emoji med navn på valgte språk.
**Close** og kombinasjonsredigering ligger nederst i vinduet.

## Favoritter og kombinasjoner

`Alt+P` eller **Pin favorite** i treffmenyen fester valgt emoji som favoritt.
Opptil ti favoritter vises først uten søketekst. **My vocabulary** lar deg flytte
dem opp/ned eller fjerne dem.

**New combination / edit...** i **My vocabulary** lagrer sekvenser av 2–8 emojier
med eget navn, for eksempel `launch` → 🚀✨. Velg hudtone per emoji og rekkefølge
før du lagrer. Lagrede sekvenser påvirkes ikke av global hudtone.

Kombinasjonsredigeringen viser sekvensen som vannrette emoji-fliser. Pilknappene
flytter valgt emoji, **Save combination** lagrer, og **Close** nederst lukker
vinduet og forkaster utkastet. Bibliotek og treffliste tilpasser seg vindusstørrelsen.

Se [kombinasjoner](combinations.md) for redigering, grenser og teststatus.

## Læring, sortering og historikk

Velgeren lærer hvilke treff du velger for hele søket. Læring kan slås av med
**Learn from searches** i systemstatusmenyen. Nye valg påvirker rangeringen
neste gang velgeren åpnes, slik at gjentatt innsetting ikke flytter treffene.

`Alt+T` bytter mellom sortering etter sist brukt og totalt antall ganger brukt.
Sorteringen kan også velges i systemstatusmenyen, som viser aktiv modus.
Sist brukt er standard. Tidligere valg og brukstellere lagres lokalt.

**Clear learned history** sletter nylige valg, brukstellere og lærte søk etter
bekreftelse. Aliaser, kombinasjoner, favoritter, utseende og språkvalg beholdes.
Se [læring, favoritter og stabile treff](learning.md) for rangeringsreglene.

## Lokale data og sikkerhetskopi

Innstillinger og brukshistorikk lagres i
`%LOCALAPPDATA%\SwashMoji\profile.tsv`. Ved første oppstart importeres eksisterende
`settings.txt`, `history.txt` og `usage.txt` automatisk; originalfilene beholdes.
Manglende eldre filer kan importeres fra `%LOCALAPPDATA%\WinMoji`.

Programmet lagrer gjennom en midlertidig fil og beholder forrige komplette profil
som `profile.tsv.bak`. Hvis profilen er ufullstendig, forsøkes gjenoppretting fra
sikkerhetskopien. Lagringsfeil vises i statuslinjen og i systemstatusikonets tekst;
valgene beholdes i minnet. Ukjente profilversjoner overskrives ikke.

Se [profilformat og migrering](profile-format.md) før du endrer filene manuelt.
Søkenavn og nøkkelord bygger på Unicode CLDR 48.2 under
[Unicode License v3](../third_party/UNICODE_LICENSE.txt).
[Katalogens kilder og oppdatering](search.md#catalog-provenance-and-regeneration)
er dokumentert separat.

## Kompatibilitet og teststatus

Klikk-/Tab-oppførselen og DPI-støtten er implementert, men full skrivebords-,
skjermleser- og flerskjermtesting gjenstår. De senest dokumenterte native
innsettingstestene fikk avslag på fokusbytte; dette er ikke en godkjent test av
faktisk innsetting. M4 og den samlede utgivelsesgodkjenningen er fortsatt åpne.

Se [endringsnotater](release-notes.md), [teststatus for valg og DPI](selection.md)
og [utgivelsesprotokollen](release-validation.md) for dokumenterte resultater og
gjenstående kontroller. Automatiske tester erstatter ikke disse kontrollene.
