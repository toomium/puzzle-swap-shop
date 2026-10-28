#ifndef PSS_UI_INPUT_H
#define PSS_UI_INPUT_H

#include <stdbool.h>

#include <3ds.h>

#include "core/focus.h"
#include "core/hit.h"

/*
 * Input for one frame, read once per frame
 * with input_update(). Scenes act on `keys`, which already contains key
 * repeat; D-pad and Circle Pad are merged (KEY_UP = KEY_DUP | KEY_CPAD_UP).
 */

/* Keys that repeat when held. */
#define INPUT_REPEAT_KEYS (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)

/* Keys that only reveal a hidden focus ring on the first press. */
#define INPUT_FOCUS_WAKE_KEYS (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT | KEY_A)

typedef struct {
    u32 keys; /* pressed this frame or repeating */
    u32 down; /* pressed this frame, without repeat */
    u32 held;
    u32 up;          /* released this frame */
    bool touching;   /* stylus on the screen */
    bool touch_down; /* stylus touched down this frame */
    bool touch_up;   /* stylus lifted this frame */
    int touch_x;     /* last touch position, valid while touching */
    int touch_y;
} InputFrame;

void input_init(void);

/* Calls hidScanInput() and fills `in`. Call exactly once per frame. */
void input_update(InputFrame *in);

/* Drops held keys after a screen change, so a held key does not fire again on the new screen. */
void input_reset_repeat(void);

/*
 * Applies the focus model to this frame: touch hides the ring, the first key
 * press shows it again. Returns the keys the scene should act on.
 */
u32 input_focus(FocusState *focus, const InputFrame *in);

/*
 * Tap handling for a set of touch targets: returns the target activated
 * this frame or -1. Draw hit_tap_pressed(tap) as pressed.
 */
int input_tap(HitTap *tap, const InputFrame *in, const HitRect *targets, int count);

/* Direction from the fired keys, or -1 if none. Up wins over down, left over right. */
int input_direction(u32 keys);

#endif
