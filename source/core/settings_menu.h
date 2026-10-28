#ifndef PSS_CORE_SETTINGS_MENU_H
#define PSS_CORE_SETTINGS_MENU_H

#include <stdbool.h>

#include "core/settings.h"
#include "core/strings.h"

/*
 * Rows and values of S8 "Einstellungen". Pure
 * logic: the screen applies and saves the result.
 */

typedef enum {
    SETTINGS_ROW_SOUND = 0,
    SETTINGS_ROW_LANGUAGE,
    SETTINGS_ROW_COUNT,
} SettingsRow;

/* Up or down by one row; false at the first or last row. */
bool settings_menu_move(int *row, int delta);

/* The language one step on, wrapping: automatic, German, English. delta is -1 or 1. */
SettingsLanguage settings_menu_cycle_language(SettingsLanguage lang, int delta);

/* Name of a language setting in its own language ("Automatisch" follows the app language). */
StringId settings_menu_language_label(SettingsLanguage lang);

#endif
