#ifndef PSS_UI_WIDGETS_TOAST_VIEW_H
#define PSS_UI_WIDGETS_TOAST_VIEW_H

#include "core/toast.h"

/*
 * Short hint above the button bar: a dark pill
 * with white text that rises in, stays for about 1.5 seconds and fades out.
 * Timing lives in core/toast.
 */

#define TOAST_IN_FRAMES   8
#define TOAST_HOLD_FRAMES 90
#define TOAST_OUT_FRAMES  12

void toast_widget_init(Toast *t);
void toast_widget_draw(const Toast *t);

#endif
