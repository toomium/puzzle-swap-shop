#ifndef PSS_CORE_PANEL_CATALOG_H
#define PSS_CORE_PANEL_CATALOG_H

#include <stdint.h>

#define PANEL_COUNT          63
#define PANEL_STANDARD_COUNT 7
#define PANEL_MAX_PIECES     40

typedef enum {
    PANEL_STANDARD = 0, /* panels 0..6, built into Mii Plaza */
    PANEL_SPOTPASS = 1, /* panels 7.., distributed via SpotPass */
} PanelKind;

typedef struct {
    uint8_t id;
    PanelKind kind;
    uint8_t piece_count; /* 15, 24 or 40 */
    const char *fallback_name;
} PanelInfo;

/* Returns NULL for an unknown panel id. */
const PanelInfo *panel_catalog_get(unsigned id);

/*
 * File name of the picture of a built-in panel (0..6), as it lies in the RomFS of
 * Mii Plaza and, copied from there, in the app's "panels" folder on the SD card. NULL for every other id.
 */
const char *panel_catalog_builtin_image(unsigned id);

/*
 * Columns of the picture grid: 5 for 15 pieces, 6 for 24, 8 for 40; 0 for any
 * other count. Piece i sits in row i / columns, column i % columns.
 */
unsigned panel_catalog_columns(unsigned piece_count);

#endif
