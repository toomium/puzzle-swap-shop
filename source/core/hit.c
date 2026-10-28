#include "core/hit.h"

bool hit_rect_contains(HitRect r, int px, int py)
{
    return r.w > 0 && r.h > 0 && px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

HitRect hit_rect_expand(HitRect r, int min_w, int min_h)
{
    if (r.w < min_w) {
        r.x -= (min_w - r.w) / 2;
        r.w = min_w;
    }
    if (r.h < min_h) {
        r.y -= (min_h - r.h) / 2;
        r.h = min_h;
    }
    return r;
}

int hit_test(const HitRect *rects, int count, int px, int py)
{
    for (int i = count - 1; i >= 0; i--) {
        if (hit_rect_contains(rects[i], px, py))
            return i;
    }

    int best = -1;
    long best_d = 0;
    for (int i = count - 1; i >= 0; i--) {
        HitRect r = rects[i];
        if (r.w <= 0 || r.h <= 0)
            continue;
        if (!hit_rect_contains(hit_rect_expand(r, HIT_MIN_SIZE, HIT_MIN_SIZE), px, py))
            continue;
        /* Doubled coordinates keep the center exact for odd sizes. */
        long dx = 2L * px - (2L * r.x + r.w);
        long dy = 2L * py - (2L * r.y + r.h);
        long d = dx * dx + dy * dy;
        if (best < 0 || d < best_d) {
            best = i;
            best_d = d;
        }
    }
    return best;
}

/* Cell along one axis; gaps are split between the two neighbors. */
static int grid_axis(int origin, int n, int cell, int gap, int p)
{
    int pitch = cell + gap;
    int total = n * cell + (n - 1) * gap;
    int rel = p - origin;
    if (n <= 0 || cell <= 0 || rel < 0 || rel >= total)
        return -1;
    int idx = rel / pitch;
    int in = rel % pitch;
    if (in >= cell && in - cell >= gap / 2)
        idx++;
    return idx < n ? idx : n - 1;
}

int hit_grid(int x0, int y0, int cols, int rows, int cell_w, int cell_h, int gap_x, int gap_y,
             int px, int py)
{
    int c = grid_axis(x0, cols, cell_w, gap_x, px);
    int r = grid_axis(y0, rows, cell_h, gap_y, py);
    if (c < 0 || r < 0)
        return -1;
    return r * cols + c;
}

void hit_tap_reset(HitTap *tap)
{
    tap->target = -1;
    tap->inside = false;
    tap->active = false;
}

int hit_tap_update(HitTap *tap, bool touching, int hit)
{
    if (touching) {
        if (!tap->active) {
            /* New touch. A touch that starts on nothing stays dead until release. */
            tap->active = true;
            tap->target = hit;
            tap->inside = hit >= 0;
        } else {
            tap->inside = tap->target >= 0 && hit == tap->target;
        }
        return -1;
    }

    int activated = tap->target >= 0 && tap->inside ? tap->target : -1;
    hit_tap_reset(tap);
    return activated;
}

int hit_tap_pressed(const HitTap *tap)
{
    return tap->target >= 0 && tap->inside ? tap->target : -1;
}
