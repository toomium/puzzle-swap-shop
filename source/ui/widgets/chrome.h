#ifndef PSS_UI_WIDGETS_CHROME_H
#define PSS_UI_WIDGETS_CHROME_H

#include <stdbool.h>

#include "core/geom.h"
#include "ui/widgets/button.h"

/*
 * Frame of every bottom screen: white title bar with a
 * line below it, and the gradient button bar at the bottom with a line above.
 * Scenes draw their content between the two and the buttons into the bar.
 */

void chrome_header_draw(const char *title, const char *right_note);

/* The bar itself; buttons go at chrome_bar_button_y(). */
void chrome_bar_draw(void);

/* y of a button in the bar. */
int chrome_bar_button_y(void);

/* Draws a button in the bar, at the left or right margin; returns its box (the touch target). */
GeomRect chrome_bar_button(const ButtonSpec *spec, bool right, const ButtonView *view);

/* Content area between header and bar. */
GeomRect chrome_content(void);

#endif
