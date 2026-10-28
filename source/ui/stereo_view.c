#include "ui/stereo_view.h"

#include <3ds.h>

#include "ui/draw.h"

float stereo_slider(void)
{
    float s = osGet3DSliderState();
    return s < 0.0f ? 0.0f : s > 1.0f ? 1.0f : s;
}

void stereo_layer_begin(const StereoView *view, StereoLayer layer)
{
    draw_translate_push(stereo_shift_px(layer, view->slider, view->eye), 0);
}

void stereo_layer_end(void)
{
    draw_translate_pop();
}
