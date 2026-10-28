#include "core/press.h"

void press_init(Press *p)
{
    p->pressed = false;
    tween_start(&p->release, 1.0f, 0.0f, 0, 0, EASE_OUT_BACK);
    tween_finish(&p->release);
}

void press_update(Press *p, bool pressed, uint16_t release_frames)
{
    if (pressed) {
        p->pressed = true;
        return;
    }
    if (p->pressed) {
        p->pressed = false;
        tween_start(&p->release, 1.0f, 0.0f, release_frames, 0, EASE_OUT_BACK);
    }
    tween_step(&p->release);
}

int press_offset(const Press *p)
{
    if (p->pressed)
        return 1;
    int px = tween_value_px(&p->release);
    return px < 0 ? 0 : px; /* the overshoot would lift the button above its rest position */
}
