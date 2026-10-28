#ifndef PSS_UI_BACKDROP_H
#define PSS_UI_BACKDROP_H

#include "ui/stereo_view.h"

/*
 * The kiosk scene behind every screen, drawn by the
 * scene host before the scenes, so scene changes cross-fade only their
 * content: sky, meadow, hills and clouds with the awning on top, and the
 * slowly moving stripes below.
 */

/* Top screen, in the depth layers of section 9. */
void backdrop_top_draw(const StereoView *view);

void backdrop_bottom_draw(void);

#endif
