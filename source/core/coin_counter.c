#include "core/coin_counter.h"

#include "core/tween.h"

void coin_counter_init(CoinCounter *c, unsigned value, unsigned step, unsigned max_frames,
                       unsigned sound_every)
{
    c->from = value;
    c->to = value;
    c->frame = 0;
    c->step = step;
    c->max_frames = max_frames;
    c->sound_every = sound_every;
}

unsigned coin_counter_value(const CoinCounter *c)
{
    return tween_count_value(c->from, c->to, c->frame, c->step, c->max_frames);
}

void coin_counter_set(CoinCounter *c, unsigned to)
{
    c->from = coin_counter_value(c);
    c->to = to;
    c->frame = 0;
}

bool coin_counter_step(CoinCounter *c)
{
    if (coin_counter_done(c))
        return false;
    c->frame++;
    return tween_count_tick(c->from, c->to, c->frame, c->step, c->max_frames, c->sound_every);
}

bool coin_counter_done(const CoinCounter *c)
{
    return c->frame >= tween_count_frames(c->from, c->to, c->step, c->max_frames);
}

bool coin_counter_shining(const CoinCounter *c)
{
    return c->frame > 0 && !coin_counter_done(c);
}
