#ifndef PSS_UI_WIDGETS_TILE_H
#define PSS_UI_WIDGETS_TILE_H

#include <stdbool.h>

#include "core/geom.h"

/*
 * Piece in the touch grid of the bottom screen.
 * Missing: white with its number. Selected: green, white number, check,
 * raised by a pixel. Owned: its part of the picture, brightened, with a
 * green check; not tappable. Rare pieces carry a pink dot. Locked pieces
 * (not affordable) are drawn faded. `bump` is the scale from
 * anim_piece_select_scale() right after a tap.
 */

typedef enum {
    TILE_MISSING,
    TILE_SELECTED,
    TILE_OWNED,
} TileState;

typedef struct {
    TileState state;
    int number; /* piece number as the player sees it, 1-based */
    bool rare;
    bool locked;
    GeomRect art; /* where the whole picture would be drawn, for TILE_OWNED */
} TileSpec;

typedef struct {
    bool focus;
    float bump; /* 1.0 at rest */
} TileView;

void tile_draw(GeomRect box, const TileSpec *spec, const TileView *view);

#endif
