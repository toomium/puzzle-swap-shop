#include "ui/widgets/coin.h"

#include <stdio.h>

#include "ui/draw.h"

static const DrawSprite s_sprite[] = {DRAW_COIN_12, DRAW_COIN_16, DRAW_COIN_20};

int coin_size_px(CoinSize size)
{
    return draw_sprite_width(s_sprite[size]);
}

void coin_draw(CoinSize size, int x, int y, u8 shine)
{
    int px = coin_size_px(size);
    draw_sprite(s_sprite[size], x, y, THEME_ALPHA_OPAQUE);
    if (shine > 0) {
        /* The glint sits on the coin's upper right and is centered on the corner. */
        int sx = x + px - draw_sprite_width(DRAW_SPARK) * 2 / 3;
        int sy = y - draw_sprite_height(DRAW_SPARK) / 3;
        draw_sprite(DRAW_SPARK, sx, sy, shine);
    }
}

int coin_price_draw(int x, int y, int h, unsigned price, ThemeColor ink)
{
    char num[12];
    snprintf(num, sizeof(num), "%u", price);
    int px = coin_size_px(COIN_LARGE);
    coin_draw(COIN_LARGE, x, y + (h - px) / 2, 0);
    int tx = x + px + THEME_PRICE_GAP;
    int w = draw_text_vcenter(tx, y, h, THEME_TEXT_BUTTON, ink, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT,
                              num);
    return tx + w - x;
}
