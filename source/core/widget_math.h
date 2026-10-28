#ifndef PSS_CORE_WIDGET_MATH_H
#define PSS_CORE_WIDGET_MATH_H

#include <stdint.h>

#include "core/geom.h"

/* Integer geometry shared by the widgets, kept out of ui/ so it is tested on the PC. */

/* Width of a progress fill: track_w * owned / total, 0 for total 0, full for owned >= total. */
int widget_progress_fill(int track_w, unsigned owned, unsigned total);

/* Cell `index` of a board with `cols` columns of `cell` pixels whose art starts at art.x/art.y. */
GeomRect widget_board_cell(GeomRect art, int cols, int cell, int index);

/*
 * r scaled around its center. Whole pixels: the grown or shrunk box keeps
 * its center within half a pixel and never gets smaller than 0.
 */
GeomRect widget_scale_rect(GeomRect r, float scale);

/* Bit `index` (piece number minus 1) of a piece mask; false beyond 63. */
int widget_mask_has(uint64_t mask, int index);

#endif
