#ifndef PSS_CORE_FOCUS_H
#define PSS_CORE_FOCUS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Focus model. The focus ring is only shown
 * while the user works with keys. Touching the screen hides it; the next key
 * press shows it again at the last position without acting on it, so a
 * hidden focus never moves or confirms anything by surprise.
 */

typedef struct {
    int index;    /* focused element */
    bool visible; /* ring drawn */
} FocusState;

typedef enum {
    FOCUS_UP,
    FOCUS_DOWN,
    FOCUS_LEFT,
    FOCUS_RIGHT,
} FocusDir;

typedef enum {
    FOCUS_MOVED = 0,
    FOCUS_BLOCKED, /* edge reached, nothing to leave to; index unchanged */
    FOCUS_EXIT_UP, /* left the grid; the caller moves focus elsewhere */
    FOCUS_EXIT_DOWN,
    FOCUS_EXIT_LEFT,
    FOCUS_EXIT_RIGHT,
} FocusMove;

void focus_init(FocusState *f, int index, bool visible);

/*
 * Feed one frame of input. `keys` are the keys that fired this frame (after
 * key repeat), `wake_mask` the keys that reveal a hidden ring (directions
 * and A; B, L, R and START keep working directly), `touch_started` whether
 * the stylus touched down this frame.
 *
 * Returns the keys the scene should act on: the wake keys are swallowed on
 * the frame that reveals the ring.
 */
uint32_t focus_update(FocusState *f, uint32_t keys, uint32_t wake_mask, bool touch_started);

/*
 * Moves within a row-major grid of `count` cells and `cols` columns; the
 * last row may be shorter. Moving down into a shorter last row lands on its
 * last cell. Leaving the grid returns FOCUS_EXIT_* when the matching
 * `exits` bit (1 << dir) is set, else FOCUS_BLOCKED.
 */
FocusMove focus_grid_move(int *index, int cols, int count, FocusDir dir, unsigned exits);

#define FOCUS_EXITS(dir) (1u << (dir))

#endif
