#include "core/purchase_flow.h"

void purchase_flow_init(PurchaseFlow *f)
{
    f->phase = PF_IDLE;
    f->frame = 0;
    f->ran = false;
}

bool purchase_flow_start(PurchaseFlow *f)
{
    if (f->phase != PF_IDLE)
        return false;
    f->phase = PF_RUNNING;
    f->frame = 0;
    f->ran = false;
    return true;
}

bool purchase_flow_work_due(const PurchaseFlow *f)
{
    return f->phase == PF_RUNNING && !f->ran && f->frame >= PF_WORK_DELAY;
}

void purchase_flow_ran(PurchaseFlow *f)
{
    if (f->phase == PF_RUNNING)
        f->ran = true;
}

void purchase_flow_step(PurchaseFlow *f)
{
    if (f->phase == PF_RUNNING)
        f->frame++;
}

bool purchase_flow_done(const PurchaseFlow *f)
{
    return f->phase == PF_RUNNING && f->ran && f->frame >= PF_MIN_FRAMES;
}

float purchase_flow_progress(const PurchaseFlow *f)
{
    if (f->phase != PF_RUNNING)
        return 0.0f;
    if (f->frame >= PF_MIN_FRAMES)
        return 1.0f;
    return (float)f->frame / (float)PF_MIN_FRAMES;
}
