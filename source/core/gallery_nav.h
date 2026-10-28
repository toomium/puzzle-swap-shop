#ifndef PSS_CORE_GALLERY_NAV_H
#define PSS_CORE_GALLERY_NAV_H

#include <stdbool.h>

#include "core/focus.h"

/*
 * Paging and focus of the puzzle gallery:
 * 3 x 2 cards per page, L and R turn the page, the focus index counts over
 * all entries in gallery order.
 */

#define GALLERY_COLS     3
#define GALLERY_ROWS     2
#define GALLERY_PER_PAGE (GALLERY_COLS * GALLERY_ROWS)

/* Pages for `count` entries; at least 1. */
unsigned gallery_pages(unsigned count);

unsigned gallery_page_of(unsigned index);

/* Entries on a page: 6, fewer on the last page, 0 beyond it. */
unsigned gallery_page_items(unsigned count, unsigned page);

/*
 * Moves the focus `delta` pages (-1 or 1) and keeps its place in the grid,
 * or the last card of a short last page. Returns false at the first or last
 * page, with the index unchanged.
 */
bool gallery_page_step(unsigned *index, int delta, unsigned count);

/* Moves within the page; edges are blocked (L and R turn pages). */
FocusMove gallery_move(unsigned *index, FocusDir dir, unsigned count);

#endif
