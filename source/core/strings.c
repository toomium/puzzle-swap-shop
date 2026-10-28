#include "core/strings.h"

#include <stdio.h>

static const char *const s_de[STR_COUNT] = {
    [STR_APP_NAME] = "Puzzle Swap Shop",
    [STR_APP_TAGLINE] = "Fehlende Teile gegen Spielmünzen",
    [STR_UNOFFICIAL] = "Inoffizielle Homebrew. Nicht von Nintendo.",

    [STR_BTN_BACK] = "Zurück",
    [STR_BTN_QUIT] = "Beenden",
    [STR_BTN_OPEN] = "Öffnen",
    [STR_BTN_BUY] = "Kaufen",
    [STR_BTN_CANCEL] = "Abbrechen",
    [STR_BTN_CONTINUE] = "Weiter",
    [STR_BTN_OK] = "OK",
    [STR_BTN_CREATE_BACKUP] = "Sicherung anlegen",
    [STR_BTN_BACKUPS] = "Sicherungen",
    [STR_BTN_TO_BACKUPS] = "Zu den Sicherungen",
    [STR_BTN_CHANGE] = "Ändern",
    [STR_BTN_SELECT_ALL] = "Alle fehlenden",
    [STR_BTN_RESTORE] = "Wiederherstellen",
    [STR_BTN_KEEP] = "Behalten",
    [STR_BTN_VIEW_ONLY] = "Nur ansehen",

    [STR_TITLE_ONBOARDING] = "Bevor es losgeht",
    [STR_TITLE_GALLERY] = "Puzzle-Galerie",
    [STR_TITLE_PICKER] = "Teile auswählen",
    [STR_TITLE_CONFIRM] = "Kauf bestätigen",
    [STR_TITLE_BOUGHT] = "Gekauft",
    [STR_TITLE_BACKUPS] = "Sicherungen",
    [STR_TITLE_SETTINGS] = "Einstellungen",

    [STR_ONBOARDING_BODY] = "Wir sichern zuerst deine Mii-Plaza-Daten und deine Spielmünzen auf "
                            "der SD-Karte. So lässt sich jeder Kauf wieder rückgängig machen.",
    [STR_FILE_MEET] = "Mii-Plaza-Spielstand",
    [STR_FILE_COINS] = "Spielmünzen",
    [STR_BACKUP_WORKING] = "Bitte warten …",
    [STR_BACKUP_FAILED] = "Die Sicherung hat nicht geklappt. Es wurde nichts verändert.",
    [STR_RESTORE_FAILED] =
        "Das Wiederherstellen hat nicht geklappt. Bitte versuche es noch einmal.",
    [STR_PUZZLE_COMPLETE] = "Puzzle fertig!",
    [STR_VERSION_LINE] = "v%s · inoffizielle Homebrew",

    [STR_PROGRESS_OF] = "%u von %u",
    [STR_PROGRESS_MISSING] = "%u fehlen",
    [STR_SELECTED] = "%u ausgewählt",
    [STR_AFTER_PURCHASE] = "danach %u/%u",
    [STR_CONFIRM_QUESTION] = "Diese %s für %s kaufen?",
    [STR_BALANCE] = "Guthaben",
    [STR_DO_NOT_TURN_OFF] = "Bitte nicht ausschalten",
    [STR_NEW_PIECES_BANNER] = "Neue Teile!",
    [STR_BOUGHT_HEADLINE] = "%s erhalten!",
    [STR_BOUGHT_PROGRESS] = "%s · jetzt %u von %u Teilen",
    [STR_COINS_LEFT] = "Du hast noch %s.",
    [STR_BACKUP_FROM] = "Sicherung vom %s",
    [STR_NOT_ENOUGH_COINS] = "Dafür reichen deine Spielmünzen nicht.",
    [STR_RARE_PIECE] = "Seltenes Teil",
    [STR_PANEL_UNAVAILABLE] = "Dieses Puzzle ist auf deiner Konsole nicht vorhanden.",
    [STR_PICTURES_MISSING] = "Bilder der ersten 7 Puzzles fehlen, siehe README.",
    [STR_BACKUP_ORIGINAL] = "Ursicherung",
    [STR_BACKUP_BEFORE_PURCHASE] = "vor Kauf: %s, %s",
    [STR_BACKUP_BEFORE_RESTORE] = "vor Wiederherstellung",
    [STR_BACKUP_BEFORE_PURCHASE_PLAIN] = "vor Kauf",
    [STR_BACKUP_BEFORE_CHANGE] = "vor Änderung",
    [STR_RESTORE_QUESTION] = "Diesen Stand wiederherstellen?",
    [STR_QUIT_QUESTION] = "App wirklich beenden?",
    [STR_RESTORE_SAFETY] = "Dein jetziger Stand wird vorher gesichert.",
    [STR_RESTORED] = "Der Stand ist wiederhergestellt.",
    [STR_BACKUPS_EMPTY] = "Es gibt noch keine Sicherungen.",
    [STR_BACKUP_LOCKED] = "Die Ursicherung bleibt immer erhalten.",

    [STR_SETTINGS_SOUND] = "Klang",
    [STR_SETTINGS_LANGUAGE] = "Sprache",
    [STR_SETTINGS_LANGUAGE_AUTO] = "Automatisch",
    [STR_LANGUAGE_DE] = "Deutsch",
    [STR_LANGUAGE_EN] = "English",
    [STR_SWITCH_ON] = "An",
    [STR_SWITCH_OFF] = "Aus",
    [STR_SETTINGS_SAVE_FAILED] = "Die Einstellung konnte nicht gespeichert werden.",
    [STR_INFO_LICENSE] = "Lizenz: GPL-3.0",
    [STR_INFO_THANKS] = "Danke an 3dbrew, Marc Robledo, MrCheeze, FlagBrew, ArmsAsuncion und "
                        "Khromasoul für ihr Wissen über die Spieldaten.",

    [STR_E01] = "Kein Zugriff auf die Spieldaten. Ist Luma3DS aktiv?",
    [STR_E02] = "StreetPass Mii Plaza wurde nicht gefunden.",
    [STR_E03] = "Dieser Spielstand sieht anders aus als erwartet. Zur Sicherheit wird nichts "
                "verändert.",
    [STR_E04] = "Der Kauf hat nicht geklappt. Es wurde nichts verändert.",
    [STR_E05] = "Bitte stelle die letzte Sicherung wieder her.",
    [STR_E06] = "Ein Kauf wurde unterbrochen.",
};

