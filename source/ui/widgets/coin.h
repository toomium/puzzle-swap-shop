#ifndef PSS_UI_WIDGETS_COIN_H
#define PSS_UI_WIDGETS_COIN_H

#include "core/geom.h"
#include "ui/theme.h"

/*
 * Play coin: 12, 16 or 20 px, static; while a counter
 * runs it glints briefly. A price tag is a 20 px coin with a number.
 */

typedef enum {
    COIN_SMALL,  /* 12 px */
    COIN_MEDIUM, /* 16 px */
    COIN_LARGE,  /* 20 px */
} CoinSize;

int coin_size_px(CoinSize size);

/* Coin with its top left at (x, y); `shine` 0..255 is the opacity of the glint. */
void coin_draw(CoinSize size, int x, int y, u8 shine);

/* A 20 px coin followed by a number of Button size, in a box of `h`; returns the width. */
int coin_price_draw(int x, int y, int h, unsigned price, ThemeColor ink);

#endif
