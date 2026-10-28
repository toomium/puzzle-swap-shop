#ifndef PSS_UI_WIDGETS_LOCK_H
#define PSS_UI_WIDGETS_LOCK_H

#include "ui/theme.h"

/*
 * Small padlock: marks the original backup, which
 * the app never removes. THEME_LOCK_W x THEME_LOCK_H px with its top left at
 * (x, y), drawn from rectangles in the given color.
 */
void lock_draw(int x, int y, ThemeColor color, unsigned char alpha);

#endif
