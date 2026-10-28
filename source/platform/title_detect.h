#ifndef PSS_PLATFORM_TITLE_DETECT_H
#define PSS_PLATFORM_TITLE_DETECT_H

#include <3ds.h>

#include "core/mii_plaza.h"

typedef enum {
    TITLE_FOUND = 0,
    TITLE_NOT_INSTALLED, /* E-02 */
    TITLE_ERR_SERVICE,   /* AM not reachable, usually no CFW: E-01 */
} TitleStatus;

typedef struct {
    const PlazaTitle *title; /* NULL unless TITLE_FOUND */
    int console_region;      /* CFG region, -1 if unknown */
    Result error;            /* failing libctru result, 0 otherwise */
} TitleInfo;

/* Finds the installed StreetPass Mii Plaza in the NAND title list. */
TitleStatus title_detect(TitleInfo *out);

#endif
