#ifndef PSS_UI_ART_SOURCE_H
#define PSS_UI_ART_SOURCE_H

#include <3ds.h>
#include <citro2d.h>

#include "app/model.h"

/*
 * The real puzzle pictures: reads the CGFX block of a SpotPass panel
 * through the model, decodes it and keeps the most recent pictures as GPU
 * textures. Everything can fail (no PNEL file, a layout that differs from the
 * hypothesis in core/cgfx.h, no memory); then art_source_get() returns NULL
 * and the caller draws the placeholder. A failure is remembered, so a
 * picture that cannot be decoded is not tried again every frame.
 */

void art_source_init(const Model *model);
void art_source_fini(void);

/* Once per frame (draw_frame_begin): allows the next picture to be loaded. */
void art_source_frame(void);

/*
 * The picture of a panel, or NULL while it is not ready or cannot be had.
 * At most one picture is decoded per frame, so a page of cards fills in
 * over a few frames instead of stalling one.
 */
const C2D_Image *art_source_get(unsigned panel);

#endif
