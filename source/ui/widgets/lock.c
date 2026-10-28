#include "ui/widgets/lock.h"

#include "ui/draw.h"

#define SHACKLE_W 2
#define BODY_H    8

void lock_draw(int x, int y, ThemeColor color, unsigned char alpha)
{
    int shackle_h = THEME_LOCK_H - BODY_H;
    int inset = SHACKLE_W + 1;
    /* Shackle: an arch of two posts and a bar. */
    draw_rect(x + inset, y, THEME_LOCK_W - 2 * inset, SHACKLE_W, color, alpha);
    draw_rect(x + inset, y, SHACKLE_W, shackle_h + 1, color, alpha);
    draw_rect(x + THEME_LOCK_W - inset - SHACKLE_W, y, SHACKLE_W, shackle_h + 1, color, alpha);
    /* Body. */
    GeomRect body = {x, y + shackle_h, THEME_LOCK_W, BODY_H};
    draw_slice_tinted(DRAW_SLICE_ROUND_3, body, color, color, alpha);
}
