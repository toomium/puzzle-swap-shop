#include "greatest.h"
#include "core/focus.h"
#include "core/hit.h"
#include "core/key_repeat.h"

#define K_A    (1u << 0)
#define K_B    (1u << 1)
#define K_UP   (1u << 6)
#define K_DOWN (1u << 7)
#define K_NAV  (K_UP | K_DOWN)

/* Frames (0 = press frame) on which a key held for `frames` fires. */
static unsigned count_fires(KeyRepeat *kr, uint32_t key, unsigned frames, unsigned *first_repeat)
{
    unsigned fires = 0;
    *first_repeat = 0;
    for (unsigned f = 0; f < frames; f++) {
        if (key_repeat_update(kr, key) & key) {
            fires++;
            if (fires == 2)
                *first_repeat = f;
        }
    }
    return fires;
}

TEST repeat_18_then_every_5(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    uint32_t fired[40];
    for (unsigned f = 0; f < 40; f++)
        fired[f] = key_repeat_update(&kr, K_DOWN);

    ASSERT_EQ(K_DOWN, fired[0]);
    for (unsigned f = 1; f < 18; f++)
        ASSERT_EQ(0u, fired[f]);
    ASSERT_EQ(K_DOWN, fired[18]);
    for (unsigned f = 19; f < 23; f++)
        ASSERT_EQ(0u, fired[f]);
    ASSERT_EQ(K_DOWN, fired[23]);
    ASSERT_EQ(K_DOWN, fired[28]);
    ASSERT_EQ(K_DOWN, fired[33]);
    ASSERT_EQ(K_DOWN, fired[38]);
    PASS();
}

TEST repeat_long_hold_stays_in_phase(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    unsigned first;
    /* 10 minutes held: 1 press + repeats at 18, 23, ... */
    unsigned frames = 36000;
    unsigned fires = count_fires(&kr, K_UP, frames, &first);
    ASSERT_EQ(18u, first);
    ASSERT_EQ(1u + (frames - 18 + 4) / 5, fires);
    PASS();
}

TEST non_repeating_keys_fire_once(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    unsigned first;
    ASSERT_EQ(1u, count_fires(&kr, K_A, 200, &first));
    PASS();
}

TEST release_and_reset(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    for (int f = 0; f < 10; f++)
        key_repeat_update(&kr, K_DOWN);
    ASSERT_EQ(0u, key_repeat_update(&kr, 0));
    ASSERT_EQ(K_DOWN, key_repeat_update(&kr, K_DOWN)); /* fresh press */

    key_repeat_reset(&kr);
    ASSERT_EQ(0u, key_repeat_update(&kr, K_DOWN)); /* still held: silent */
    ASSERT_EQ(0u, key_repeat_update(&kr, 0));
    ASSERT_EQ(K_DOWN, key_repeat_update(&kr, K_DOWN));
    ASSERT_EQ(0u, key_repeat_update(&kr, K_DOWN));
    PASS();
}

TEST reset_silences_held_keys_until_released(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    /* B closes a screen; the key is still down on the next screen. */
    ASSERT_EQ(K_B, key_repeat_update(&kr, K_B));
    key_repeat_reset(&kr);
    for (int f = 0; f < 60; f++)
        ASSERT_EQ(0u, key_repeat_update(&kr, K_B));
    ASSERT_EQ(0u, key_repeat_update(&kr, 0));
    ASSERT_EQ(K_B, key_repeat_update(&kr, K_B)); /* a new press fires */
    PASS();
}

TEST reset_keeps_new_presses(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    ASSERT_EQ(K_B, key_repeat_update(&kr, K_B));
    key_repeat_reset(&kr);
    /* B was let go and A is pressed on the first frame of the new screen. */
    ASSERT_EQ(K_A, key_repeat_update(&kr, K_A));
    PASS();
}

TEST independent_keys(void)
{
    KeyRepeat kr;
    key_repeat_init(&kr, K_NAV);
    for (int f = 0; f < 10; f++)
        key_repeat_update(&kr, K_DOWN);
    /* A joins while DOWN is held; both run their own counters. */
    ASSERT_EQ(K_A, key_repeat_update(&kr, K_DOWN | K_A));
    for (int f = 11; f < 18; f++)
        key_repeat_update(&kr, K_DOWN | K_A);
    ASSERT_EQ(K_DOWN, key_repeat_update(&kr, K_DOWN | K_A));
    PASS();
}

TEST rect_contains_half_open(void)
{
    HitRect r = {10, 20, 30, 28};
    ASSERT(hit_rect_contains(r, 10, 20));
    ASSERT(hit_rect_contains(r, 39, 47));
    ASSERT_FALSE(hit_rect_contains(r, 40, 30));
    ASSERT_FALSE(hit_rect_contains(r, 20, 48));
    ASSERT_FALSE(hit_rect_contains(r, 9, 30));
    HitRect empty = {0, 0, 0, 10};
    ASSERT_FALSE(hit_rect_contains(empty, 0, 0));
    PASS();
}

