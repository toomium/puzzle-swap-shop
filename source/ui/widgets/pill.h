#ifndef PSS_UI_WIDGETS_PILL_H
#define PSS_UI_WIDGETS_PILL_H

#include <stdbool.h>

#include "core/geom.h"

/*
 * Pills of the top screen: 22 px high, rounded,
 * with a soft inner shadow. Static; they sit in the front depth layer.
 */

/* Name pill: name in button size, count in body size ("28/40"). x is the left edge. */
GeomRect pill_name_draw(int x, int y, const char *name, const char *count);

/*
 * Coin pill: coin and number. x is the left edge, or the right edge if
 * right_aligned. `shine` is the glint opacity while the number counts.
 */
GeomRect pill_coin_draw(int x, int y, bool right_aligned, unsigned coins, unsigned char shine);

#endif
