#ifndef PSS_UI_WIDGETS_ART_H
#define PSS_UI_WIDGETS_ART_H

#include "core/geom.h"
#include "ui/theme.h"

/*
 * The picture of a puzzle: the real one read from the console if the
 * panel has one, else a placeholder, a meadow under the sky. Boards, owned
 * pieces and card previews all call this, so they always agree.
 *
 * Which panel is drawn is set with art_use(); scenes call it before they
 * draw a board, the pieces or a card, and it stays until the next call.
 * ART_PLACEHOLDER (the start value) always draws the placeholder.
 */

#define ART_PLACEHOLDER (-1)

void art_use(int panel);

/* Drawn to fill `r` and clipped to it. */
void art_draw(GeomRect r, unsigned char alpha);

#endif
