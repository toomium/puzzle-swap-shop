#include "core/widget_math.h"

int widget_progress_fill(int track_w, unsigned owned, unsigned total)
{
    if (total == 0 || track_w <= 0)
        return 0;
    if (owned >= total)
        return track_w;
    return (int)((unsigned long long)track_w * owned / total);
}

GeomRect widget_board_cell(GeomRect art, int cols, int cell, int index)
{
    GeomRect r = {art.x + (index % cols) * cell, art.y + (index / cols) * cell, cell, cell};
    return r;
}

static int round_px(float v)
{
    return v >= 0.0f ? (int)(v + 0.5f) : -(int)(-v + 0.5f);
}

GeomRect widget_scale_rect(GeomRect r, float scale)
{
    int w = round_px((float)r.w * scale);
    int h = round_px((float)r.h * scale);
    if (w < 0)
        w = 0;
    if (h < 0)
        h = 0;
    GeomRect out = {r.x + (r.w - w) / 2, r.y + (r.h - h) / 2, w, h};
    return out;
}

int widget_mask_has(uint64_t mask, int index)
{
    if (index < 0 || index > 63)
        return 0;
    return (int)((mask >> index) & 1u);
}
