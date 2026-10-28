#ifndef PSS_UI_WIDGETS_DIALOG_H
#define PSS_UI_WIDGETS_DIALOG_H

#include <stdbool.h>

#include "core/geom.h"
#include "core/tween.h"

/*
 * Dialog: 280 px wide, rounded, with a green
 * title bar, over a darkened screen. Opening dims the screen in 8 frames and
 * pops the dialog from 0.92 to 1.00 in 10 frames; the content fades in
 * while the dialog settles, because text cannot be scaled without blur.
 * Closing is immediate: scenes hide the dialog and the scrim goes away.
 */

typedef struct {
    bool open;
    Tween scrim; /* 0 -> 1 */
    Tween pop;   /* 0.92 -> 1.00 */
} Dialog;

void dialog_init(Dialog *d);
void dialog_open(Dialog *d);
void dialog_close(Dialog *d);

/* Advance one frame. */
void dialog_step(Dialog *d);

/* Fully opened: input may go to the dialog's buttons. */
bool dialog_ready(const Dialog *d);

/* Dims everything drawn so far. Call before the dialog's own content. */
void dialog_draw_scrim(const Dialog *d);

/*
 * Draws the dialog frame and title for a dialog of the given total height,
 * centered on the bottom screen. Returns the box the content lies in (final
 * size, below the title bar). Does nothing while closed.
 */
GeomRect dialog_draw_frame(const Dialog *d, const char *title, int height);

/* Box of the dialog at rest, for hit testing and layout. */
GeomRect dialog_box(int height);

/* Opacity 0..255 for the content, rising while the dialog pops in. */
unsigned char dialog_content_alpha(const Dialog *d);

#endif