/* "Play Coins" is Nintendo's English term; same tone and rules as German. */
static const char *const s_en[STR_COUNT] = {
    [STR_APP_NAME] = "Puzzle Swap Shop",
    [STR_APP_TAGLINE] = "Missing pieces for Play Coins",
    [STR_UNOFFICIAL] = "Unofficial homebrew. Not made by Nintendo.",

    [STR_BTN_BACK] = "Back",
    [STR_BTN_QUIT] = "Quit",
    [STR_BTN_OPEN] = "Open",
    [STR_BTN_BUY] = "Buy",
    [STR_BTN_CANCEL] = "Cancel",
    [STR_BTN_CONTINUE] = "Continue",
    [STR_BTN_OK] = "OK",
    [STR_BTN_CREATE_BACKUP] = "Create backup",
    [STR_BTN_BACKUPS] = "Backups",
    [STR_BTN_TO_BACKUPS] = "To the backups",
    [STR_BTN_CHANGE] = "Change",
    [STR_BTN_SELECT_ALL] = "All missing",
    [STR_BTN_RESTORE] = "Restore",
    [STR_BTN_KEEP] = "Keep",
    [STR_BTN_VIEW_ONLY] = "View only",

    [STR_TITLE_ONBOARDING] = "Before you start",
    [STR_TITLE_GALLERY] = "Puzzle gallery",
    [STR_TITLE_PICKER] = "Choose pieces",
    [STR_TITLE_CONFIRM] = "Confirm purchase",
    [STR_TITLE_BOUGHT] = "Purchased",
    [STR_TITLE_BACKUPS] = "Backups",
    [STR_TITLE_SETTINGS] = "Settings",

    [STR_ONBOARDING_BODY] = "First we back up your Mii Plaza data and your Play Coins to the SD "
                            "card. That way, every purchase can be undone.",
    [STR_FILE_MEET] = "Mii Plaza save data",
    [STR_FILE_COINS] = "Play Coins",
    [STR_BACKUP_WORKING] = "Please wait …",
    [STR_BACKUP_FAILED] = "The backup didn't work. Nothing was changed.",
    [STR_RESTORE_FAILED] = "Restoring didn't work. Please try again.",
    [STR_PUZZLE_COMPLETE] = "Puzzle complete!",
    [STR_VERSION_LINE] = "v%s · unofficial homebrew",

    [STR_PROGRESS_OF] = "%u of %u",
    [STR_PROGRESS_MISSING] = "%u missing",
    [STR_SELECTED] = "%u selected",
    [STR_AFTER_PURCHASE] = "then %u/%u",
    /* No "these": reads correctly for 1 piece as well. */
    [STR_CONFIRM_QUESTION] = "Buy %s for %s?",
    [STR_BALANCE] = "Balance",
    [STR_DO_NOT_TURN_OFF] = "Please don't turn off the system",
    [STR_NEW_PIECES_BANNER] = "New pieces!",
    [STR_BOUGHT_HEADLINE] = "You got %s!",
    [STR_BOUGHT_PROGRESS] = "%s · now %u of %u pieces",
    [STR_COINS_LEFT] = "You have %s left.",
    [STR_BACKUP_FROM] = "Backup from %s",
    [STR_NOT_ENOUGH_COINS] = "You don't have enough Play Coins for that.",
    [STR_RARE_PIECE] = "Rare piece",
    [STR_PANEL_UNAVAILABLE] = "This puzzle isn't on your system.",
    [STR_PICTURES_MISSING] = "Pictures of the first 7 puzzles are missing, see README.",
    [STR_BACKUP_ORIGINAL] = "Original backup",
    [STR_BACKUP_BEFORE_PURCHASE] = "before purchase: %s, %s",
    [STR_BACKUP_BEFORE_RESTORE] = "before restore",
    [STR_BACKUP_BEFORE_PURCHASE_PLAIN] = "before purchase",
    [STR_BACKUP_BEFORE_CHANGE] = "before change",
    [STR_RESTORE_QUESTION] = "Restore this backup?",
    [STR_QUIT_QUESTION] = "Really quit the app?",
    [STR_RESTORE_SAFETY] = "Your current state is saved first.",
    [STR_RESTORED] = "The backup is restored.",
    [STR_BACKUPS_EMPTY] = "There are no backups yet.",
    [STR_BACKUP_LOCKED] = "The original backup is always kept.",

    [STR_SETTINGS_SOUND] = "Sound",
    [STR_SETTINGS_LANGUAGE] = "Language",
    [STR_SETTINGS_LANGUAGE_AUTO] = "Automatic",
    [STR_LANGUAGE_DE] = "Deutsch",
    [STR_LANGUAGE_EN] = "English",
    [STR_SWITCH_ON] = "On",
    [STR_SWITCH_OFF] = "Off",
    [STR_SETTINGS_SAVE_FAILED] = "The setting could not be saved.",
    [STR_INFO_LICENSE] = "License: GPL-3.0",
    [STR_INFO_THANKS] = "Thanks to 3dbrew, Marc Robledo, MrCheeze, FlagBrew, ArmsAsuncion and "
                        "Khromasoul for what they found out about the game data.",

    [STR_E01] = "Can't access the game data. Is Luma3DS running?",
    [STR_E02] = "StreetPass Mii Plaza was not found.",
    [STR_E03] = "This save data looks different than expected. To be safe, nothing will be "
                "changed.",
    [STR_E04] = "The purchase didn't go through. Nothing was changed.",
    [STR_E05] = "Please restore the last backup.",
    [STR_E06] = "A purchase was interrupted.",
};

