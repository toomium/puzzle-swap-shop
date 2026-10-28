#include "core/key_repeat.h"

#include <string.h>

void key_repeat_init(KeyRepeat *kr, uint32_t repeat_mask)
{
    kr->repeat_mask = repeat_mask;
    kr->blocked = 0;
    memset(kr->held, 0, sizeof(kr->held));
}

void key_repeat_reset(KeyRepeat *kr)
{
    for (unsigned bit = 0; bit < 32; bit++) {
        if (kr->held[bit])
            kr->blocked |= 1u << bit;
    }
    memset(kr->held, 0, sizeof(kr->held));
}

uint32_t key_repeat_update(KeyRepeat *kr, uint32_t held)
{
    uint32_t fire = 0;
    for (unsigned bit = 0; bit < 32; bit++) {
        uint32_t mask = 1u << bit;
        if (!(held & mask)) {
            kr->held[bit] = 0;
            kr->blocked &= ~mask;
            continue;
        }
        if (kr->blocked & mask)
            continue;

        uint16_t n = kr->held[bit];
        if (n == 0) {
            fire |= mask;
        } else if ((kr->repeat_mask & mask) && n >= KEY_REPEAT_DELAY &&
                   (n - KEY_REPEAT_DELAY) % KEY_REPEAT_INTERVAL == 0) {
            fire |= mask;
        }

        /* Keep the counter bounded but in the same repeat phase. */
        n++;
        if (n >= KEY_REPEAT_DELAY + KEY_REPEAT_INTERVAL)
            n -= KEY_REPEAT_INTERVAL;
        kr->held[bit] = n;
    }
    return fire;
}
