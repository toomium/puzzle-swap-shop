#ifndef PSS_UI_WIDGETS_CARD_H
#define PSS_UI_WIDGETS_CARD_H

#include <stdbool.h>

#include "core/geom.h"

/*
 * Cards. A plain card is a white rounded panel with a
 * 1 px line and a 2 px shadow; the caller draws its content. A puzzle card
 * (96 x 78) shows the picture, name, progress and count; a completed puzzle
 * gets the star on its picture, and a panel that is not on the console is drawn at 40 %.
 */

typedef struct {
    bool focus;
    bool available; /* false: faded, not tappable */
    unsigned char alpha;
} CardView;

void card_draw(GeomRect box, const CardView *view);

typedef struct {
    const char *name;
    unsigned owned;
    unsigned total;
} PuzzleCardSpec;

/* done and unavailable follow from the spec and view.available. */
void puzzle_card_draw(GeomRect box, const PuzzleCardSpec *spec, bool focus, bool available);

#endif
