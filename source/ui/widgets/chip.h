#ifndef PSS_UI_WIDGETS_CHIP_H
#define PSS_UI_WIDGETS_CHIP_H

#include <stdbool.h>

#include "core/geom.h"

/* Chip: 20 px high, glyph at the left, small label. States: normal, pressed.
 */

typedef struct {
    const char *glyph; /* DRAW_GLYPH_*, or NULL */
    const char *label;
} ChipSpec;

typedef struct {
    bool pressed;
    bool focus;
} ChipView;

int chip_width(const ChipSpec *spec);
GeomRect chip_box(const ChipSpec *spec, int x, int y);
GeomRect chip_draw(const ChipSpec *spec, int x, int y, const ChipView *view);

#endif
