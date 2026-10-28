#ifndef PSS_UI_WIDGETS_BOARD_H
#define PSS_UI_WIDGETS_BOARD_H

#include <stdint.h>

#include "core/geom.h"

/*
 * The puzzle board of the top screen: a
 * white frame around the picture; pieces that are not owned are covered
 * with paper and their number, selected ones with a green frame. Piece i is
 * bit i of each mask (piece number minus 1).
 *
 * New pieces (S6) are owned already but pop in one after another, each
 * shrinking from 1.3 to 1.0: `fresh` marks them, `fresh_frame` counts frames
 * since the animation began. Pass fresh 0 outside of S6.
 */

typedef struct {
    int pieces; /* 15, 24 or 40 */
    uint64_t owned;
    uint64_t selected;
    uint64_t rare;
    uint64_t fresh;
    unsigned fresh_frame;
} BoardSpec;

/* Where the picture lies for a layout; empty for an unknown piece count. */
GeomRect board_art_rect(int pieces);

/* Draws the board; returns the picture rectangle. */
GeomRect board_draw(const BoardSpec *spec);

#endif
