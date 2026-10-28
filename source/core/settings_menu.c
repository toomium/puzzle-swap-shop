#include "core/settings_menu.h"

bool settings_menu_move(int *row, int delta)
{
    int next = *row + delta;
    if (next < 0 || next >= SETTINGS_ROW_COUNT)
        return false;
    *row = next;
    return true;
}

SettingsLanguage settings_menu_cycle_language(SettingsLanguage lang, int delta)
{
    int next = ((int)lang + delta) % SETTINGS_LANG_COUNT;
    if (next < 0)
        next += SETTINGS_LANG_COUNT;
    return (SettingsLanguage)next;
}

StringId settings_menu_language_label(SettingsLanguage lang)
{
    switch (lang) {
    case SETTINGS_LANG_DE:
        return STR_LANGUAGE_DE;
    case SETTINGS_LANG_EN:
        return STR_LANGUAGE_EN;
    default:
        return STR_SETTINGS_LANGUAGE_AUTO;
    }
}
