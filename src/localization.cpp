#include "localization.h"
#include <iterator>
#include <map>
#include <cwctype>

namespace SwashMoji {
namespace {
using Table = std::map<std::wstring, std::wstring>;
#include "localization_supplement.h"
#include "localization_messages.h"
#include "localization_diagnostics.h"
#include "localization_help.h"
const std::map<std::string, Table>& Tables() {
    static const auto tables = [] {
    std::map<std::string, Table> result{
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
            ,{L"My vocabulary - SwashMoji", L"Mein Vokabular – SwashMoji"}, {L"Save phrases and emoji combinations for quick access.", L"Speichern Sie Ausdrücke und Emoji-Kombinationen für schnellen Zugriff."}, {L"Saved aliases", L"Gespeicherte Aliase"}, {L"Pinned favorites", L"Angeheftete Favoriten"}, {L"Alias editor", L"Alias-Editor"}, {L"Phrase", L"Ausdruck"}, {L"Search emoji", L"Emoji suchen"}, {L"Choose an emoji", L"Emoji auswählen"}, {L"Save alias", L"Alias speichern"}, {L"Pin favorite", L"Favorit anheften"}, {L"Unpin favorite", L"Favorit lösen"}, {L"Up", L"Nach oben"}, {L"Down", L"Nach unten"}, {L"Unpin", L"Lösen"}
            ,{L"+ New", L"+ Neu"}, {L"+ New combination / edit...", L"+ Neue Kombination / bearbeiten …"}, {L"Combinations - SwashMoji", L"Kombinationen – SwashMoji"}, {L"Create reusable emoji sequences triggered by a short phrase.", L"Erstellen Sie wiederverwendbare Emoji-Folgen mit einem kurzen Auslöser."}, {L"Saved combinations", L"Gespeicherte Kombinationen"}, {L"Combination editor", L"Kombinationseditor"}, {L"Name / trigger", L"Name / Auslöser"}, {L"Short phrase used to find this combination.", L"Kurzer Ausdruck zum Finden dieser Kombination."}, {L"Add emoji", L"Emoji hinzufügen"}, {L"Results", L"Ergebnisse"}, {L"Add", L"Hinzufügen"}, {L"Sequence", L"Reihenfolge"}, {L"2-8 emoji, in insertion order", L"2–8 Emojis in Einfügereihenfolge"}, {L"Remove", L"Entfernen"}, {L"Save combination", L"Kombination speichern"}
            ,{L"Search emoji across languages", L"Emoji in allen Sprachen suchen"}, {L"No pinned favorites yet", L"Noch keine angehefteten Favoriten"}, {L"Your saved phrases appear here", L"Ihre gespeicherten Ausdrücke erscheinen hier"}, {L"No saved combinations yet", L"Noch keine gespeicherten Kombinationen"}, {L"No emoji added yet", L"Noch keine Emojis hinzugefügt"}, {L"No matching emoji", L"Keine passenden Emojis"}, {L"Create one to get started.", L"Erstellen Sie eine, um zu beginnen."}, {L"Unsaved changes", L"Ungespeicherte Änderungen"}
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
            ,{L"My vocabulary - SwashMoji", L"Il mio vocabolario – SwashMoji"}, {L"Save phrases and emoji combinations for quick access.", L"Salva frasi e combinazioni di emoji per un accesso rapido."}, {L"Saved aliases", L"Alias salvati"}, {L"Pinned favorites", L"Preferiti fissati"}, {L"Alias editor", L"Editor alias"}, {L"Phrase", L"Frase"}, {L"Search emoji", L"Cerca emoji"}, {L"Choose an emoji", L"Scegli un’emoji"}, {L"Save alias", L"Salva alias"}, {L"Pin favorite", L"Fissa preferito"}, {L"Unpin favorite", L"Rimuovi preferito"}, {L"Up", L"Su"}, {L"Down", L"Giù"}, {L"Unpin", L"Rimuovi"}
            ,{L"+ New", L"+ Nuovo"}, {L"+ New combination / edit...", L"+ Nuova combinazione / modifica…"}, {L"Combinations - SwashMoji", L"Combinazioni – SwashMoji"}, {L"Create reusable emoji sequences triggered by a short phrase.", L"Crea sequenze di emoji riutilizzabili attivate da una breve frase."}, {L"Saved combinations", L"Combinazioni salvate"}, {L"Combination editor", L"Editor combinazioni"}, {L"Name / trigger", L"Nome / attivazione"}, {L"Short phrase used to find this combination.", L"Breve frase usata per trovare questa combinazione."}, {L"Add emoji", L"Aggiungi emoji"}, {L"Results", L"Risultati"}, {L"Add", L"Aggiungi"}, {L"Sequence", L"Sequenza"}, {L"2-8 emoji, in insertion order", L"2–8 emoji in ordine di inserimento"}, {L"Remove", L"Rimuovi"}, {L"Save combination", L"Salva combinazione"}
            ,{L"Search emoji across languages", L"Cerca emoji in tutte le lingue"}, {L"No pinned favorites yet", L"Nessun preferito fissato"}, {L"Your saved phrases appear here", L"Le frasi salvate appariranno qui"}, {L"No saved combinations yet", L"Nessuna combinazione salvata"}, {L"No emoji added yet", L"Nessuna emoji aggiunta"}, {L"No matching emoji", L"Nessuna emoji corrispondente"}, {L"Create one to get started.", L"Creane una per iniziare."}, {L"Unsaved changes", L"Modifiche non salvate"}
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
            ,{L"My vocabulary - SwashMoji", L"Mon vocabulaire – SwashMoji"}, {L"Save phrases and emoji combinations for quick access.", L"Enregistrez des phrases et combinaisons d’émojis pour un accès rapide."}, {L"Saved aliases", L"Alias enregistrés"}, {L"Pinned favorites", L"Favoris épinglés"}, {L"Alias editor", L"Éditeur d’alias"}, {L"Phrase", L"Phrase"}, {L"Search emoji", L"Rechercher un émoji"}, {L"Choose an emoji", L"Choisir un émoji"}, {L"Save alias", L"Enregistrer l’alias"}, {L"Pin favorite", L"Épingler le favori"}, {L"Unpin favorite", L"Désépingler le favori"}, {L"Up", L"Monter"}, {L"Down", L"Descendre"}, {L"Unpin", L"Désépingler"}
            ,{L"+ New", L"+ Nouveau"}, {L"+ New combination / edit...", L"+ Nouvelle combinaison / modifier…"}, {L"Combinations - SwashMoji", L"Combinaisons – SwashMoji"}, {L"Create reusable emoji sequences triggered by a short phrase.", L"Créez des séquences d’émojis réutilisables déclenchées par une courte phrase."}, {L"Saved combinations", L"Combinaisons enregistrées"}, {L"Combination editor", L"Éditeur de combinaison"}, {L"Name / trigger", L"Nom / déclencheur"}, {L"Short phrase used to find this combination.", L"Courte phrase utilisée pour trouver cette combinaison."}, {L"Add emoji", L"Ajouter un émoji"}, {L"Results", L"Résultats"}, {L"Add", L"Ajouter"}, {L"Sequence", L"Séquence"}, {L"2-8 emoji, in insertion order", L"2 à 8 émojis dans l’ordre d’insertion"}, {L"Remove", L"Retirer"}, {L"Save combination", L"Enregistrer la combinaison"}
            ,{L"Search emoji across languages", L"Rechercher des émojis dans toutes les langues"}, {L"No pinned favorites yet", L"Aucun favori épinglé"}, {L"Your saved phrases appear here", L"Vos phrases enregistrées apparaîtront ici"}, {L"No saved combinations yet", L"Aucune combinaison enregistrée"}, {L"No emoji added yet", L"Aucun émoji ajouté"}, {L"No matching emoji", L"Aucun émoji correspondant"}, {L"Create one to get started.", L"Créez-en une pour commencer."}, {L"Unsaved changes", L"Modifications non enregistrées"}
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
            ,{L"My vocabulary - SwashMoji", L"Mi vocabulario – SwashMoji"}, {L"Save phrases and emoji combinations for quick access.", L"Guarda frases y combinaciones de emoji para acceder rápidamente."}, {L"Saved aliases", L"Alias guardados"}, {L"Pinned favorites", L"Favoritos fijados"}, {L"Alias editor", L"Editor de alias"}, {L"Phrase", L"Frase"}, {L"Search emoji", L"Buscar emoji"}, {L"Choose an emoji", L"Elige un emoji"}, {L"Save alias", L"Guardar alias"}, {L"Pin favorite", L"Fijar favorito"}, {L"Unpin favorite", L"Desfijar favorito"}, {L"Up", L"Subir"}, {L"Down", L"Bajar"}, {L"Unpin", L"Desfijar"}
            ,{L"+ New", L"+ Nuevo"}, {L"+ New combination / edit...", L"+ Nueva combinación / editar…"}, {L"Combinations - SwashMoji", L"Combinaciones – SwashMoji"}, {L"Create reusable emoji sequences triggered by a short phrase.", L"Crea secuencias de emoji reutilizables activadas por una frase corta."}, {L"Saved combinations", L"Combinaciones guardadas"}, {L"Combination editor", L"Editor de combinaciones"}, {L"Name / trigger", L"Nombre / activador"}, {L"Short phrase used to find this combination.", L"Frase corta usada para encontrar esta combinación."}, {L"Add emoji", L"Añadir emoji"}, {L"Results", L"Resultados"}, {L"Add", L"Añadir"}, {L"Sequence", L"Secuencia"}, {L"2-8 emoji, in insertion order", L"2–8 emojis en orden de inserción"}, {L"Remove", L"Quitar"}, {L"Save combination", L"Guardar combinación"}
            ,{L"Search emoji across languages", L"Buscar emojis en todos los idiomas"}, {L"No pinned favorites yet", L"Aún no hay favoritos fijados"}, {L"Your saved phrases appear here", L"Tus frases guardadas aparecerán aquí"}, {L"No saved combinations yet", L"Aún no hay combinaciones guardadas"}, {L"No emoji added yet", L"Aún no se han añadido emojis"}, {L"No matching emoji", L"No hay emojis coincidentes"}, {L"Create one to get started.", L"Crea una para empezar."}, {L"Unsaved changes", L"Cambios sin guardar"}
        }}
    };
    const char* locales[]{"nb", "de", "it", "fr", "es"};
    for (const auto& row : kDialogTranslations)
        for (size_t i = 0; i < std::size(locales); ++i)
            result[locales[i]].emplace(row[0], row[i + 1]);
    for (const auto& row : kMessageTranslations)
        for (size_t i = 0; i < std::size(locales); ++i)
            result[locales[i]].emplace(row[0], row[i + 1]);
    for (const auto& row : kDiagnosticTranslations)
        for (size_t i = 0; i < std::size(locales); ++i)
            result[locales[i]].emplace(row[0], row[i + 1]);
    for (const auto& row : kHelpTranslations)
        for (size_t i = 0; i < std::size(locales); ++i)
            result[locales[i]].emplace(row[0], row[i + 1]);
    return result;
    }();
    return tables;
}
std::wstring StripMnemonic(std::wstring text) {
    std::wstring result;
    for (size_t p = 0; p < text.size(); ++p) {
        if (text[p] != L'&') result += text[p];
        else if (p + 1 < text.size() && text[p + 1] == L'&') { result += L'&'; ++p; }
    }
    return result;
}
std::wstring PreserveMnemonic(const std::wstring& original, const std::wstring& translated) {
    wchar_t key{};
    for (size_t p = 0; p + 1 < original.size(); ++p) {
        if (original[p] != L'&') continue;
        if (original[p + 1] == L'&') { ++p; continue; }
        key = original[p + 1]; break;
    }
    std::wstring result;
    bool assigned = false;
    for (const auto character : translated) {
        if (key && !assigned && towlower(character) == towlower(key)) {
            result += L'&'; assigned = true;
        }
        if (character == L'&') result += L'&';
        result += character;
    }
    if (key && !assigned) { result += L" (&"; result += key; result += L')'; }
    return result;
}
BOOL CALLBACK LocalizeChild(HWND child, LPARAM parameter) {
    const auto* locale = reinterpret_cast<const std::string*>(parameter);
    // Translate labels only. Edit/list contents can contain personal vocabulary
    // that happens to equal a translation key.
    wchar_t className[32]{};
    GetClassNameW(child, className, static_cast<int>(std::size(className)));
    if (_wcsicmp(className, L"STATIC") && _wcsicmp(className, L"BUTTON")) return TRUE;
    wchar_t text[1024]{};
    GetWindowTextW(child, text, static_cast<int>(std::size(text)));
    if (!*text) return TRUE;
    auto translated = UiText(*locale, StripMnemonic(text).c_str());
    if (translated != StripMnemonic(text)) SetWindowTextW(child, PreserveMnemonic(text, translated).c_str());
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
std::wstring UiDiagnostic(const std::string& locale, const std::wstring& english) {
    const auto exact = UiText(locale, english.c_str());
    if (exact != english || locale == "en") return exact;
    // Storage can join recovery and save diagnostics. Match whole known messages
    // and their separating spaces; never replace words inside arbitrary strings.
    std::wstring result;
    for (size_t position = 0; position < english.size();) {
        if (english[position] == L' ') { result += L' '; ++position; continue; }
        const wchar_t* matched = nullptr;
        size_t length = 0;
        for (const auto& row : kDiagnosticTranslations) {
            const auto size = wcslen(row[0]);
            if (size > length && english.compare(position, size, row[0]) == 0 &&
                (position + size == english.size() || english[position + size] == L' ')) {
                matched = row[0]; length = size;
            }
        }
        if (!matched) return english;
        result += UiText(locale, matched);
        position += length;
    }
    return result;
}
std::wstring UiHelpText(const std::string& locale) {
    std::wstring result;
    for (const auto& row : kHelpTranslations) {
        if (!result.empty()) result += L"\r\n\r\n";
        result += UiText(locale, row[0]);
    }
    return result;
}
std::wstring UiProfileFilter(const std::string& locale) {
    auto result = UiText(locale, L"SwashMoji profile (*.tsv)");
    result += L'\0'; result += L"*.tsv"; result += L'\0';
    result += UiText(locale, L"All files (*.*)");
    result += L'\0'; result += L"*.*"; result += L'\0'; result += L'\0';
    return result;
}
}
