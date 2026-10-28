#ifndef PSS_CORE_LIST_NAV_H
#define PSS_CORE_LIST_NAV_H

#include <stdbool.h>

/*
 * Focus and scrolling of a vertical list that shows `visible` rows of
 * `count`. `first` is the top row; it
 * follows the focus so that the focused row is always inside the window.
 */

typedef struct {
    unsigned index; /* focused row */
    unsigned first; /* top row of the window */
} ListNav;

void list_nav_init(ListNav *l);

/* Keeps index below count and first so that the window holds index and no empty space. */
void list_nav_clamp(ListNav *l, unsigned count, unsigned visible);

/* Moves the focus by delta rows. Returns false at the first or last row. */
bool list_nav_move(ListNav *l, int delta, unsigned count, unsigned visible);

/* Moves by a whole window (L and R). Returns false if the focus did not change. */
bool list_nav_page(ListNav *l, int direction, unsigned count, unsigned visible);

/* True if there are rows above or below the window. */
bool list_nav_more_above(const ListNav *l);
bool list_nav_more_below(const ListNav *l, unsigned count, unsigned visible);

#endif
