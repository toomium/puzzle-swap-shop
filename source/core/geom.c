#include "core/geom.h"

bool geom_rect_empty(GeomRect r)
{
    return r.w <= 0 || r.h <= 0;
}

bool geom_rect_intersect(GeomRect a, GeomRect b, GeomRect *out)
{
    int x0 = a.x > b.x ? a.x : b.x;
    int y0 = a.y > b.y ? a.y : b.y;
    int x1 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    int y1 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);

    if (geom_rect_empty(a) || geom_rect_empty(b) || x1 <= x0 || y1 <= y0) {
        *out = (GeomRect){0, 0, 0, 0};
        return false;
    }
    *out = (GeomRect){x0, y0, x1 - x0, y1 - y0};
    return true;
}

bool geom_rect_contains(GeomRect r, int x, int y)
{
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

GeomRect geom_rect_inset(GeomRect r, int d)
{
    return (GeomRect){r.x - d, r.y - d, r.w + 2 * d, r.h + 2 * d};
}

GeomScissor geom_scissor(GeomRect clip, int screen_w, int screen_h)
{
    GeomRect c;
    if (!geom_rect_intersect(clip, (GeomRect){0, 0, screen_w, screen_h}, &c))
        return (GeomScissor){0, 0, 0, 0};

    /* Framebuffer x = screen_h - screen y, framebuffer y = screen_w - screen x. */
    return (GeomScissor){
        .left = (unsigned)(screen_h - (c.y + c.h)),
        .top = (unsigned)(screen_w - (c.x + c.w)),
        .right = (unsigned)(screen_h - c.y),
        .bottom = (unsigned)(screen_w - c.x),
    };
}

/* Fits two fixed borders into a length; shrinks them in proportion if needed. */
static void fit_borders(int len, int *a, int *b)
{
    int sum = *a + *b;
    if (len >= sum)
        return;
    if (len <= 0 || sum <= 0) {
        *a = 0;
        *b = 0;
        return;
    }
    int na = (*a * len) / sum;
    *b = len - na;
    *a = na;
}

int geom_nine_slice(int src_w, int src_h, GeomInsets in, GeomRect dst,
                    GeomPatch out[GEOM_NINE_SLICE_MAX])
{
    if (src_w <= 0 || src_h <= 0 || geom_rect_empty(dst))
        return 0;

    /* Source columns and rows. */
    int sx[4] = {0, in.left, src_w - in.right, src_w};
    int sy[4] = {0, in.top, src_h - in.bottom, src_h};

    int dl = in.left, dr = in.right, dt = in.top, db = in.bottom;
    fit_borders(dst.w, &dl, &dr);
    fit_borders(dst.h, &dt, &db);
    int dx[4] = {dst.x, dst.x + dl, dst.x + dst.w - dr, dst.x + dst.w};
    int dy[4] = {dst.y, dst.y + dt, dst.y + dst.h - db, dst.y + dst.h};

    int n = 0;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            GeomRect s = {sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]};
            GeomRect d = {dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row]};
            if (geom_rect_empty(s) || geom_rect_empty(d))
                continue;
            out[n].src = s;
            out[n].dst = d;
            n++;
        }
    }
    return n;
}
