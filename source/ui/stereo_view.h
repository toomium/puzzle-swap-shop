#ifndef PSS_UI_STEREO_VIEW_H
#define PSS_UI_STEREO_VIEW_H

#include "core/stereo.h"

/*
 * Stereoscopic rendering of the top screen. The top
 * screen is drawn once per eye; each layer is moved by its depth times the
 * 3D slider with stereo_layer_begin(). With the slider at 0 only one image
 * is rendered and every layer stays where it is.
 */

typedef struct {
    StereoEye eye;
    float slider; /* 0..1 */
} StereoView;

/* Current position of the 3D slider, 0..1. */
float stereo_slider(void);

/* Everything drawn until stereo_layer_end() sits in the given depth layer. */
void stereo_layer_begin(const StereoView *view, StereoLayer layer);
void stereo_layer_end(void);

#endif
