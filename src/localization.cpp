#include "localization.h"
#include <iterator>
#include <map>

namespace SwashMoji {
namespace {
using Table = std::map<std::wstring, std::wstring>;
const std::map<std::string, Table>& Tables() {
    static const std::map<std::string, Table> tables{
        {"nb", {
            {L"English", L"Engelsk"}, {L"Norwegian", L"Norsk"}, {L"German", L"Tysk"}, {L"Italian", L"Italiensk"}, {L"French", L"Fransk"}, {L"Spanish", L"Spansk"},
            {L"Languages - SwashMoji", L"Språk – SwashMoji"}, {L"Interface language:", L"Grensesnittspråk:"},
            {L"Choose one or two languages for displayed emoji names.", L"Velg ett eller to språk for viste emojinavn."},
            {L"Primary:", L"Primært:"}, {L"Secondary:", L"Sekundært:"}, {L"None", L"Ingen"},
            {L"Search continues to use all available languages.", L"Søk bruker fortsatt alle tilgjengelige språk."},
            {L"Save", L"Lagre"}, {L"Close", L"Lukk"}, {L"Settings - SwashMoji", L"Innstillinger – SwashMoji"},
            {L"Import profile...", L"Importer profil …"}, {L"Export profile...", L"Eksporter profil …"},
            {L"Settings...", L"Innstillinger …"}, {L"Languages...", L"Språk …"}, {L"My vocabulary...", L"Mitt ordforråd …"},
            {L"Learn from searches", L"Lær av søk"}, {L"Exit", L"Avslutt"}, {L"Sort: Most recent", L"Sorter: Sist brukt"},
            {L"Sort: Most used", L"Sorter: Mest brukt"}, {L"Details...", L"Detaljer …"}, {L"Add alias...", L"Legg til alias …"},
            {L"Copy instead", L"Kopier i stedet"}, {L"Cancel", L"Avbryt"}, {L"Delete", L"Slett"}, {L"New", L"Ny"}
            ,{L"No matches. Teach this phrase (Alt+A)", L"Ingen treff. Lær denne frasen (Alt+A)"}
            ,{L"My vocabulary - SwashMoji", L"Mitt ordforråd – SwashMoji"}, {L"Save phrases and emoji combinations for quick access.", L"Lagre fraser og emojikombinasjoner for rask tilgang."}
            ,{L"Saved aliases", L"Lagrede aliaser"}, {L"Pinned favorites", L"Festede favoritter"}, {L"Alias editor", L"Aliaseditor"}
            ,{L"Phrase", L"Frase"}, {L"Search emoji", L"Søk etter emoji"}, {L"Choose an emoji", L"Velg en emoji"}, {L"Save alias", L"Lagre alias"}
            ,{L"Pin favorite", L"Fest favoritt"}, {L"Unpin favorite", L"Løsne favoritt"}, {L"Up", L"Opp"}, {L"Down", L"Ned"}, {L"Unpin", L"Løsne"}
            ,{L"New combination / edit...", L"Ny kombinasjon / rediger …"}, {L"Combinations - SwashMoji", L"Kombinasjoner – SwashMoji"}
            ,{L"Create reusable emoji sequences triggered by a short phrase.", L"Lag gjenbrukbare emojisekvenser som utløses av en kort frase."}
            ,{L"Saved combinations", L"Lagrede kombinasjoner"}, {L"Combination editor", L"Kombinasjonseditor"}, {L"Name / trigger", L"Navn / utløser"}
            ,{L"Short phrase used to find this combination.", L"Kort frase som brukes til å finne kombinasjonen."}, {L"Add emoji", L"Legg til emoji"}
            ,{L"Results", L"Resultater"}, {L"Variant", L"Variant"}, {L"Add", L"Legg til"}, {L"Sequence", L"Sekvens"}
            ,{L"2-8 emoji, in insertion order", L"2–8 emojier i innsettingsrekkefølge"}, {L"Move left", L"Flytt til venstre"}
            ,{L"Move right", L"Flytt til høyre"}, {L"Remove", L"Fjern"}, {L"Save combination", L"Lagre kombinasjon"}
            ,{L"Combination details - SwashMoji", L"Kombinasjonsdetaljer – SwashMoji"}, {L"Exact sequence", L"Nøyaktig sekvens"}
            ,{L"Emoji in insertion order", L"Emojier i innsettingsrekkefølge"}, {L"Delete combination", L"Slett kombinasjon"}
            ,{L"Affected aliases", L"Berørte aliaser"}, {L"Delete this combination, its favorites, learned history, and the aliases listed below?", L"Slette denne kombinasjonen, favorittene, den lærte historikken og aliasene nedenfor?"}
            ,{L"Unsaved changes", L"Ulagrede endringer"}, {L"Favorites saved.", L"Favoritter lagret."}, {L"Alias saved.", L"Alias lagret."}
            ,{L"Alias deleted.", L"Alias slettet."}, {L"Combination saved.", L"Kombinasjon lagret."}, {L"Combination deleted.", L"Kombinasjon slettet."}
            ,{L"No dependent aliases.", L"Ingen avhengige aliaser."}
            ,{L"+ New", L"+ Ny"}, {L"+ New combination / edit...", L"+ Ny kombinasjon / rediger …"}
            ,{L"Search emoji across languages", L"Søk etter emoji på alle språk"}
            ,{L"No pinned favorites yet", L"Ingen festede favoritter ennå"}, {L"Your saved phrases appear here", L"De lagrede frasene vises her"}
            ,{L"No saved combinations yet", L"Ingen lagrede kombinasjoner ennå"}, {L"No emoji added yet", L"Ingen emojier lagt til ennå"}
            ,{L"No matching emoji", L"Ingen samsvarende emojier"}, {L"Create one to get started.", L"Opprett en for å komme i gang."}
        }},
        {"de", {
            {L"English", L"Englisch"}, {L"Norwegian", L"Norwegisch"}, {L"German", L"Deutsch"}, {L"Italian", L"Italienisch"}, {L"French", L"Französisch"}, {L"Spanish", L"Spanisch"},
            {L"Languages - SwashMoji", L"Sprachen – SwashMoji"}, {L"Interface language:", L"Oberflächensprache:"},
            {L"Choose one or two languages for displayed emoji names.", L"Wählen Sie eine oder zwei Sprachen für Emoji-Namen."},
            {L"Primary:", L"Primär:"}, {L"Secondary:", L"Sekundär:"}, {L"None", L"Keine"},
            {L"Search continues to use all available languages.", L"Die Suche verwendet weiterhin alle verfügbaren Sprachen."},
            {L"Save", L"Speichern"}, {L"Close", L"Schließen"}, {L"Settings - SwashMoji", L"Einstellungen – SwashMoji"},
            {L"Import profile...", L"Profil importieren …"}, {L"Export profile...", L"Profil exportieren …"},
            {L"Settings...", L"Einstellungen …"}, {L"Languages...", L"Sprachen …"}, {L"My vocabulary...", L"Mein Vokabular …"},
            {L"Learn from searches", L"Aus Suchvorgängen lernen"}, {L"Exit", L"Beenden"}, {L"Sort: Most recent", L"Sortierung: Zuletzt verwendet"},
            {L"Sort: Most used", L"Sortierung: Meistverwendet"}, {L"Details...", L"Details …"}, {L"Add alias...", L"Alias hinzufügen …"},
            {L"Copy instead", L"Stattdessen kopieren"}, {L"Cancel", L"Abbrechen"}, {L"Delete", L"Löschen"}, {L"New", L"Neu"}
            ,{L"No matches. Teach this phrase (Alt+A)", L"Keine Treffer. Diesen Ausdruck lernen (Alt+A)"}
        }},
        {"it", {
            {L"English", L"Inglese"}, {L"Norwegian", L"Norvegese"}, {L"German", L"Tedesco"}, {L"Italian", L"Italiano"}, {L"French", L"Francese"}, {L"Spanish", L"Spagnolo"},
            {L"Languages - SwashMoji", L"Lingue – SwashMoji"}, {L"Interface language:", L"Lingua dell'interfaccia:"},
            {L"Choose one or two languages for displayed emoji names.", L"Scegli una o due lingue per i nomi delle emoji."},
            {L"Primary:", L"Primaria:"}, {L"Secondary:", L"Secondaria:"}, {L"None", L"Nessuna"},
            {L"Search continues to use all available languages.", L"La ricerca continua a usare tutte le lingue disponibili."},
            {L"Save", L"Salva"}, {L"Close", L"Chiudi"}, {L"Settings - SwashMoji", L"Impostazioni – SwashMoji"},
            {L"Import profile...", L"Importa profilo …"}, {L"Export profile...", L"Esporta profilo …"},
            {L"Settings...", L"Impostazioni …"}, {L"Languages...", L"Lingue …"}, {L"My vocabulary...", L"Il mio vocabolario …"},
            {L"Learn from searches", L"Impara dalle ricerche"}, {L"Exit", L"Esci"}, {L"Sort: Most recent", L"Ordina: Più recenti"},
            {L"Sort: Most used", L"Ordina: Più usate"}, {L"Details...", L"Dettagli …"}, {L"Add alias...", L"Aggiungi alias …"},
            {L"Copy instead", L"Copia invece"}, {L"Cancel", L"Annulla"}, {L"Delete", L"Elimina"}, {L"New", L"Nuovo"}
            ,{L"No matches. Teach this phrase (Alt+A)", L"Nessun risultato. Insegna questa frase (Alt+A)"}
        }},
        {"fr", {
            {L"English", L"Anglais"}, {L"Norwegian", L"Norvégien"}, {L"German", L"Allemand"}, {L"Italian", L"Italien"}, {L"French", L"Français"}, {L"Spanish", L"Espagnol"},
            {L"Languages - SwashMoji", L"Langues – SwashMoji"}, {L"Interface language:", L"Langue de l’interface :"},
            {L"Choose one or two languages for displayed emoji names.", L"Choisissez une ou deux langues pour les noms des émojis."},
            {L"Primary:", L"Principale :"}, {L"Secondary:", L"Secondaire :"}, {L"None", L"Aucune"},
            {L"Search continues to use all available languages.", L"La recherche utilise toujours toutes les langues disponibles."},
            {L"Save", L"Enregistrer"}, {L"Close", L"Fermer"}, {L"Settings - SwashMoji", L"Paramètres – SwashMoji"},
            {L"Import profile...", L"Importer le profil…"}, {L"Export profile...", L"Exporter le profil…"},
            {L"Settings...", L"Paramètres…"}, {L"Languages...", L"Langues…"}, {L"My vocabulary...", L"Mon vocabulaire…"},
            {L"Learn from searches", L"Apprendre des recherches"}, {L"Exit", L"Quitter"}, {L"Sort: Most recent", L"Tri : plus récents"},
            {L"Sort: Most used", L"Tri : plus utilisés"}, {L"Details...", L"Détails…"}, {L"Add alias...", L"Ajouter un alias…"},
            {L"Copy instead", L"Copier à la place"}, {L"Cancel", L"Annuler"}, {L"Delete", L"Supprimer"}, {L"New", L"Nouveau"}
            ,{L"No matches. Teach this phrase (Alt+A)", L"Aucun résultat. Apprendre cette phrase (Alt+A)"}
        }},
        {"es", {
            {L"English", L"Inglés"}, {L"Norwegian", L"Noruego"}, {L"German", L"Alemán"}, {L"Italian", L"Italiano"}, {L"French", L"Francés"}, {L"Spanish", L"Español"},
            {L"Languages - SwashMoji", L"Idiomas – SwashMoji"}, {L"Interface language:", L"Idioma de la interfaz:"},
            {L"Choose one or two languages for displayed emoji names.", L"Elige uno o dos idiomas para los nombres de emoji."},
            {L"Primary:", L"Principal:"}, {L"Secondary:", L"Secundario:"}, {L"None", L"Ninguno"},
            {L"Search continues to use all available languages.", L"La búsqueda sigue usando todos los idiomas disponibles."},
            {L"Save", L"Guardar"}, {L"Close", L"Cerrar"}, {L"Settings - SwashMoji", L"Configuración – SwashMoji"},
            {L"Import profile...", L"Importar perfil…"}, {L"Export profile...", L"Exportar perfil…"},
            {L"Settings...", L"Configuración…"}, {L"Languages...", L"Idiomas…"}, {L"My vocabulary...", L"Mi vocabulario…"},
            {L"Learn from searches", L"Aprender de las búsquedas"}, {L"Exit", L"Salir"}, {L"Sort: Most recent", L"Ordenar: Más recientes"},
            {L"Sort: Most used", L"Ordenar: Más usados"}, {L"Details...", L"Detalles…"}, {L"Add alias...", L"Añadir alias…"},
            {L"Copy instead", L"Copiar en su lugar"}, {L"Cancel", L"Cancelar"}, {L"Delete", L"Eliminar"}, {L"New", L"Nuevo"}
            ,{L"No matches. Teach this phrase (Alt+A)", L"Sin resultados. Enseñar esta frase (Alt+A)"}
        }}
    };
    return tables;
}
std::wstring StripMnemonic(std::wstring text) {
    for (size_t p = 0; (p = text.find(L'&', p)) != std::wstring::npos; ) text.erase(p, 1);
    return text;
}
BOOL CALLBACK LocalizeChild(HWND child, LPARAM parameter) {
    const auto* locale = reinterpret_cast<const std::string*>(parameter);
    wchar_t text[1024]{};
    GetWindowTextW(child, text, static_cast<int>(std::size(text)));
    if (!*text) return TRUE;
    auto translated = UiText(*locale, StripMnemonic(text).c_str());
    if (translated != StripMnemonic(text)) SetWindowTextW(child, translated.c_str());
    return TRUE;
}
}

std::wstring UiText(const std::string& locale, const wchar_t* english) {
    if (locale == "en") return english;
    const auto table = Tables().find(locale);
    if (table == Tables().end()) return english;
    const auto found = table->second.find(english);
    return found == table->second.end() ? english : found->second;
}
void LocalizeDialog(HWND dialog, const std::string& locale) {
    wchar_t title[1024]{};
    GetWindowTextW(dialog, title, static_cast<int>(std::size(title)));
    const auto localized = UiText(locale, title);
    SetWindowTextW(dialog, localized.c_str());
    EnumChildWindows(dialog, LocalizeChild, reinterpret_cast<LPARAM>(&locale));
}
}
