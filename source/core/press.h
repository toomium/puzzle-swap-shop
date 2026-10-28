#ifndef PSS_CORE_PRESS_H
#define PSS_CORE_PRESS_H

#include <stdbool.h>
#include <stdint.h>

#include "core/tween.h"

/*
 * Press feedback of a button: one pixel
 * down while held, a spring back with ease-out-back on release. Only the
 * pixel offset is kept here; looks are the widget's business.
 */

typedef struct {
    bool pressed;
    Tween release; /* 1 -> 0, only meaningful after a release */
} Press;

void press_init(Press *p);

/* Call once per frame with whether the element is held now. */
void press_update(Press *p, bool pressed, uint16_t release_frames);

/* Pixels the element is pushed down this frame: 1 while held, then 1 -> 0 (rounded). */
int press_offset(const Press *p);

#endif
