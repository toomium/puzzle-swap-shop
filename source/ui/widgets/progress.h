#ifndef PSS_UI_WIDGETS_PROGRESS_H
#define PSS_UI_WIDGETS_PROGRESS_H

#include "core/geom.h"

/*
 * Progress bar: rounded track and a green fill that
 * turns gold when everything is owned. The height comes from the track box
 * (THEME_PROGRESS_H on the top screen, THEME_PROGRESS_SMALL_H on cards).
 */

typedef enum {
    PROGRESS_ON_CARD, /* light track on a white card */
    PROGRESS_ON_SKY,  /* faint dark track on the top screen */
} ProgressStyle;

/* `alpha` fades the whole bar; a panel that is not on the console passes owned 0 and 40 %. */
void progress_draw(GeomRect track, unsigned owned, unsigned total, ProgressStyle style,
                   unsigned char alpha);

#endif