TEST expand_small_targets(void)
{
    HitRect r = hit_rect_expand((HitRect){100, 100, 20, 16}, HIT_MIN_SIZE, HIT_MIN_SIZE);
    ASSERT_EQ(96, r.x);
    ASSERT_EQ(94, r.y);
    ASSERT_EQ(28, r.w);
    ASSERT_EQ(28, r.h);
    HitRect big = hit_rect_expand((HitRect){0, 0, 96, 78}, HIT_MIN_SIZE, HIT_MIN_SIZE);
    ASSERT_EQ(96, big.w);
    ASSERT_EQ(78, big.h);
    PASS();
}

TEST hit_test_topmost_and_slop(void)
{
    /* Card and a button drawn on top of it, plus a small chip. */
    HitRect rects[] = {
        {0, 0, 100, 100}, {10, 10, 40, 30}, {200, 100, 20, 20}, {0, 0, 0, 0}, /* disabled */
    };
    ASSERT_EQ(1, hit_test(rects, 4, 20, 20));
    ASSERT_EQ(0, hit_test(rects, 4, 80, 80));
    ASSERT_EQ(2, hit_test(rects, 4, 210, 110));
    /* Just outside the chip but within the 28 px minimum. */
    ASSERT_EQ(2, hit_test(rects, 4, 197, 98));
    ASSERT_EQ(-1, hit_test(rects, 4, 150, 150));
    ASSERT_EQ(-1, hit_test(rects, 0, 20, 20));
    PASS();
}

TEST hit_test_nearest_small_target(void)
{
    /* Two 20 px chips 4 px apart; the touch between them goes to the nearer one. */
    HitRect rects[] = {{0, 0, 20, 20}, {24, 0, 20, 20}};
    ASSERT_EQ(0, hit_test(rects, 2, 21, 10));
    ASSERT_EQ(1, hit_test(rects, 2, 23, 10));
    PASS();
}

TEST grid_cells_and_gaps(void)
{
    /* 40-piece raster: 8 x 5 cells of 33 x 29 with 2 px gaps. */
    int x0 = 12, y0 = 32;
    ASSERT_EQ(0, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 12, 32));
    ASSERT_EQ(0, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 44, 60));
    ASSERT_EQ(1, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 47, 32));
    ASSERT_EQ(39, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 12 + 8 * 35 - 3, 32 + 5 * 31 - 3));
    /* Gap pixels: first goes left/up, second right/down. */
    ASSERT_EQ(0, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 45, 32));
    ASSERT_EQ(1, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 46, 32));
    ASSERT_EQ(8, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 12, 62));
    /* Outside. */
    ASSERT_EQ(-1, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 11, 40));
    ASSERT_EQ(-1, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 12 + 8 * 35 - 2, 40));
    ASSERT_EQ(-1, hit_grid(x0, y0, 8, 5, 33, 29, 2, 2, 20, 32 + 5 * 31 - 2));
    ASSERT_EQ(-1, hit_grid(x0, y0, 0, 5, 33, 29, 2, 2, 20, 40));
    PASS();
}

TEST tap_activates_on_release_inside(void)
{
    HitTap tap;
    hit_tap_reset(&tap);
    ASSERT_EQ(-1, hit_tap_update(&tap, true, 3));
    ASSERT_EQ(3, hit_tap_pressed(&tap));
    ASSERT_EQ(-1, hit_tap_update(&tap, true, 3));
    ASSERT_EQ(3, hit_tap_update(&tap, false, -1));
    ASSERT_EQ(-1, hit_tap_pressed(&tap));
    PASS();
}

TEST tap_slide_off_cancels_slide_back_rearms(void)
{
    HitTap tap;
    hit_tap_reset(&tap);
    hit_tap_update(&tap, true, 2);
    hit_tap_update(&tap, true, 5); /* slid onto another target */
    ASSERT_EQ(-1, hit_tap_pressed(&tap));
    hit_tap_update(&tap, true, 2); /* back */
    ASSERT_EQ(2, hit_tap_pressed(&tap));
    hit_tap_update(&tap, true, -1);
    ASSERT_EQ(-1, hit_tap_update(&tap, false, -1));
    PASS();
}

TEST tap_started_on_nothing_stays_dead(void)
{
    HitTap tap;
    hit_tap_reset(&tap);
    hit_tap_update(&tap, true, -1);
    hit_tap_update(&tap, true, 4); /* dragged onto a target */
    ASSERT_EQ(-1, hit_tap_pressed(&tap));
    ASSERT_EQ(-1, hit_tap_update(&tap, false, -1));
    /* The next touch works normally. */
    hit_tap_update(&tap, true, 4);
    ASSERT_EQ(4, hit_tap_update(&tap, false, -1));
    PASS();
}

