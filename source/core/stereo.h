#ifndef PSS_CORE_STEREO_H
#define PSS_CORE_STEREO_H

/*
 * Depth layers of the top screen. A layer's
 * horizontal offset is its depth times the 3D slider, mirrored for the two
 * eyes. Pure arithmetic, so it is tested on the PC; ui/stereo.c applies it.
 */

typedef enum {
    STEREO_LAYER_BACKGROUND, /* sky, meadow */
    STEREO_LAYER_CLOUDS,
    STEREO_LAYER_BOARD, /* screen plane */
    STEREO_LAYER_FRONT, /* pills, awning */
    STEREO_LAYER_POP,   /* banner, sparks */
    STEREO_LAYER_COUNT,
} StereoLayer;

typedef enum {
    STEREO_EYE_LEFT,
    STEREO_EYE_RIGHT,
} StereoEye;

/* Largest offset any layer reaches at full slider; backgrounds are drawn this much wider. */
#define STEREO_MAX_SHIFT 6

/* Depth in pixels; negative lies behind the screen, positive in front of it. */
int stereo_layer_depth(StereoLayer layer);

/*
 * Whole-pixel x offset of a layer in one eye for a slider value 0..1 (values
 * outside are clamped). A layer in front of the screen moves right in the
 * left eye and left in the right eye. This sign is the one hypothesis here:
 * it follows the geometry of crossed disparity and is confirmed when it is checked
 * checks the smoke test on the console (sky behind, awning in front).
 * Slider 0 gives 0 for every layer.
 */
int stereo_shift_px(StereoLayer layer, float slider, StereoEye eye);

#endif
