#include "core/focus.h"

void focus_init(FocusState *f, int index, bool visible)
{
    f->index = index;
    f->visible = visible;
}

uint32_t focus_update(FocusState *f, uint32_t keys, uint32_t wake_mask, bool touch_started)
{
    if (touch_started) {
        f->visible = false;
        /* Keys pressed in the same frame as a touch still count as touch use. */
        return keys & ~wake_mask;
    }
    if (!f->visible && (keys & wake_mask)) {
        f->visible = true;
        return keys & ~wake_mask;
    }
    return keys;
}

static FocusMove leave(FocusDir dir, unsigned exits)
{
    if (!(exits & FOCUS_EXITS(dir)))
        return FOCUS_BLOCKED;
    switch (dir) {
    case FOCUS_UP:
        return FOCUS_EXIT_UP;
    case FOCUS_DOWN:
        return FOCUS_EXIT_DOWN;
    case FOCUS_LEFT:
        return FOCUS_EXIT_LEFT;
    case FOCUS_RIGHT:
    default:
        return FOCUS_EXIT_RIGHT;
    }
}

FocusMove focus_grid_move(int *index, int cols, int count, FocusDir dir, unsigned exits)
{
    if (cols <= 0 || count <= 0 || *index < 0 || *index >= count)
        return FOCUS_BLOCKED;

    int row = *index / cols;
    int col = *index % cols;
    int last_row = (count - 1) / cols;

    switch (dir) {
    case FOCUS_UP:
        if (row == 0)
            return leave(dir, exits);
        *index -= cols;
        return FOCUS_MOVED;
    case FOCUS_DOWN:
        if (row == last_row)
            return leave(dir, exits);
        *index = *index + cols < count ? *index + cols : count - 1;
        return FOCUS_MOVED;
    case FOCUS_LEFT:
        if (col == 0)
            return leave(dir, exits);
        *index -= 1;
        return FOCUS_MOVED;
    case FOCUS_RIGHT:
    default:
        if (col == cols - 1 || *index == count - 1)
            return leave(dir, exits);
        *index += 1;
        return FOCUS_MOVED;
    }
}
