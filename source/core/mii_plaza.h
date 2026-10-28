#ifndef PSS_CORE_MII_PLAZA_H
#define PSS_CORE_MII_PLAZA_H

#include <stddef.h>
#include <stdint.h>

/* StreetPass Mii Plaza titles and their storage ids. */

typedef enum {
    PLAZA_JPN = 0,
    PLAZA_USA,
    PLAZA_EUR,
    PLAZA_REGION_COUNT,
} PlazaRegion;

typedef struct {
    PlazaRegion region;
    const char *code;        /* "JPN", "USA", "EUR" */
    uint64_t title_id;       /* NAND system application */
    uint32_t save_id;        /* ARCHIVE_SYSTEM_SAVEDATA id holding meet.dat */
    uint32_t extdata_ids[2]; /* candidates on SD, most likely first; 0 = unused */
} PlazaTitle;

#define PLAZA_GAMECOIN_EXTDATA 0xF000000Bu

const PlazaTitle *mii_plaza_title(PlazaRegion region);

/* System save id rule used for NAND titles: 0x00020000 | (low title id >> 8). */
uint32_t mii_plaza_save_id(uint64_t title_id);

/*
 * Picks the installed Mii Plaza from a NAND title list. Prefers the title of
 * the console region (CFG region: 0 JPN, 1 USA, 2 EUR, 3 AUS, ...; AUS uses
 * the EUR title), otherwise the first found in the order EUR, USA, JPN.
 * Returns NULL if none is installed.
 */
const PlazaTitle *mii_plaza_find(const uint64_t *titles, size_t count, int console_region);

#endif
