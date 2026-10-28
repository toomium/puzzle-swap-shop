#include "core/toast.h"

#include <string.h>

#include "core/easing.h"

void toast_init(Toast *t, uint16_t in_frames, uint16_t hold_frames, uint16_t out_frames)
{
    memset(t, 0, sizeof(*t));
    t->in_frames = in_frames;
    t->hold_frames = hold_frames;
    t->out_frames = out_frames;
}

void toast_show(Toast *t, const char *text)
{
    size_t n = strlen(text);
    if (n >= TOAST_TEXT_MAX) {
        n = TOAST_TEXT_MAX - 1;
        /* Do not leave half of a multi-byte character behind. */
        while (n > 0 && ((unsigned char)text[n] & 0xC0) == 0x80)
            n--;
    }
    memcpy(t->text, text, n);
    t->text[n] = '\0';
    t->age = 0;
    t->active = true;
}

void toast_step(Toast *t)
{
    if (!t->active)
        return;
    t->age++;
    if (t->age >= (unsigned)t->in_frames + t->hold_frames + t->out_frames)
        t->active = false;
}

bool toast_visible(const Toast *t)
{
    return t->active;
}

uint8_t toast_alpha(const Toast *t)
{
    if (!t->active)
        return 0;
    unsigned in = t->in_frames, hold_end = in + t->hold_frames;
    if (t->age < in)
        return (uint8_t)(255u * t->age / in);
    if (t->age < hold_end || t->out_frames == 0)
        return 255;
    unsigned left = hold_end + t->out_frames - t->age;
    return (uint8_t)(255u * left / t->out_frames);
}

int toast_offset(const Toast *t, int rise_px)
{
    if (!t->active || t->age >= t->in_frames || t->in_frames == 0)
        return 0;
    float p = easing_out_cubic((float)t->age / (float)t->in_frames);
    return (int)((float)rise_px * (1.0f - p) + 0.5f);
}
