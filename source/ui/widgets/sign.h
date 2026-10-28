#ifndef PSS_UI_WIDGETS_SIGN_H
#define PSS_UI_WIDGETS_SIGN_H

#include "core/geom.h"

/*
 * The shop sign of the top screen:
 * a white card hanging from the awning on two rods, with the logo, a title
 * and a line below it. Used by the start screen, S1 and the error screens.
 * `alpha` fades the whole sign in; returns its box.
 */
GeomRect sign_draw(const char *title, const char *subtitle, unsigned char alpha);

/* The version line at the lower right of the top screen. */
void sign_version_draw(const char *version);

#endif
