#ifndef PSS_UI_WIDGETS_BUTTON_H
#define PSS_UI_WIDGETS_BUTTON_H

#include <stdbool.h>

#include "core/geom.h"

/*
 * Button: 30 px high, an optional button glyph at the
 * left, bold-size label. Secondary is white, primary green with white text.
 * States: normal, pressed (one pixel lower, no shadow), disabled (45 %), focus.
 */

typedef enum {
    BUTTON_SECONDARY,
    BUTTON_PRIMARY,
} ButtonKind;

typedef struct {
    ButtonKind kind;
    const char *glyph; /* DRAW_GLYPH_*, or NULL */
    const char *label;
} ButtonSpec;

typedef struct {
    bool disabled;
    bool focus;
    int press_px; /* from core/press: 0 at rest, 1 while pressed */
} ButtonView;

int button_width(const ButtonSpec *spec);

/* Box of a button whose top left is (x, y); also the touch target. */
GeomRect button_box(const ButtonSpec *spec, int x, int y);

/* Draws the button and returns its box. */
GeomRect button_draw(const ButtonSpec *spec, int x, int y, const ButtonView *view);

#endif
