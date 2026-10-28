#include <math.h>

#include "greatest.h"
#include "core/easing.h"
#include "core/tween.h"

#define EPS 0.0005f

TEST endpoints(void)
{
    for (int c = 0; c < EASE_COUNT; c++) {
        ASSERT_IN_RANGE(0.0f, easing_apply((Easing)c, 0.0f), EPS);
        ASSERT_IN_RANGE(1.0f, easing_apply((Easing)c, 1.0f), EPS);
        /* Clamped outside 0..1. */
        ASSERT_IN_RANGE(0.0f, easing_apply((Easing)c, -3.0f), EPS);
        ASSERT_IN_RANGE(1.0f, easing_apply((Easing)c, 7.0f), EPS);
    }
    ASSERT_IN_RANGE(0.0f, easing_apply(EASE_OUT_CUBIC, NAN), EPS);
    PASS();
}

TEST known_values(void)
{
    ASSERT_IN_RANGE(0.5f, easing_linear(0.5f), EPS);
    ASSERT_IN_RANGE(0.875f, easing_out_cubic(0.5f), EPS);
    /* ease-out-back overshoots: c3*u^3 + c1*u^2 + 1 at t = 0.5 */
    ASSERT_IN_RANGE(1.0876975f, easing_out_back(0.5f), EPS);
    ASSERT_IN_RANGE(0.5f, easing_in_out_sine(0.5f), EPS);
    ASSERT_IN_RANGE(0.1464466f, easing_in_out_sine(0.25f), EPS);
    PASS();
}

TEST out_cubic_monotonic_and_fast_start(void)
{
    float prev = 0.0f;
    for (int i = 1; i <= 12; i++) {
        float v = easing_out_cubic(i / 12.0f);
        ASSERT(v > prev);
        ASSERT(v >= i / 12.0f - EPS); /* ease-out: always ahead of linear */
        prev = v;
    }
    PASS();
}

TEST out_back_overshoots_once(void)
{
    float max = 0.0f;
    for (int i = 0; i <= 100; i++) {
        float v = easing_out_back(i / 100.0f);
        if (v > max)
            max = v;
    }
    ASSERT(max > 1.09f && max < 1.11f); /* about 10 % */
    PASS();
}

