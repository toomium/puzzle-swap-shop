#ifndef PSS_CORE_COIN_COUNTER_H
#define PSS_CORE_COIN_COUNTER_H

#include <stdbool.h>

/*
 * The coin display counting from one value to another: one coin every `step` frames, the whole count at most
 * `max_frames` long. Wraps the tween_count_* helpers with a frame counter.
 */

typedef struct {
    unsigned from;
    unsigned to;
    unsigned frame;
    unsigned step;
    unsigned max_frames;
    unsigned sound_every;
} CoinCounter;

/* Starts at rest on `value`. */
void coin_counter_init(CoinCounter *c, unsigned value, unsigned step, unsigned max_frames,
                       unsigned sound_every);

/* Counts from the displayed value to `to`; counting again mid-way starts from the shown value. */
void coin_counter_set(CoinCounter *c, unsigned to);

/* Advance one frame. Returns true if the coin sound is due on this frame. */
bool coin_counter_step(CoinCounter *c);

unsigned coin_counter_value(const CoinCounter *c);
bool coin_counter_done(const CoinCounter *c);

/* True on frames where the display just changed, for the brief shine of the coin. */
bool coin_counter_shining(const CoinCounter *c);

#endif
