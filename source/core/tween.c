#include "core/tween.h"

void tween_start(Tween *t, float from, float to, uint16_t duration, uint16_t delay, Easing curve)
{
    t->from = from;
    t->to = to;
    t->duration = duration;
    t->delay = delay;
    t->elapsed = 0;
    t->curve = curve;
}

void tween_finish(Tween *t)
{
    t->elapsed = (uint16_t)(t->delay + t->duration);
}

bool tween_done(const Tween *t)
{
    return t->elapsed >= (unsigned)t->delay + t->duration;
}

bool tween_step(Tween *t)
{
    if (tween_done(t))
        return false;
    t->elapsed++;
    return !tween_done(t);
}

float tween_progress(const Tween *t)
{
    if (t->elapsed <= t->delay)
        return t->duration == 0 && tween_done(t) ? 1.0f : 0.0f;
    if (tween_done(t))
        return 1.0f;
    return (float)(t->elapsed - t->delay) / (float)t->duration;
}

float tween_value(const Tween *t)
{
    float e = easing_apply(t->curve, tween_progress(t));
    return t->from + (t->to - t->from) * e;
}

static int round_px(float v)
{
    return v >= 0.0f ? (int)(v + 0.5f) : -(int)(-v + 0.5f);
}

int tween_value_px(const Tween *t)
{
    return round_px(tween_value(t));
}

float tween_bump(unsigned frame, unsigned duration, float base, float peak)
{
    if (duration == 0 || frame >= duration)
        return base;
    unsigned half = duration / 2;
    if (half == 0)
        half = 1;
    if (frame < half) {
        float e = easing_out_cubic((float)frame / (float)half);
        return base + (peak - base) * e;
    }
    float e = easing_out_back((float)(frame - half) / (float)(duration - half));
    return peak + (base - peak) * e;
}

float tween_stagger(unsigned frame, unsigned index, unsigned duration, unsigned offset)
{
    unsigned start = index * offset;
    if (frame <= start)
        return 0.0f;
    if (duration == 0 || frame - start >= duration)
        return 1.0f;
    return (float)(frame - start) / (float)duration;
}

static unsigned diff_u(unsigned a, unsigned b)
{
    return a > b ? a - b : b - a;
}

unsigned tween_count_frames(unsigned from, unsigned to, unsigned step, unsigned max_frames)
{
    unsigned frames = diff_u(from, to) * step;
    return frames > max_frames ? max_frames : frames;
}

unsigned tween_count_value(unsigned from, unsigned to, unsigned frame, unsigned step,
                           unsigned max_frames)
{
    unsigned total = tween_count_frames(from, to, step, max_frames);
    if (frame >= total)
        return to;
    unsigned diff = diff_u(from, to);
    /* counted = floor(diff * frame / total); total > 0 here since frame < total. */
    unsigned counted = (unsigned)((unsigned long)diff * frame / total);
    return to >= from ? from + counted : from - counted;
}

bool tween_count_tick(unsigned from, unsigned to, unsigned frame, unsigned step,
                      unsigned max_frames, unsigned every)
{
    if (frame == 0 || every == 0)
        return false;
    unsigned prev = diff_u(from, tween_count_value(from, to, frame - 1, step, max_frames));
    unsigned cur = diff_u(from, tween_count_value(from, to, frame, step, max_frames));
    return cur / every != prev / every;
}

int tween_scroll_px(unsigned frame, unsigned num, unsigned den, unsigned period)
{
    if (den == 0 || period == 0)
        return 0;
    unsigned long px = (unsigned long)frame * num / den;
    return (int)(px % period);
}
