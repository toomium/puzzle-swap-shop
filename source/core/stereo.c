#include "core/stereo.h"

static const int s_depth[STEREO_LAYER_COUNT] = {
    [STEREO_LAYER_BACKGROUND] = -6, [STEREO_LAYER_CLOUDS] = -4, [STEREO_LAYER_BOARD] = 0,
    [STEREO_LAYER_FRONT] = 2,       [STEREO_LAYER_POP] = 3,
};

int stereo_layer_depth(StereoLayer layer)
{
    if ((unsigned)layer >= STEREO_LAYER_COUNT)
        return 0;
    return s_depth[layer];
}

int stereo_shift_px(StereoLayer layer, float slider, StereoEye eye)
{
    if (!(slider > 0.0f)) /* also catches NaN */
        return 0;
    if (slider > 1.0f)
        slider = 1.0f;
    float v = (float)stereo_layer_depth(layer) * slider;
    int px = v >= 0.0f ? (int)(v + 0.5f) : -(int)(-v + 0.5f);
    return eye == STEREO_EYE_LEFT ? px : -px;
}
