#include "ui/input.h"

#include "core/key_repeat.h"

static KeyRepeat s_repeat;
static bool s_was_touching;

void input_init(void)
{
    key_repeat_init(&s_repeat, INPUT_REPEAT_KEYS);
    s_was_touching = false;
}

void input_reset_repeat(void)
{
    key_repeat_reset(&s_repeat);
}

void input_update(InputFrame *in)
{
    hidScanInput();

    /* Touch is reported as a key bit; handle it separately from buttons. */
    u32 held = hidKeysHeld();
    in->touching = (held & KEY_TOUCH) != 0;
    held &= ~KEY_TOUCH;

    in->held = held;
    in->down = hidKeysDown() & ~KEY_TOUCH;
    in->up = hidKeysUp() & ~KEY_TOUCH;
    in->keys = key_repeat_update(&s_repeat, held);

    in->touch_down = in->touching && !s_was_touching;
    in->touch_up = !in->touching && s_was_touching;
    s_was_touching = in->touching;

    if (in->touching) {
        touchPosition tp;
        hidTouchRead(&tp);
        in->touch_x = tp.px;
        in->touch_y = tp.py;
    }
}

u32 input_focus(FocusState *focus, const InputFrame *in)
{
    return focus_update(focus, in->keys, INPUT_FOCUS_WAKE_KEYS, in->touch_down);
}

int input_tap(HitTap *tap, const InputFrame *in, const HitRect *targets, int count)
{
    int hit = in->touching ? hit_test(targets, count, in->touch_x, in->touch_y) : -1;
    return hit_tap_update(tap, in->touching, hit);
}

int input_direction(u32 keys)
{
    if (keys & KEY_UP)
        return FOCUS_UP;
    if (keys & KEY_DOWN)
        return FOCUS_DOWN;
    if (keys & KEY_LEFT)
        return FOCUS_LEFT;
    if (keys & KEY_RIGHT)
        return FOCUS_RIGHT;
    return -1;
}
