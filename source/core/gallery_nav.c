#include "core/gallery_nav.h"

unsigned gallery_pages(unsigned count)
{
    unsigned pages = (count + GALLERY_PER_PAGE - 1) / GALLERY_PER_PAGE;
    return pages ? pages : 1;
}

unsigned gallery_page_of(unsigned index)
{
    return index / GALLERY_PER_PAGE;
}

unsigned gallery_page_items(unsigned count, unsigned page)
{
    unsigned first = page * GALLERY_PER_PAGE;
    if (first >= count)
        return 0;
    unsigned left = count - first;
    return left < GALLERY_PER_PAGE ? left : GALLERY_PER_PAGE;
}

bool gallery_page_step(unsigned *index, int delta, unsigned count)
{
    int page = (int)gallery_page_of(*index) + delta;
    if (page < 0 || (unsigned)page >= gallery_pages(count))
        return false;
    unsigned local = *index % GALLERY_PER_PAGE;
    unsigned items = gallery_page_items(count, (unsigned)page);
    if (items == 0)
        return false;
    if (local >= items)
        local = items - 1;
    *index = (unsigned)page * GALLERY_PER_PAGE + local;
    return true;
}

FocusMove gallery_move(unsigned *index, FocusDir dir, unsigned count)
{
    unsigned page = gallery_page_of(*index);
    int local = (int)(*index % GALLERY_PER_PAGE);
    FocusMove r =
        focus_grid_move(&local, GALLERY_COLS, (int)gallery_page_items(count, page), dir, 0);
    *index = page * GALLERY_PER_PAGE + (unsigned)local;
    return r;
}
