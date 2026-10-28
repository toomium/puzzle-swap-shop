#include "core/list_nav.h"

void list_nav_init(ListNav *l)
{
    l->index = 0;
    l->first = 0;
}

void list_nav_clamp(ListNav *l, unsigned count, unsigned visible)
{
    if (count == 0) {
        l->index = 0;
        l->first = 0;
        return;
    }
    if (l->index >= count)
        l->index = count - 1;
    if (l->index < l->first)
        l->first = l->index;
    if (visible > 0 && l->index >= l->first + visible)
        l->first = l->index - visible + 1;
    if (count > visible && l->first > count - visible)
        l->first = count - visible;
    if (count <= visible)
        l->first = 0;
}

bool list_nav_move(ListNav *l, int delta, unsigned count, unsigned visible)
{
    if (count == 0)
        return false;
    if (delta < 0 && l->index < (unsigned)-delta)
        return false;
    if (delta > 0 && l->index + (unsigned)delta >= count)
        return false;
    l->index = (unsigned)((int)l->index + delta);
    list_nav_clamp(l, count, visible);
    return true;
}

bool list_nav_page(ListNav *l, int direction, unsigned count, unsigned visible)
{
    if (count == 0 || visible == 0)
        return false;
    unsigned before = l->index;
    if (direction < 0) {
        l->index = l->index > visible ? l->index - visible : 0;
        l->first = l->first > visible ? l->first - visible : 0;
    } else {
        l->index = l->index + visible < count ? l->index + visible : count - 1;
        l->first += visible;
    }
    list_nav_clamp(l, count, visible);
    return l->index != before;
}

bool list_nav_more_above(const ListNav *l)
{
    return l->first > 0;
}

bool list_nav_more_below(const ListNav *l, unsigned count, unsigned visible)
{
    return l->first + visible < count;
}