TEST sine_known_values(void)
{
    /* sin(2*pi*turns) at exact angles, including negative and wrapped ones. */
    static const struct {
        float turns, sin;
    } cases[] = {
        {0.0f, 0.0f},        {1.0f / 12, 0.5f},      {0.125f, 0.7071068f}, {1.0f / 6, 0.8660254f},
        {0.25f, 1.0f},       {1.0f / 3, 0.8660254f}, {0.5f, 0.0f},         {0.75f, -1.0f},
        {-0.25f, -1.0f},     {-1.0f / 12, -0.5f},    {2.125f, 0.7071068f}, {-3.75f, 1.0f},
        {0.9f, -0.5877853f}, {0.05f, 0.3090170f},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        ASSERT_IN_RANGE(cases[i].sin, easing_sin_turns(cases[i].turns), 0.0002f);
    PASS();
}

TEST focus_pulse(void)
{
    ASSERT_EQ(255u, easing_pulse_alpha(0, 60));
    ASSERT_EQ(153u, easing_pulse_alpha(30, 60)); /* 60 % */
    ASSERT_EQ(255u, easing_pulse_alpha(60, 60));
    ASSERT_EQ(easing_pulse_alpha(15, 60), easing_pulse_alpha(45, 60));
    for (unsigned f = 0; f < 120; f++) {
        unsigned a = easing_pulse_alpha(f, 60);
        ASSERT(a >= 153 && a <= 255);
    }
    ASSERT_EQ(255u, easing_pulse_alpha(7, 0));
    PASS();
}

TEST tween_runs_in_frames(void)
{
    Tween t;
    tween_start(&t, 0.0f, 16.0f, 12, 0, EASE_LINEAR);
    ASSERT_EQ(0, tween_value_px(&t));
    int steps = 0;
    while (tween_step(&t))
        steps++;
    ASSERT_EQ(11, steps); /* the 12th step finishes */
    ASSERT(tween_done(&t));
    ASSERT_IN_RANGE(16.0f, tween_value(&t), EPS);
    ASSERT_FALSE(tween_step(&t));
    ASSERT_EQ(12, t.elapsed);

    tween_start(&t, 0.0f, 16.0f, 12, 0, EASE_LINEAR);
    for (int i = 0; i < 6; i++)
        tween_step(&t);
    ASSERT_EQ(8, tween_value_px(&t));
    PASS();
}

TEST tween_delay_and_zero_duration(void)
{
    Tween t;
    tween_start(&t, 1.3f, 1.0f, 14, 4, EASE_OUT_BACK);
    for (int i = 0; i < 4; i++) {
        ASSERT_IN_RANGE(1.3f, tween_value(&t), EPS);
        tween_step(&t);
    }
    ASSERT_IN_RANGE(1.3f, tween_value(&t), EPS);
    tween_step(&t);
    ASSERT(tween_value(&t) < 1.3f);

    tween_start(&t, 0.0f, 5.0f, 0, 0, EASE_LINEAR);
    ASSERT(tween_done(&t));
    ASSERT_IN_RANGE(5.0f, tween_value(&t), EPS);

    tween_start(&t, 0.0f, 5.0f, 10, 2, EASE_LINEAR);
    tween_finish(&t);
    ASSERT(tween_done(&t));
    ASSERT_IN_RANGE(5.0f, tween_value(&t), EPS);
    PASS();
}

TEST negative_px_rounding(void)
{
    Tween t;
    tween_start(&t, 0.0f, -16.0f, 12, 0, EASE_LINEAR);
    for (int i = 0; i < 3; i++)
        tween_step(&t);
    ASSERT_EQ(-4, tween_value_px(&t));
    PASS();
}

TEST bump_goes_up_and_back(void)
{
    ASSERT_IN_RANGE(1.0f, tween_bump(0, 8, 1.0f, 1.08f), EPS);
    ASSERT_IN_RANGE(1.08f, tween_bump(4, 8, 1.0f, 1.08f), EPS);
    ASSERT_IN_RANGE(1.0f, tween_bump(8, 8, 1.0f, 1.08f), EPS);
    ASSERT_IN_RANGE(1.0f, tween_bump(99, 8, 1.0f, 1.08f), EPS);
    for (unsigned f = 0; f < 8; f++) {
        float v = tween_bump(f, 8, 1.0f, 1.08f);
        ASSERT(v > 0.99f && v < 1.09f);
    }
    PASS();
}

TEST stagger(void)
{
    ASSERT_IN_RANGE(0.0f, tween_stagger(4, 1, 14, 4), EPS);
    ASSERT_IN_RANGE(1.0f / 14.0f, tween_stagger(5, 1, 14, 4), EPS);
    ASSERT_IN_RANGE(1.0f, tween_stagger(18, 1, 14, 4), EPS);
    ASSERT_IN_RANGE(0.5f, tween_stagger(7, 0, 14, 4), EPS);
    PASS();
}

TEST coin_counter_two_frames_per_coin(void)
{
    /* 120 -> 114: 6 coins, 12 frames. */
    ASSERT_EQ(12u, tween_count_frames(120, 114, 2, 45));
    ASSERT_EQ(120u, tween_count_value(120, 114, 0, 2, 45));
    ASSERT_EQ(119u, tween_count_value(120, 114, 2, 2, 45));
    ASSERT_EQ(117u, tween_count_value(120, 114, 7, 2, 45));
    ASSERT_EQ(114u, tween_count_value(120, 114, 12, 2, 45));
    ASSERT_EQ(114u, tween_count_value(120, 114, 500, 2, 45));
    /* Counting up works too. */
    ASSERT_EQ(3u, tween_count_value(0, 10, 6, 2, 45));
    PASS();
}

TEST coin_counter_capped_at_45_frames(void)
{
    ASSERT_EQ(45u, tween_count_frames(300, 0, 2, 45));
    ASSERT_EQ(300u, tween_count_value(300, 0, 0, 2, 45));
    ASSERT_EQ(0u, tween_count_value(300, 0, 45, 2, 45));
    unsigned prev = 300;
    for (unsigned f = 1; f <= 45; f++) {
        unsigned v = tween_count_value(300, 0, f, 2, 45);
        ASSERT(v <= prev);
        prev = v;
    }
    ASSERT_EQ(0u, tween_count_frames(50, 50, 2, 45));
    ASSERT_EQ(50u, tween_count_value(50, 50, 0, 2, 45));
    PASS();
}

TEST coin_sound_every_five(void)
{
    unsigned ticks = 0;
    for (unsigned f = 0; f <= 12; f++)
        ticks += tween_count_tick(120, 114, f, 2, 45, 5);
    ASSERT_EQ(1u, ticks); /* 6 coins: one tick at the fifth */

    ticks = 0;
    for (unsigned f = 0; f <= 45; f++)
        ticks += tween_count_tick(300, 0, f, 2, 45, 5);
    ASSERT_EQ(45u, ticks); /* 60 marks, but at most one per frame */
    PASS();
}

TEST scroll_whole_pixels(void)
{
    /* 0.25 px per frame: 1 px every 4 frames, wrapped. */
    ASSERT_EQ(0, tween_scroll_px(3, 1, 4, 40));
    ASSERT_EQ(1, tween_scroll_px(4, 1, 4, 40));
    ASSERT_EQ(0, tween_scroll_px(160, 1, 4, 40));
    ASSERT_EQ(3, tween_scroll_px(35, 1, 10, 20));
    ASSERT_EQ(0, tween_scroll_px(35, 1, 0, 20));
    PASS();
}

SUITE(easing_suite)
{
    RUN_TEST(endpoints);
    RUN_TEST(known_values);
    RUN_TEST(out_cubic_monotonic_and_fast_start);
    RUN_TEST(out_back_overshoots_once);
    RUN_TEST(sine_known_values);
    RUN_TEST(focus_pulse);
    RUN_TEST(tween_runs_in_frames);
    RUN_TEST(tween_delay_and_zero_duration);
    RUN_TEST(negative_px_rounding);
    RUN_TEST(bump_goes_up_and_back);
    RUN_TEST(stagger);
    RUN_TEST(coin_counter_two_frames_per_coin);
    RUN_TEST(coin_counter_capped_at_45_frames);
    RUN_TEST(coin_sound_every_five);
    RUN_TEST(scroll_whole_pixels);
}
