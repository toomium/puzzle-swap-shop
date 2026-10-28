#include "core/settings.h"

#include <stdio.h>
#include <string.h>

#include "core/gamecoin.h"
#include "core/kvtext.h"

static const char *const language_names[SETTINGS_LANG_COUNT] = {"auto", "de", "en"};

Settings settings_default(void)
{
    Settings s;
    memset(&s, 0, sizeof(s)); /* callers compare settings with memcmp */
    s.prices = shop_default_prices();
    s.sound = true;
    s.language = SETTINGS_LANG_AUTO;
    return s;
}

static void read_price(const KvList *kv, const char *key, unsigned min, uint16_t *price,
                       unsigned *bad)
{
    uint32_t v;
    if (kv_get(kv, key) == NULL)
        return;
    if (kv_get_u32(kv, key, &v) && v >= min && v <= GAMECOIN_MAX)
        *price = (uint16_t)v;
    else
        (*bad)++;
}

bool settings_parse(const char *text, size_t len, Settings *out, unsigned *bad)
{
    KvList kv;
    unsigned dummy = 0;
    if (bad == NULL)
        bad = &dummy;
    *bad = 0;
    *out = settings_default();
    if (kv_parse(text, len, &kv) != KV_OK)
        return false;

    read_price(&kv, "price_per_piece", 1, &out->prices.per_piece, bad);
    read_price(&kv, "price_rare_piece", 0, &out->prices.per_rare_piece, bad);

    const char *sound = kv_get(&kv, "sound");
    if (sound != NULL) {
        if (strcmp(sound, "on") == 0)
            out->sound = true;
        else if (strcmp(sound, "off") == 0)
            out->sound = false;
        else
            (*bad)++;
    }

    const char *language = kv_get(&kv, "language");
    if (language != NULL) {
        unsigned i = 0;
        while (i < SETTINGS_LANG_COUNT && strcmp(language, language_names[i]) != 0)
            i++;
        if (i < SETTINGS_LANG_COUNT)
            out->language = (SettingsLanguage)i;
        else
            (*bad)++;
    }
    return true;
}

bool settings_format(const Settings *s, char *out, size_t cap, size_t *len)
{
    unsigned language = (unsigned)s->language < SETTINGS_LANG_COUNT ? s->language : 0;
    int n = snprintf(out, cap,
                     "# Puzzle Swap Shop\n"
                     "# Preise in Spielmuenzen pro Teil (1 bis 300)\n"
                     "# price_rare_piece=0: seltene Teile nicht verkaufen\n"
                     "# language: auto, de oder en\n"
                     "price_per_piece=%u\n"
                     "price_rare_piece=%u\n"
                     "sound=%s\n"
                     "language=%s\n",
                     (unsigned)s->prices.per_piece, (unsigned)s->prices.per_rare_piece,
                     s->sound ? "on" : "off", language_names[language]);
    if (n < 0 || (size_t)n >= cap)
        return false;
    *len = (size_t)n;
    return true;
}
