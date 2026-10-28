#ifndef PSS_CORE_KEY_REPEAT_H
#define PSS_CORE_KEY_REPEAT_H

#include <stdint.h>

/*
 * Key repeat: a held key fires once when
 * pressed, again after KEY_REPEAT_DELAY frames and then every
 * KEY_REPEAT_INTERVAL frames. Keys are a 32-bit mask, one bit per key, so
 * the same code works with libctru's KEY_* bits and in host tests.
 */

#define KEY_REPEAT_DELAY    18
#define KEY_REPEAT_INTERVAL 5

typedef struct {
    uint32_t repeat_mask; /* keys that repeat; all others fire only on press */
    uint16_t held[32];    /* frames each key has been held, 0 = not held */
    uint32_t blocked;     /* held at a reset: silent until released */
} KeyRepeat;

void key_repeat_init(KeyRepeat *kr, uint32_t repeat_mask);

/*
 * After a screen change: keys held right now stay silent until they are
 * released, so the press that closed one screen does not act on the next.
 * Keys pressed afterwards fire normally.
 */
void key_repeat_reset(KeyRepeat *kr);

/*
 * Feed the keys held this frame (once per frame). Returns the keys that fire:
 * every newly pressed key plus repeating keys on their repeat frames.
 */
uint32_t key_repeat_update(KeyRepeat *kr, uint32_t held);

#endif
