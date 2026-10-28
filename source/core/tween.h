#ifndef PSS_CORE_TWEEN_H
#define PSS_CORE_TWEEN_H

#include <stdbool.h>
#include <stdint.h>

#include "core/easing.h"

/*
 * Frame-based tweens. Time is counted in
 * frames at 60 fps, never in seconds, so motion is identical on every run.
 */

typedef struct {
    float from;
    float to;
    uint16_t delay;    /* frames before the tween starts moving */
    uint16_t duration; /* frames from start to end; 0 jumps to `to` */
    uint16_t elapsed;  /* frames advanced so far, including the delay */
    Easing curve;
} Tween;

void tween_start(Tween *t, float from, float to, uint16_t duration, uint16_t delay, Easing curve);

/* Jumps to the end value. */
void tween_finish(Tween *t);

/* Advances one frame. Returns true while the tween is still running. */
bool tween_step(Tween *t);

bool tween_done(const Tween *t);

/* Linear progress 0..1, ignoring the curve. */
float tween_progress(const Tween *t);

/* Current value with the curve applied. */
float tween_value(const Tween *t);

/* Current value rounded to the nearest whole pixel. */
int tween_value_px(const Tween *t);

/*
 * Out-and-back bump, e.g. piece selection 1.00 -> 1.08 -> 1.00: the first
 * half eases out to `peak`, the second half springs back to `base` with
 * ease-out-back. Returns `base` once frame >= duration.
 */
float tween_bump(unsigned frame, unsigned duration, float base, float peak);

/*
 * Linear 0..1 progress of item `index` in a staggered sequence where each
 * item starts `offset` frames after the previous one and runs `duration`.
 */
float tween_stagger(unsigned frame, unsigned index, unsigned duration, unsigned offset);

/*
 * Coin counter: one coin every `step` frames, but the whole count never
 * takes longer than `max_frames`; larger differences speed up.
 */
unsigned tween_count_frames(unsigned from, unsigned to, unsigned step, unsigned max_frames);
unsigned tween_count_value(unsigned from, unsigned to, unsigned frame, unsigned step,
                           unsigned max_frames);

/*
 * True if the counter passed a multiple of `every` counted coins between
 * frame - 1 and frame (for the coin sound every 5 coins).
 */
bool tween_count_tick(unsigned from, unsigned to, unsigned frame, unsigned step,
                      unsigned max_frames, unsigned every);

/* Whole-pixel offset of an endless scroll moving `num`/`den` px per frame, wrapped to `period`. */
int tween_scroll_px(unsigned frame, unsigned num, unsigned den, unsigned period);

#endif
