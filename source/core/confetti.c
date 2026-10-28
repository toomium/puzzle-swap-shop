#include "core/confetti.h"

#define START_SPREAD 36 /* start frames 0..35 */
#define SWAY_PX      6
#define SWAY_PERIOD  40

static unsigned hash(unsigned index, unsigned salt)
{
    unsigned h = (index + 1) * 2654435761u + salt * 40503u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return h;
}

/* Triangle wave -SWAY_PX..SWAY_PX over SWAY_PERIOD frames. */
static int sway(unsigned t)
{
    int p = (int)(t % SWAY_PERIOD);
    int half = SWAY_PERIOD / 2;
    int v = p < half ? p : SWAY_PERIOD - p; /* 0..half */
    return (v * 2 * SWAY_PX) / half - SWAY_PX;
}

static int speed_q(unsigned index) /* quarter pixels per frame, 6..13 */
{
    return 6 + (int)(hash(index, 3) % 8);
}

bool confetti_at(unsigned frame, unsigned index, int width, int height, ConfettiBit *out)
{
    unsigned start = hash(index, 1) % START_SPREAD;
    if (frame < start)
        return false;
    unsigned t = frame - start;
    int y = -5 + (int)t * speed_q(index) / 4;
    if (y >= height)
        return false;
    int base = (int)(hash(index, 2) % (unsigned)width);
    out->x = base + sway(t + hash(index, 4) % SWAY_PERIOD);
    out->y = y;
    out->size = 3 + (int)(hash(index, 5) % 3);
    out->color = (int)(hash(index, 6) % CONFETTI_COLORS);
    return true;
}

unsigned confetti_frames(int height)
{
    /* Slowest piece: 6 quarter pixels per frame, latest start. */
    return START_SPREAD + (unsigned)((height + 5) * 4 / 6) + 1;
}
