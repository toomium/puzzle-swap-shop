#include "greatest.h"
#include "core/scene_stack.h"
#include "core/stereo.h"

static int a, b, c; /* scenes are only identified by address */

#define FRAMES 12
#define SHIFT  16

static void fresh(SceneStack *s)
{
    scene_stack_init(s, FRAMES, EASE_OUT_CUBIC);
}

TEST first_scene_has_no_transition(void)
{
    SceneStack s;
    fresh(&s);
    ASSERT_EQ(NULL, scene_stack_top(&s));
    ASSERT(scene_stack_push(&s, &a));
    ASSERT(!scene_stack_transitioning(&s));
    SceneBlend bl = scene_stack_blend(&s, SHIFT);
    ASSERT_EQ(NULL, bl.outgoing);
    ASSERT_EQ(&a, bl.incoming);
    ASSERT_EQ(0, bl.dx_in);
    ASSERT_EQ(255, bl.alpha_in);
    PASS();
}

TEST push_slides_in_from_the_right_and_fades_in(void)
{
    SceneStack s;
    fresh(&s);
    scene_stack_push(&s, &a);
    ASSERT(scene_stack_push(&s, &b));
    ASSERT(scene_stack_transitioning(&s));

    SceneBlend bl = scene_stack_blend(&s, SHIFT);
    ASSERT_EQ(&a, bl.outgoing);
    ASSERT_EQ(&b, bl.incoming);
    ASSERT_EQ(SHIFT, bl.dx_in);
    ASSERT_EQ(0, bl.dx_out);
    ASSERT_EQ(0, bl.alpha_in);
    ASSERT_EQ(255, bl.alpha_out);

    int prev_dx = SHIFT, prev_alpha = 0, prev_out = 255;
    for (int f = 1; f < FRAMES; f++) {
        scene_stack_step(&s);
        ASSERT(scene_stack_transitioning(&s));
        bl = scene_stack_blend(&s, SHIFT);
        ASSERT(bl.dx_in <= prev_dx);
        ASSERT(bl.alpha_in >= prev_alpha);
        ASSERT(bl.alpha_out <= prev_out);
        /* Never both mostly opaque: the old scene cannot show through the new one. */
        ASSERT(bl.alpha_in + bl.alpha_out <= 256);
        ASSERT(bl.dx_out <= 0); /* the old scene leaves to the left */
        prev_dx = bl.dx_in;
        prev_alpha = bl.alpha_in;
        prev_out = bl.alpha_out;
    }
    scene_stack_step(&s);
    ASSERT(!scene_stack_transitioning(&s));
    bl = scene_stack_blend(&s, SHIFT);
    ASSERT_EQ(NULL, bl.outgoing);
    ASSERT_EQ(&b, bl.incoming);
    ASSERT_EQ(0, bl.dx_in);
    ASSERT_EQ(255, bl.alpha_in);
    PASS();
}

TEST pop_slides_in_from_the_left(void)
{
    SceneStack s;
    fresh(&s);
    scene_stack_push(&s, &a);
    scene_stack_push(&s, &b);
    for (int f = 0; f < FRAMES; f++)
        scene_stack_step(&s);

    ASSERT(scene_stack_pop(&s));
    ASSERT_EQ(&a, scene_stack_top(&s));
    SceneBlend bl = scene_stack_blend(&s, SHIFT);
    ASSERT_EQ(&b, bl.outgoing);
    ASSERT_EQ(&a, bl.incoming);
    ASSERT_EQ(-SHIFT, bl.dx_in);
    scene_stack_step(&s);
    scene_stack_step(&s);
    bl = scene_stack_blend(&s, SHIFT);
    ASSERT(bl.dx_out >= 0); /* the old scene leaves to the right */
    ASSERT(bl.dx_in < 0 && bl.dx_in > -SHIFT);
    PASS();
}

TEST replace_keeps_depth(void)
{
    SceneStack s;
    fresh(&s);
    scene_stack_push(&s, &a);
    ASSERT(scene_stack_replace(&s, &b));
    ASSERT_EQ(1, s.count);
    ASSERT_EQ(&b, scene_stack_top(&s));
    SceneBlend bl = scene_stack_blend(&s, SHIFT);
    ASSERT_EQ(&a, bl.outgoing);
    ASSERT_EQ(SHIFT, bl.dx_in);
    PASS();
}

TEST limits_change_nothing(void)
{
    SceneStack s;
    fresh(&s);
    ASSERT(!scene_stack_pop(&s));
    ASSERT(!scene_stack_replace(&s, &a));
    ASSERT(!scene_stack_push(&s, NULL));
    scene_stack_push(&s, &a);
    ASSERT(!scene_stack_pop(&s)); /* the root stays */
    ASSERT_EQ(&a, scene_stack_top(&s));

    static int many[SCENE_STACK_MAX];
    for (int i = 1; i < SCENE_STACK_MAX; i++)
        ASSERT(scene_stack_push(&s, &many[i]));
    ASSERT(!scene_stack_push(&s, &c));
    ASSERT_EQ(SCENE_STACK_MAX, s.count);
    ASSERT_EQ(&many[SCENE_STACK_MAX - 1], scene_stack_top(&s));
    PASS();
}

