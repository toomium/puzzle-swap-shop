#include "core/meet_save.h"

#include "core/panel_bits.h"
#include "core/panel_catalog.h"

size_t meet_save_panel_offset(unsigned panel_id)
{
    const PanelInfo *info = panel_catalog_get(panel_id);
    if (!info)
        return 0;
    if (info->kind == PANEL_STANDARD)
        return MEET_STANDARD_PANELS + (size_t)panel_id * MEET_STANDARD_PANEL_SIZE;
    return MEET_SPOTPASS_PANELS +
           (size_t)(panel_id - PANEL_STANDARD_COUNT) * MEET_SPOTPASS_PANEL_SIZE;
}

MeetResult meet_save_validate(const uint8_t *buf, size_t len)
{
    if (!buf || len != MEET_SAVE_SIZE)
        return MEET_ERR_SIZE;
    for (unsigned id = 0; id < PANEL_COUNT; id++) {
        const PanelInfo *info = panel_catalog_get(id);
        const uint8_t *field = buf + meet_save_panel_offset(id);
        if (!panel_bits_unused_clear(field, info->kind, info->piece_count))
            return MEET_ERR_UNUSED_BITS;
    }
    return MEET_OK;
}

MeetResult meet_save_get_owned(const uint8_t *buf, size_t len, unsigned panel_id, uint64_t *owned)
{
    if (!buf || len != MEET_SAVE_SIZE)
        return MEET_ERR_SIZE;
    const PanelInfo *info = panel_catalog_get(panel_id);
    if (!info)
        return MEET_ERR_PANEL;
    *owned =
        panel_bits_decode(buf + meet_save_panel_offset(panel_id), info->kind, info->piece_count);
    return MEET_OK;
}

MeetResult meet_save_add_pieces(uint8_t *buf, size_t len, unsigned panel_id, uint64_t pieces)
{
    uint64_t owned;
    MeetResult res = meet_save_get_owned(buf, len, panel_id, &owned);
    if (res != MEET_OK)
        return res;

    const PanelInfo *info = panel_catalog_get(panel_id);
    if (pieces & ~panel_bits_all(info->piece_count))
        return MEET_ERR_RANGE;
    if (pieces & owned)
        return MEET_ERR_OWNED;

    panel_bits_encode(buf + meet_save_panel_offset(panel_id), info->kind, info->piece_count,
                      owned | pieces);
    return MEET_OK;
}

MeetResult meet_save_remove_pieces(uint8_t *buf, size_t len, unsigned panel_id, uint64_t pieces)
{
    uint64_t owned;
    MeetResult res = meet_save_get_owned(buf, len, panel_id, &owned);
    if (res != MEET_OK)
        return res;

    const PanelInfo *info = panel_catalog_get(panel_id);
    if (pieces & ~panel_bits_all(info->piece_count))
        return MEET_ERR_RANGE;
    if (pieces & ~owned)
        return MEET_ERR_NOT_OWNED;

    panel_bits_encode(buf + meet_save_panel_offset(panel_id), info->kind, info->piece_count,
                      owned & ~pieces);
    return MEET_OK;
}
