#ifndef PSS_CORE_PNEL_INDEX_H
#define PSS_CORE_PNEL_INDEX_H

#include <stdbool.h>
#include <stdint.h>

#include "core/panel_catalog.h"
#include "core/panel_state.h"
#include "core/pnel.h"

/*
 * What the PNEL files on the console say about the SpotPass panels: which panels are downloaded,
 * their names and where their pictures lie. Pure logic; app/pnel_load.c fills it from the files.
 *
 * Hypothesis: the entry's order number is the
 * panel id. An entry whose piece count differs from panel_catalog is
 * ignored, so the shop never sells for a panel it does not understand.
 */

typedef struct {
    bool present;
    uint8_t file; /* index of the PNEL file, as the caller numbered it */
    uint32_t image_offset, image_size;
    uint8_t pink;              /* raw byte at entry offset 0xC2D, see rare_pieces.h */
    char name[PANEL_NAME_MAX]; /* UTF-8; empty if the file has none */
} PnelPanel;

typedef struct {
    PnelPanel panel[PANEL_COUNT]; /* by panel id; standard panels are never present */
    unsigned accepted;
    unsigned ignored;
} PnelIndex;

void pnel_index_init(PnelIndex *idx);

/*
 * Adds one entry. `name` is the name in the console language, `name_fallback`
 * the English one (either may be empty or NULL). Returns false and counts the
 * entry as ignored if it does not map onto a known SpotPass panel. A later
 * entry for the same panel replaces the earlier one.
 */
bool pnel_index_add(PnelIndex *idx, unsigned file, const PnelEntry *entry, const char *name,
                    const char *name_fallback);

/*
 * Whether the console has the panel. Standard panels always. A SpotPass panel
 * if a PNEL entry lists it or the player already owns a piece of it (the save
 * data proves it exists); idx may be NULL when no PNEL file was found.
 */
bool pnel_index_panel_present(const PnelIndex *idx, unsigned id, uint64_t owned);

#endif
