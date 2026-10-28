#ifndef PSS_CORE_EASING_H
#define PSS_CORE_EASING_H

/*
 * Easing curves . Input and output are
 * normalized: t = 0 is the start, t = 1 the end. Inputs outside [0, 1] are
 * clamped. ease-out-back overshoots above 1 on purpose.
 *
 * Only polynomials, so the core stays free of libm.
 */

typedef enum {
    EASE_LINEAR = 0,
    EASE_OUT_CUBIC,
    EASE_OUT_BACK,
    EASE_IN_OUT_SINE,
    EASE_COUNT,
} Easing;

float easing_apply(Easing curve, float t);

float easing_linear(float t);
float easing_out_cubic(float t);
float easing_out_back(float t);
float easing_in_out_sine(float t);

/* sin(2*pi*turns), accurate to about 1e-4. */
float easing_sin_turns(float turns);

/*
 * Focus ring opacity: 60 % to 100 % and back as a sine over `period` frames
 * (60 in the design). Returns 0..255.
 */
unsigned easing_pulse_alpha(unsigned frame, unsigned period);

#endif