TEST focus_hidden_by_touch_revealed_by_key(void)
{
    FocusState f;
    focus_init(&f, 5, true);
    ASSERT_EQ(K_DOWN, focus_update(&f, K_DOWN, K_NAV | K_A, false));
    ASSERT(f.visible);

    ASSERT_EQ(0u, focus_update(&f, 0, K_NAV | K_A, true));
    ASSERT_FALSE(f.visible);

    /* First direction only shows the ring at the old place. */
    ASSERT_EQ(0u, focus_update(&f, K_DOWN, K_NAV | K_A, false));
    ASSERT(f.visible);
    ASSERT_EQ(5, f.index);
    ASSERT_EQ(K_DOWN, focus_update(&f, K_DOWN, K_NAV | K_A, false));
    PASS();
}

TEST focus_b_works_while_hidden(void)
{
    FocusState f;
    focus_init(&f, 0, false);
    ASSERT_EQ(K_B, focus_update(&f, K_B, K_NAV | K_A, false));
    ASSERT_FALSE(f.visible);
    /* A only reveals, never confirms a hidden focus. */
    ASSERT_EQ(K_B, focus_update(&f, K_A | K_B, K_NAV | K_A, false));
    ASSERT(f.visible);
    /* Touch in the same frame as a key counts as touch. */
    ASSERT_EQ(K_B, focus_update(&f, K_A | K_B, K_NAV | K_A, true));
    ASSERT_FALSE(f.visible);
    PASS();
}

TEST grid_navigation(void)
{
    /* 3 x 2 gallery page with 5 cards: last row is shorter. */
    int i = 0;
    ASSERT_EQ(FOCUS_MOVED, focus_grid_move(&i, 3, 5, FOCUS_RIGHT, 0));
    ASSERT_EQ(1, i);
    ASSERT_EQ(FOCUS_MOVED, focus_grid_move(&i, 3, 5, FOCUS_DOWN, 0));
    ASSERT_EQ(4, i);
    ASSERT_EQ(FOCUS_BLOCKED, focus_grid_move(&i, 3, 5, FOCUS_RIGHT, 0));
    ASSERT_EQ(4, i);
    ASSERT_EQ(FOCUS_EXIT_RIGHT, focus_grid_move(&i, 3, 5, FOCUS_RIGHT, FOCUS_EXITS(FOCUS_RIGHT)));
    ASSERT_EQ(4, i);

    i = 2;
    ASSERT_EQ(FOCUS_MOVED, focus_grid_move(&i, 3, 5, FOCUS_DOWN, 0));
    ASSERT_EQ(4, i); /* no cell below 2: last cell */

    i = 0;
    ASSERT_EQ(FOCUS_BLOCKED, focus_grid_move(&i, 3, 5, FOCUS_UP, 0));
    ASSERT_EQ(FOCUS_BLOCKED, focus_grid_move(&i, 3, 5, FOCUS_LEFT, 0));
    ASSERT_EQ(FOCUS_EXIT_LEFT, focus_grid_move(&i, 3, 5, FOCUS_LEFT, FOCUS_EXITS(FOCUS_LEFT)));

    /* S3: below the last row of the raster into the button bar. */
    i = 37;
    ASSERT_EQ(FOCUS_EXIT_DOWN, focus_grid_move(&i, 8, 40, FOCUS_DOWN, FOCUS_EXITS(FOCUS_DOWN)));
    ASSERT_EQ(37, i);
    ASSERT_EQ(FOCUS_MOVED, focus_grid_move(&i, 8, 40, FOCUS_UP, FOCUS_EXITS(FOCUS_DOWN)));
    ASSERT_EQ(29, i);

    i = 9;
    ASSERT_EQ(FOCUS_BLOCKED, focus_grid_move(&i, 3, 5, FOCUS_UP, 0));
    PASS();
}

SUITE(input_suite)
{
    RUN_TEST(repeat_18_then_every_5);
    RUN_TEST(repeat_long_hold_stays_in_phase);
    RUN_TEST(non_repeating_keys_fire_once);
    RUN_TEST(release_and_reset);
    RUN_TEST(reset_silences_held_keys_until_released);
    RUN_TEST(reset_keeps_new_presses);
    RUN_TEST(independent_keys);
    RUN_TEST(rect_contains_half_open);
    RUN_TEST(expand_small_targets);
    RUN_TEST(hit_test_topmost_and_slop);
    RUN_TEST(hit_test_nearest_small_target);
    RUN_TEST(grid_cells_and_gaps);
    RUN_TEST(tap_activates_on_release_inside);
    RUN_TEST(tap_slide_off_cancels_slide_back_rearms);
    RUN_TEST(tap_started_on_nothing_stays_dead);
    RUN_TEST(focus_hidden_by_touch_revealed_by_key);
    RUN_TEST(focus_b_works_while_hidden);
    RUN_TEST(grid_navigation);
}
