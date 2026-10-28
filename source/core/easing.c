#include "core/easing.h"

/* Standard back overshoot constant (about 10 % overshoot). */
#define BACK_C1 1.70158f
#define BACK_C3 (BACK_C1 + 1.0f)

static float clamp01(float t)
{
    if (!(t > 0.0f)) /* also catches NaN */
        return 0.0f;
    if (t > 1.0f)
        return 1.0f;
    return t;
}

float easing_linear(float t)
{
    return clamp01(t);
}

float easing_out_cubic(float t)
{
    float u = 1.0f - clamp01(t);
    return 1.0f - u * u * u;
}

float easing_out_back(float t)
{
    float u = clamp01(t) - 1.0f;
    return 1.0f + BACK_C3 * u * u * u + BACK_C1 * u * u;
}

float easing_sin_turns(float turns)
{
    /* Reduce to x in [-0.5, 0.5) turns. */
    float x = turns - (float)(long)turns;
    if (x >= 0.5f)
        x -= 1.0f;
    else if (x < -0.5f)
        x += 1.0f;

    /* Fold into [-0.25, 0.25] using sin(pi - a) = sin(a). */
    if (x > 0.25f)
        x = 0.5f - x;
    else if (x < -0.25f)
        x = -0.5f - x;

    /* Taylor series of sin(2*pi*x) up to x^9; error below 1e-4 on the range. */
    const float a = 6.28318531f * x;
    const float a2 = a * a;
    return a * (1.0f - a2 / 6.0f * (1.0f - a2 / 20.0f * (1.0f - a2 / 42.0f * (1.0f - a2 / 72.0f))));
}

float easing_in_out_sine(float t)
{
    /* (1 - cos(pi*t)) / 2 == (1 + sin(pi*t - pi/2)) / 2 */
    float s = easing_sin_turns(clamp01(t) * 0.5f - 0.25f);
    return 0.5f * (1.0f + s);
}

float easing_apply(Easing curve, float t)
{
    switch (curve) {
    case EASE_OUT_CUBIC:
        return easing_out_cubic(t);
    case EASE_OUT_BACK:
        return easing_out_back(t);
    case EASE_IN_OUT_SINE:
        return easing_in_out_sine(t);
    case EASE_LINEAR:
    default:
        return easing_linear(t);
    }
}

unsigned easing_pulse_alpha(unsigned frame, unsigned period)
{
    if (period == 0)
        return 255;
    /* Starts at full opacity, dips to 60 % at half the period. */
    float phase = (float)(frame % period) / (float)period;
    float s = easing_sin_turns(phase + 0.25f); /* cos */
    float alpha = 0.8f + 0.2f * s;
    return (unsigned)(alpha * 255.0f + 0.5f);
}