TEST new_change_ends_the_running_one(void)
{
    SceneStack s;
    fresh(&s);
    scene_stack_push(&s, &a);
    scene_stack_push(&s, &b);
    scene_stack_step(&s);
    scene_stack_step(&s);

    /* Never more than two scenes on screen: b becomes the outgoing one at once. */
    ASSERT(scene_stack_push(&s, &c));
    SceneBlend bl = scene_stack_blend(&s, SHIFT);
    ASSERT_EQ(&b, bl.outgoing);
    ASSERT_EQ(&c, bl.incoming);
    ASSERT_EQ(SHIFT, bl.dx_in);
    PASS();
}

TEST zero_frames_switches_at_once(void)
{
    SceneStack s;
    scene_stack_init(&s, 0, EASE_LINEAR);
    scene_stack_push(&s, &a);
    scene_stack_push(&s, &b);
    ASSERT(!scene_stack_transitioning(&s));
    ASSERT_EQ(&b, scene_stack_blend(&s, SHIFT).incoming);
    PASS();
}

SUITE(scene_stack_suite)
{
    RUN_TEST(first_scene_has_no_transition);
    RUN_TEST(push_slides_in_from_the_right_and_fades_in);
    RUN_TEST(pop_slides_in_from_the_left);
    RUN_TEST(replace_keeps_depth);
    RUN_TEST(limits_change_nothing);
    RUN_TEST(new_change_ends_the_running_one);
    RUN_TEST(zero_frames_switches_at_once);
}

TEST depths_follow_the_design(void)
{
    ASSERT_EQ(-6, stereo_layer_depth(STEREO_LAYER_BACKGROUND));
    ASSERT_EQ(-4, stereo_layer_depth(STEREO_LAYER_CLOUDS));
    ASSERT_EQ(0, stereo_layer_depth(STEREO_LAYER_BOARD));
    ASSERT_EQ(2, stereo_layer_depth(STEREO_LAYER_FRONT));
    ASSERT_EQ(3, stereo_layer_depth(STEREO_LAYER_POP));
    ASSERT_EQ(0, stereo_layer_depth(STEREO_LAYER_COUNT));
    PASS();
}

TEST slider_zero_means_no_offset(void)
{
    for (int l = 0; l < STEREO_LAYER_COUNT; l++) {
        ASSERT_EQ(0, stereo_shift_px((StereoLayer)l, 0.0f, STEREO_EYE_LEFT));
        ASSERT_EQ(0, stereo_shift_px((StereoLayer)l, 0.0f, STEREO_EYE_RIGHT));
        ASSERT_EQ(0, stereo_shift_px((StereoLayer)l, -0.5f, STEREO_EYE_LEFT));
    }
    ASSERT_EQ(0, stereo_shift_px(STEREO_LAYER_POP, (float)(0.0 / 0.0), STEREO_EYE_LEFT));
    PASS();
}

TEST eyes_are_mirrored(void)
{
    ASSERT_EQ(3, stereo_shift_px(STEREO_LAYER_POP, 1.0f, STEREO_EYE_LEFT));
    ASSERT_EQ(-3, stereo_shift_px(STEREO_LAYER_POP, 1.0f, STEREO_EYE_RIGHT));
    ASSERT_EQ(-6, stereo_shift_px(STEREO_LAYER_BACKGROUND, 1.0f, STEREO_EYE_LEFT));
    ASSERT_EQ(6, stereo_shift_px(STEREO_LAYER_BACKGROUND, 1.0f, STEREO_EYE_RIGHT));
    ASSERT_EQ(0, stereo_shift_px(STEREO_LAYER_BOARD, 1.0f, STEREO_EYE_LEFT));
    PASS();
}

TEST offset_scales_with_slider_in_whole_pixels(void)
{
    ASSERT_EQ(-3, stereo_shift_px(STEREO_LAYER_BACKGROUND, 0.5f, STEREO_EYE_LEFT));
    ASSERT_EQ(-2, stereo_shift_px(STEREO_LAYER_BACKGROUND, 0.25f, STEREO_EYE_LEFT)); /* -1.5 */
    ASSERT_EQ(1, stereo_shift_px(STEREO_LAYER_FRONT, 0.5f, STEREO_EYE_LEFT));
    ASSERT_EQ(-6, stereo_shift_px(STEREO_LAYER_BACKGROUND, 3.0f, STEREO_EYE_LEFT)); /* clamped */
    for (int l = 0; l < STEREO_LAYER_COUNT; l++) {
        for (int i = 0; i <= 100; i++) {
            int px = stereo_shift_px((StereoLayer)l, (float)i / 100.0f, STEREO_EYE_LEFT);
            ASSERT(px >= -STEREO_MAX_SHIFT && px <= STEREO_MAX_SHIFT);
        }
    }
    PASS();
}

SUITE(stereo_suite)
{
    RUN_TEST(depths_follow_the_design);
    RUN_TEST(slider_zero_means_no_offset);
    RUN_TEST(eyes_are_mirrored);
    RUN_TEST(offset_scales_with_slider_in_whole_pixels);
}
