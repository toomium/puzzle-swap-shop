#ifndef PSS_CORE_SETTINGS_H
#define PSS_CORE_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

#include "core/shop.h"

/*
 * settings.ini in the app folder. Prices can
 * be changed only in the file, sound and language also in S8.
 */

#define SETTINGS_FILE     "settings.ini"
#define SETTINGS_TEXT_MAX 512

/* Same order as LanguageSetting in core/lang_map.h. */
typedef enum {
    SETTINGS_LANG_AUTO = 0,
    SETTINGS_LANG_DE,
    SETTINGS_LANG_EN,
    SETTINGS_LANG_COUNT,
} SettingsLanguage;

typedef struct {
    ShopPrices prices;
    bool sound;
    SettingsLanguage language;
} Settings;

Settings settings_default(void);

/*
 * Reads settings.ini text over the defaults. Unknown keys are ignored; a value
 * that is out of range keeps its default and counts in *bad (may be NULL).
 * Prices: per piece 1..300, rare piece 0..300 (0 = not for sale).
 * Returns false and gives the defaults if the text is not key=value at all.
 */
bool settings_parse(const char *text, size_t len, Settings *out, unsigned *bad);

/* NUL-terminated text with short German comments; false if cap is too small. */
bool settings_format(const Settings *s, char *out, size_t cap, size_t *len);

#endif
