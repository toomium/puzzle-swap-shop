#include "core/mii_plaza.h"

/*
 * Title ids and the save id rule are confirmed for EUR only (seen on a console:
 * sysdata/00020228). JPN and USA follow
 * the same rule. The extdata ids come from 3dbrew; for USA it lists 0x217,
 * which does not fit the rule, so both are tried until a console settles it.
 */
static const PlazaTitle titles[PLAZA_REGION_COUNT] = {
    {PLAZA_JPN, "JPN", 0x0004001000020800ull, 0x00020208u, {0x00000208u, 0}},
    {PLAZA_USA, "USA", 0x0004001000021800ull, 0x00020218u, {0x00000217u, 0x00000218u}},
    {PLAZA_EUR, "EUR", 0x0004001000022800ull, 0x00020228u, {0x00000228u, 0}},
};

const PlazaTitle *mii_plaza_title(PlazaRegion region)
{
    if ((unsigned)region >= PLAZA_REGION_COUNT)
        return NULL;
    return &titles[region];
}

uint32_t mii_plaza_save_id(uint64_t title_id)
{
    return 0x00020000u | ((uint32_t)title_id >> 8);
}

static int installed(const uint64_t *list, size_t count, uint64_t title_id)
{
    for (size_t i = 0; i < count; i++) {
        if (list[i] == title_id)
            return 1;
    }
    return 0;
}

const PlazaTitle *mii_plaza_find(const uint64_t *list, size_t count, int console_region)
{
    static const PlazaRegion fallback[] = {PLAZA_EUR, PLAZA_USA, PLAZA_JPN};
    PlazaRegion preferred;

    switch (console_region) {
    case 0:
        preferred = PLAZA_JPN;
        break;
    case 1:
        preferred = PLAZA_USA;
        break;
    case 2:
    case 3:
        preferred = PLAZA_EUR;
        break;
    default:
        preferred = PLAZA_REGION_COUNT;
        break;
    }

    if (preferred != PLAZA_REGION_COUNT && installed(list, count, titles[preferred].title_id))
        return &titles[preferred];
    for (size_t i = 0; i < sizeof(fallback) / sizeof(fallback[0]); i++) {
        if (installed(list, count, titles[fallback[i]].title_id))
            return &titles[fallback[i]];
    }
    return NULL;
}
