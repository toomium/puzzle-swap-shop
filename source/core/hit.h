#ifndef PSS_CORE_HIT_H
#define PSS_CORE_HIT_H

#include <stdbool.h>

/*
 * Touch hit testing in screen pixels.
 * Rectangles are half-open: x <= px < x + w.
 */

/* Minimum touch target edge. */
#define HIT_MIN_SIZE 28

typedef struct {
    int x, y, w, h;
} HitRect;

bool hit_rect_contains(HitRect r, int px, int py);

/* Grows r around its center so both edges are at least min_w and min_h. */
HitRect hit_rect_expand(HitRect r, int min_w, int min_h);

/*
 * Index of the rectangle at (px, py), or -1. Exact hits win, the last one
 * in the array (drawn on top) first. Otherwise a rectangle smaller than
 * HIT_MIN_SIZE counts as grown to that size; among those the one whose
 * center is closest wins. Rectangles with w or h <= 0 are disabled.
 */
int hit_test(const HitRect *rects, int count, int px, int py);

/*
 * Cell index in a grid of cols x rows cells of cell_w x cell_h with gaps
 * between them, or -1 outside the grid. A touch in a gap counts for the
 * nearer neighbor, so the raster has no dead spots.
 */
int hit_grid(int x0, int y0, int cols, int rows, int cell_w, int cell_h, int gap_x, int gap_y,
             int px, int py);

/*
 * Tap tracking: a target is activated when the stylus is lifted while still
 * over the target it first touched. Sliding off cancels the press, sliding
 * back on re-arms it, like buttons in the system menus.
 */
typedef struct {
    int target;  /* target touched first, -1 = none */
    bool inside; /* stylus currently over that target */
    bool active; /* a touch is in progress */
} HitTap;

void hit_tap_reset(HitTap *tap);

/*
 * Call once per frame. `touching` is whether the screen is touched, `hit`
 * the target under the stylus this frame (ignored when not touching).
 * Returns the activated target on release, otherwise -1.
 */
int hit_tap_update(HitTap *tap, bool touching, int hit);

/* Target to draw as pressed this frame, or -1. */
int hit_tap_pressed(const HitTap *tap);

#endif
