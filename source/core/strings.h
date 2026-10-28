#ifndef PSS_CORE_STRINGS_H
#define PSS_CORE_STRINGS_H

#include <stddef.h>

/*
 * All user-visible text. Strings with
 * placeholders are printf formats; the comment names the arguments.
 */

typedef enum {
    LANG_DE = 0,
    LANG_EN,
    LANG_COUNT,
} Language;

typedef enum {
    STR_APP_NAME,
    STR_APP_TAGLINE,
    STR_UNOFFICIAL,

    STR_BTN_BACK,
    STR_BTN_QUIT,
    STR_BTN_OPEN,
    STR_BTN_BUY,
    STR_BTN_CANCEL,
    STR_BTN_CONTINUE,
    STR_BTN_OK,
    STR_BTN_CREATE_BACKUP,
    STR_BTN_BACKUPS,
    STR_BTN_SELECT_ALL,
    STR_BTN_RESTORE,
    STR_BTN_KEEP,
    STR_BTN_VIEW_ONLY,
    STR_BTN_TO_BACKUPS,
    STR_BTN_CHANGE,

    STR_TITLE_ONBOARDING,
    STR_TITLE_GALLERY,
    STR_TITLE_PICKER,
    STR_TITLE_CONFIRM,
    STR_TITLE_BOUGHT,
    STR_TITLE_BACKUPS,
    STR_TITLE_SETTINGS,

    STR_ONBOARDING_BODY,
    STR_FILE_MEET,
    STR_FILE_COINS,
    STR_BACKUP_WORKING,
    STR_BACKUP_FAILED,
    STR_RESTORE_FAILED,
    STR_PUZZLE_COMPLETE,
    STR_VERSION_LINE, /* %s version: "v1.0.0 · inoffizielle Homebrew" */

    STR_PROGRESS_OF,      /* %u owned, %u total: "28 von 40" */
    STR_PROGRESS_MISSING, /* %u missing: "12 fehlen" */
    STR_SELECTED,         /* %u selected: "3 ausgewählt" */
    STR_AFTER_PURCHASE,   /* %u owned, %u total: "danach 31/40" */
    STR_CONFIRM_QUESTION, /* %s pieces, %s coins: "Diese 3 Teile für 6 Spielmünzen kaufen?" */
    STR_BALANCE,
    STR_DO_NOT_TURN_OFF,
    STR_NEW_PIECES_BANNER,
    STR_BOUGHT_HEADLINE, /* %s new pieces: "3 neue Teile erhalten!" */
    STR_BOUGHT_PROGRESS, /* %s panel, %u owned, %u total */
    STR_COINS_LEFT,      /* %s coins: "Du hast noch 114 Spielmünzen." */
    STR_BACKUP_FROM,     /* %s date and time */
    STR_NOT_ENOUGH_COINS,
    STR_RARE_PIECE,
    STR_PANEL_UNAVAILABLE,
    STR_PICTURES_MISSING,
    STR_BACKUP_ORIGINAL,
    STR_BACKUP_BEFORE_PURCHASE, /* %s panel, %s pieces */
    STR_BACKUP_BEFORE_RESTORE,
    STR_BACKUP_BEFORE_PURCHASE_PLAIN, /* purchase backup without details */
    STR_BACKUP_BEFORE_CHANGE,         /* made by the diagnostic build */
    STR_RESTORE_QUESTION,
    STR_QUIT_QUESTION,
    STR_RESTORE_SAFETY, /* the current state is saved first */
    STR_RESTORED,
    STR_BACKUPS_EMPTY,
    STR_BACKUP_LOCKED,

    STR_SETTINGS_SOUND,
    STR_SETTINGS_LANGUAGE,
    STR_SETTINGS_LANGUAGE_AUTO,
    STR_LANGUAGE_DE, /* same in every language: the name of the language itself */
    STR_LANGUAGE_EN,
    STR_SWITCH_ON,
    STR_SWITCH_OFF,
    STR_SETTINGS_SAVE_FAILED,
    STR_INFO_LICENSE,
    STR_INFO_THANKS,

    STR_E01,
    STR_E02,
    STR_E03,
    STR_E04,
    STR_E05,
    STR_E06,

    STR_COUNT,
} StringId;

void strings_set_language(Language lang);
Language strings_language(void);

/* Text in the current language, falling back to German. Never NULL. */
const char *str(StringId id);

/* Text in a specific language, or NULL if not translated yet. */
const char *str_lang(Language lang, StringId id);

/* "1 Teil" / "3 Teile". Returns the snprintf result. */
int strings_pieces(char *out, size_t size, unsigned count);

/* "1 neues Teil" / "3 neue Teile". */
int strings_new_pieces(char *out, size_t size, unsigned count);

/* "1 Spielmünze" / "6 Spielmünzen". */
int strings_coins(char *out, size_t size, unsigned count);

#endif
