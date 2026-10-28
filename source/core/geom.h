#ifndef PSS_CORE_GEOM_H
#define PSS_CORE_GEOM_H

#include <stdbool.h>

/*
 * Integer rectangles for drawing: clipping, the GPU scissor box and the
 * patches of a 9-slice image. Coordinates are 3DS screen pixels, origin top
 * left, x to the right, y down.
 */

typedef struct {
    int x, y, w, h;
} GeomRect;

typedef struct {
    int left, top, right, bottom;
} GeomInsets;

/* GPU scissor box in framebuffer coordinates; right and bottom exclusive. */
typedef struct {
    unsigned left, top, right, bottom;
} GeomScissor;

/* One source/destination pair of a 9-slice. */
typedef struct {
    GeomRect src;
    GeomRect dst;
} GeomPatch;

#define GEOM_NINE_SLICE_MAX 9

bool geom_rect_empty(GeomRect r);

/* Intersection of a and b. Returns false and an empty rectangle if they do not overlap. */
bool geom_rect_intersect(GeomRect a, GeomRect b, GeomRect *out);

bool geom_rect_contains(GeomRect r, int x, int y);

/* r grown by d pixels on every side (d may be negative). */
GeomRect geom_rect_inset(GeomRect r, int d);

/*
 * Scissor box for a clip rectangle on a screen of screen_w x screen_h.
 * The 3DS framebuffers are rotated by 90 degrees, so screen x runs along the
 * framebuffer's long side and both axes are mirrored. The clip is first cut
 * to the screen.
 */
GeomScissor geom_scissor(GeomRect clip, int screen_w, int screen_h);

/*
 * Splits a src_w x src_h image with the given fixed borders into up to nine
 * patches that fill dst. Corners keep their size, edges stretch along one
 * axis, the center along both. If dst is narrower or lower than the borders,
 * the borders shrink proportionally. Empty patches are left out.
 * Returns the number of patches written to out.
 */
int geom_nine_slice(int src_w, int src_h, GeomInsets in, GeomRect dst,
                    GeomPatch out[GEOM_NINE_SLICE_MAX]);

#endif