static const char *const *const s_tables[LANG_COUNT] = {s_de, s_en};

static Language s_lang = LANG_DE;

void strings_set_language(Language lang)
{
    if (lang < LANG_COUNT)
        s_lang = lang;
}

Language strings_language(void)
{
    return s_lang;
}

const char *str_lang(Language lang, StringId id)
{
    if (lang >= LANG_COUNT || id >= STR_COUNT)
        return NULL;
    return s_tables[lang][id];
}

const char *str(StringId id)
{
    const char *s = str_lang(s_lang, id);
    if (!s)
        s = str_lang(LANG_DE, id);
    return s ? s : "";
}

int strings_pieces(char *out, size_t size, unsigned count)
{
    if (s_lang == LANG_EN)
        return snprintf(out, size, count == 1 ? "%u piece" : "%u pieces", count);
    return snprintf(out, size, count == 1 ? "%u Teil" : "%u Teile", count);
}

int strings_new_pieces(char *out, size_t size, unsigned count)
{
    if (s_lang == LANG_EN)
        return snprintf(out, size, count == 1 ? "%u new piece" : "%u new pieces", count);
    return snprintf(out, size, count == 1 ? "%u neues Teil" : "%u neue Teile", count);
}

int strings_coins(char *out, size_t size, unsigned count)
{
    if (s_lang == LANG_EN)
        return snprintf(out, size, count == 1 ? "%u Play Coin" : "%u Play Coins", count);
    return snprintf(out, size, count == 1 ? "%u Spielmünze" : "%u Spielmünzen", count);
}
