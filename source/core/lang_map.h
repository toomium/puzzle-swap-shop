#ifndef PSS_CORE_LANG_MAP_H
#define PSS_CORE_LANG_MAP_H

#include "core/strings.h"

/*
 * Picks the app language from the setting in settings.ini and the console's
 * system language. Pure logic; platform/language.c reads the system value.
 */

/* System language codes as returned by CFGU_GetSystemLanguage (libctru CFG_Language). */
enum {
    LANG_MAP_SYS_JP = 0,
    LANG_MAP_SYS_EN = 1,
    LANG_MAP_SYS_FR = 2,
    LANG_MAP_SYS_DE = 3,
    LANG_MAP_SYS_IT = 4,
    LANG_MAP_SYS_ES = 5,
    LANG_MAP_SYS_ZH = 6,
    LANG_MAP_SYS_KO = 7,
    LANG_MAP_SYS_NL = 8,
    LANG_MAP_SYS_PT = 9,
    LANG_MAP_SYS_RU = 10,
    LANG_MAP_SYS_TW = 11,
};

typedef enum {
    LANG_SETTING_AUTO = 0,
    LANG_SETTING_DE,
    LANG_SETTING_EN,
    LANG_SETTING_COUNT,
} LanguageSetting;

/* German consoles get German, every other system language English. */
Language lang_map_system(unsigned system_language);

/* LANG_SETTING_AUTO follows the console; out-of-range settings count as auto. */
Language lang_map_resolve(LanguageSetting setting, unsigned system_language);

#endif
