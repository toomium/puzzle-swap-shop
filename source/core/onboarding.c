#include "core/onboarding.h"

static void enter(Onboarding *o, OnbPhase phase)
{
    o->phase = phase;
    o->frame = 0;
}

void onboarding_init(Onboarding *o)
{
    enter(o, ONB_WAIT);
}

bool onboarding_start(Onboarding *o)
{
    if (o->phase != ONB_WAIT && o->phase != ONB_FAILED)
        return false;
    enter(o, ONB_WORKING);
    return true;
}

bool onboarding_work_due(const Onboarding *o)
{
    return o->phase == ONB_WORKING && o->frame >= ONB_WORK_DELAY;
}

void onboarding_finish(Onboarding *o, bool ok)
{
    if (o->phase == ONB_WORKING)
        enter(o, ok ? ONB_REVEAL : ONB_FAILED);
}

void onboarding_step(Onboarding *o)
{
    o->frame++;
    if (o->phase == ONB_REVEAL && o->frame >= ONB_FILES * ONB_DOT_STEP + ONB_HOLD)
        enter(o, ONB_DONE);
}

bool onboarding_dot_green(const Onboarding *o, unsigned file)
{
    if (o->phase == ONB_DONE)
        return true;
    return o->phase == ONB_REVEAL && o->frame >= (file + 1) * ONB_DOT_STEP;
}

bool onboarding_busy(const Onboarding *o)
{
    return o->phase == ONB_WORKING || o->phase == ONB_REVEAL;
}
