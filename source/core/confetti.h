#ifndef PSS_CORE_CONFETTI_H
#define PSS_CORE_CONFETTI_H

#include <stdbool.h>

/*
 * Confetti for a puzzle that was just completed. Every piece falls from the top edge with its own start frame,
 * speed and sway, computed from its index alone, so the effect is the same
 * on every run and needs no state.
 */

#define CONFETTI_COUNT  40
#define CONFETTI_COLORS 4 /* the scene maps 0..3 to theme colors */

typedef struct {
    int x, y;  /* top left, whole pixels */
    int size;  /* edge in pixels, 3..5 */
    int color; /* 0..CONFETTI_COLORS-1 */
} ConfettiBit;

/*
 * Piece `index` at `frame` frames after the effect began, on a screen
 * `width` x `height`. False while it has not started yet or after it left
 * the bottom edge.
 */
bool confetti_at(unsigned frame, unsigned index, int width, int height, ConfettiBit *out);

/* Frame after which no piece is visible any more for a screen of this height. */
unsigned confetti_frames(int height);

#endif
