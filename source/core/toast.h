#ifndef PSS_CORE_TOAST_H
#define PSS_CORE_TOAST_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Timing of a short hint: it fades in
 * while rising a few pixels, stays, and fades out. Frames at 60 fps.
 */

#define TOAST_TEXT_MAX 96

typedef struct {
    char text[TOAST_TEXT_MAX];
    uint16_t age;
    uint16_t in_frames;
    uint16_t hold_frames;
    uint16_t out_frames;
    bool active;
} Toast;

void toast_init(Toast *t, uint16_t in_frames, uint16_t hold_frames, uint16_t out_frames);

/* Shows a text, restarting a running toast. Longer texts are cut at a UTF-8 boundary. */
void toast_show(Toast *t, const char *text);

/* Advance one frame; the toast ends by itself. */
void toast_step(Toast *t);

bool toast_visible(const Toast *t);

/* Opacity 0..255 for the current frame. */
uint8_t toast_alpha(const Toast *t);

/* Pixels the toast still has to rise: `rise_px` at the start, 0 when it has arrived. */
int toast_offset(const Toast *t, int rise_px);

#endif
