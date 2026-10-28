#include "ui/widgets/pill.h"

#include <stdio.h>

#include "ui/draw.h"
#include "ui/theme.h"
#include "ui/widgets/coin.h"

GeomRect pill_name_draw(int x, int y, const char *name, const char *count)
{
    int name_w = draw_text_width(THEME_TEXT_BUTTON, name);
    int count_w = draw_text_width(THEME_TEXT_BODY, count);
    GeomRect box = {x, y, THEME_PILL_PAD + name_w + THEME_TEXT_GAP + count_w + THEME_PILL_PAD,
                    THEME_PILL_H};
    draw_slice(DRAW_SLICE_PILL, box, THEME_ALPHA_OPAQUE);
    int tx = box.x + THEME_PILL_PAD;
    draw_text_vcenter(tx, box.y, box.h, THEME_TEXT_BUTTON, THEME_INK, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, name);
    draw_text_vcenter(tx + name_w + THEME_TEXT_GAP, box.y, box.h, THEME_TEXT_BODY, THEME_INK_2,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, count);
    return box;
}

GeomRect pill_coin_draw(int x, int y, bool right_aligned, unsigned coins, unsigned char shine)
{
    char num[12];
    snprintf(num, sizeof(num), "%u", coins);
    int w = THEME_PILL_COIN_PAD + THEME_COIN_MEDIUM + THEME_TEXT_GAP +
            draw_text_width(THEME_TEXT_BUTTON, num) + THEME_PILL_PAD;
    if (right_aligned)
        x -= w;
    GeomRect box = {x, y, w, THEME_PILL_H};
    draw_slice(DRAW_SLICE_PILL, box, THEME_ALPHA_OPAQUE);
    coin_draw(COIN_MEDIUM, x + THEME_PILL_COIN_PAD, y + (THEME_PILL_H - THEME_COIN_MEDIUM) / 2,
              shine);
    draw_text_vcenter(x + THEME_PILL_COIN_PAD + THEME_COIN_MEDIUM + THEME_TEXT_GAP, y, THEME_PILL_H,
                      THEME_TEXT_BUTTON, THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, num);
    return box;
}
