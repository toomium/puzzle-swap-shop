#include "core/pnel_index.h"

#include <string.h>

void pnel_index_init(PnelIndex *idx)
{
    memset(idx, 0, sizeof(*idx));
}

bool pnel_index_add(PnelIndex *idx, unsigned file, const PnelEntry *entry, const char *name,
                    const char *name_fallback)
{
    const PanelInfo *info = entry->order < PANEL_COUNT ? panel_catalog_get(entry->order) : NULL;
    if (info == NULL || info->kind != PANEL_SPOTPASS ||
        pnel_piece_count(entry->piece_count_id) != info->piece_count || file > UINT8_MAX) {
        idx->ignored++;
        return false;
    }
    PnelPanel *p = &idx->panel[info->id];
    memset(p, 0, sizeof(*p));
    p->present = true;
    p->file = (uint8_t)file;
    p->image_offset = entry->image_offset;
    p->image_size = entry->image_size;
    p->pink = entry->pink;
    const char *use = name != NULL && name[0] != '\0' ? name : name_fallback;
    if (use != NULL)
        strncpy(p->name, use, sizeof(p->name) - 1);
    idx->accepted++;
    return true;
}

bool pnel_index_panel_present(const PnelIndex *idx, unsigned id, uint64_t owned)
{
    const PanelInfo *info = panel_catalog_get(id);
    if (info == NULL)
        return false;
    if (info->kind == PANEL_STANDARD)
        return true;
    return owned != 0 || (idx != NULL && idx->panel[id].present);
}
