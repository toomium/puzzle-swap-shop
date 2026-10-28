#include "ui/widgets/progress.h"

#include "core/widget_math.h"
#include "ui/draw.h"
#include "ui/theme.h"

void progress_draw(GeomRect track, unsigned owned, unsigned total, ProgressStyle style,
                   unsigned char alpha)
{
    if (style == PROGRESS_ON_SKY)
        draw_slice_tinted(DRAW_SLICE_ROUND_3, track, THEME_BLACK, THEME_BLACK,
                          (u8)(THEME_ALPHA_SHADOW * alpha / THEME_ALPHA_OPAQUE));
    else
        draw_slice_tinted(DRAW_SLICE_ROUND_3, track, THEME_TRACK, THEME_TRACK, alpha);

    int w = widget_progress_fill(track.w, owned, total);
    if (w <= 0)
        return;
    GeomRect fill = {track.x, track.y, w, track.h};
    if (total > 0 && owned >= total)
        draw_slice_tinted(DRAW_SLICE_ROUND_3, fill, THEME_COIN_LIGHT, THEME_COIN, alpha);
    else
        draw_slice_tinted(DRAW_SLICE_ROUND_3, fill, THEME_PLAZA_HI, THEME_PLAZA, alpha);
}
