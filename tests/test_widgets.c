#include <string.h>

#include "greatest.h"
#include "core/coin_counter.h"
#include "core/press.h"
#include "core/toast.h"
#include "core/widget_math.h"

#define RELEASE_FRAMES 6

TEST press_is_one_pixel_while_held(void)
{
    Press p;
    press_init(&p);
    ASSERT_EQ(0, press_offset(&p));
    press_update(&p, true, RELEASE_FRAMES);
    ASSERT_EQ(1, press_offset(&p));
    press_update(&p, true, RELEASE_FRAMES);
    ASSERT_EQ(1, press_offset(&p));
    PASS();
}

TEST press_springs_back_within_six_frames(void)
{
    Press p;
    press_init(&p);
    press_update(&p, true, RELEASE_FRAMES);
    press_update(&p, false, RELEASE_FRAMES);
    int last = 1;
    for (int f = 0; f < RELEASE_FRAMES + 1; f++) {
        int off = press_offset(&p);
        ASSERT(off >= 0 && off <= last); /* never above the rest position, never back down */
        last = off;
        press_update(&p, false, RELEASE_FRAMES);
    }
    ASSERT_EQ(0, press_offset(&p));
    PASS();
}

TEST press_again_during_spring_back(void)
{
    Press p;
    press_init(&p);
    press_update(&p, true, RELEASE_FRAMES);
    press_update(&p, false, RELEASE_FRAMES);
    press_update(&p, true, RELEASE_FRAMES);
    ASSERT_EQ(1, press_offset(&p));
    PASS();
}

TEST toast_fades_in_holds_and_ends(void)
{
    Toast t;
    toast_init(&t, 8, 60, 12);
    ASSERT(!toast_visible(&t));
    ASSERT_EQ(0, toast_alpha(&t));
    toast_show(&t, "Zu wenig Spielmünzen");
    ASSERT(toast_visible(&t));
    ASSERT_STR_EQ("Zu wenig Spielmünzen", t.text);
    ASSERT_EQ(0, toast_alpha(&t));
    ASSERT_EQ(4, toast_offset(&t, 4));

    unsigned prev = 0;
    for (int f = 0; f < 8; f++) {
        toast_step(&t);
        ASSERT(toast_alpha(&t) >= prev);
        prev = toast_alpha(&t);
    }
    ASSERT_EQ(255, toast_alpha(&t));
    ASSERT_EQ(0, toast_offset(&t, 4));
    for (int f = 0; f < 60; f++)
        toast_step(&t);
    ASSERT_EQ(255, toast_alpha(&t));
    for (int f = 0; f < 11; f++) {
        toast_step(&t);
        ASSERT(toast_visible(&t));
        ASSERT(toast_alpha(&t) <= prev);
        prev = toast_alpha(&t);
    }
    ASSERT(toast_alpha(&t) < 40);
    toast_step(&t);
    ASSERT(!toast_visible(&t));
    ASSERT_EQ(0, toast_alpha(&t));
    PASS();
}

TEST toast_restarts_with_new_text(void)
{
    Toast t;
    toast_init(&t, 8, 60, 12);
    toast_show(&t, "eins");
    for (int f = 0; f < 30; f++)
        toast_step(&t);
    toast_show(&t, "zwei");
    ASSERT_STR_EQ("zwei", t.text);
    ASSERT_EQ(0, t.age);
    PASS();
}

TEST toast_cuts_long_text_on_a_character_boundary(void)
{
    char text[200];
    for (int i = 0; i < 199; i++)
        text[i] = 'a';
    text[199] = '\0';
    /* Put a two-byte character across the limit. */
    text[TOAST_TEXT_MAX - 2] = (char)0xC3;
    text[TOAST_TEXT_MAX - 1] = (char)0xA4;

    Toast t;
    toast_init(&t, 8, 60, 12);
    toast_show(&t, text);
    size_t n = strlen(t.text);
    ASSERT(n < TOAST_TEXT_MAX);
    /* The last kept byte is not the first half of a split character. */
    ASSERT(((unsigned char)t.text[n - 1] & 0xC0) != 0xC0);

    text[TOAST_TEXT_MAX - 3] = (char)0xC3;
    text[TOAST_TEXT_MAX - 2] = (char)0xA4;
    text[TOAST_TEXT_MAX - 1] = 'x';
    toast_show(&t, text);
    n = strlen(t.text);
    ASSERT_EQ(TOAST_TEXT_MAX - 1, (int)n);
    PASS();
}

TEST coin_counter_counts_and_sounds(void)
{
    CoinCounter c;
    coin_counter_init(&c, 120, 2, 45, 5);
    ASSERT_EQ(120, coin_counter_value(&c));
    ASSERT(coin_counter_done(&c));
    ASSERT(!coin_counter_shining(&c));

    coin_counter_set(&c, 114); /* 6 coins, 12 frames */
    ASSERT(!coin_counter_done(&c));
    int sounds = 0, frames = 0;
    while (!coin_counter_done(&c)) {
        if (coin_counter_step(&c))
            sounds++;
        frames++;
        if (!coin_counter_done(&c))
            ASSERT(coin_counter_shining(&c));
    }
    ASSERT_EQ(12, frames);
    ASSERT_EQ(114, coin_counter_value(&c));
    ASSERT_EQ(1, sounds); /* one sound per 5 counted coins */
    ASSERT(!coin_counter_shining(&c));
    PASS();
}

TEST coin_counter_never_longer_than_45_frames(void)
{
    CoinCounter c;
    coin_counter_init(&c, 300, 2, 45, 5);
    coin_counter_set(&c, 0);
    int frames = 0;
    while (!coin_counter_done(&c)) {
        coin_counter_step(&c);
        frames++;
    }
    ASSERT_EQ(45, frames);
    ASSERT_EQ(0, coin_counter_value(&c));
    PASS();
}

TEST coin_counter_restarts_from_shown_value(void)
{
    CoinCounter c;
    coin_counter_init(&c, 100, 2, 45, 5);
    coin_counter_set(&c, 90);
    for (int f = 0; f < 10; f++)
        coin_counter_step(&c);
    ASSERT_EQ(95, coin_counter_value(&c));
    coin_counter_set(&c, 100);
    ASSERT_EQ(95, coin_counter_value(&c));
    while (!coin_counter_done(&c))
        coin_counter_step(&c);
    ASSERT_EQ(100, coin_counter_value(&c));
    PASS();
}

TEST progress_fill_widths(void)
{
    ASSERT_EQ(0, widget_progress_fill(150, 0, 40));
    ASSERT_EQ(105, widget_progress_fill(150, 28, 40));
    ASSERT_EQ(150, widget_progress_fill(150, 40, 40));
    ASSERT_EQ(150, widget_progress_fill(150, 99, 40));
    ASSERT_EQ(0, widget_progress_fill(150, 5, 0));
    ASSERT_EQ(0, widget_progress_fill(0, 5, 10));
    PASS();
}

TEST board_cells_are_row_major(void)
{
    GeomRect art = {100, 69, 200, 125};
    GeomRect c0 = widget_board_cell(art, 8, 25, 0);
    ASSERT_EQ(100, c0.x);
    ASSERT_EQ(69, c0.y);
    ASSERT_EQ(25, c0.w);
    GeomRect c9 = widget_board_cell(art, 8, 25, 9);
    ASSERT_EQ(125, c9.x);
    ASSERT_EQ(94, c9.y);
    GeomRect last = widget_board_cell(art, 8, 25, 39);
    ASSERT_EQ(275, last.x);
    ASSERT_EQ(169, last.y);
    ASSERT_EQ(300, last.x + last.w);
    ASSERT_EQ(194, last.y + last.h);
    PASS();
}

TEST scale_rect_keeps_the_center(void)
{
    GeomRect r = {10, 20, 40, 30};
    GeomRect same = widget_scale_rect(r, 1.0f);
    ASSERT_EQ(10, same.x);
    ASSERT_EQ(20, same.y);
    ASSERT_EQ(40, same.w);
    ASSERT_EQ(30, same.h);

    GeomRect big = widget_scale_rect(r, 1.5f); /* 60 x 45 */
    ASSERT_EQ(60, big.w);
    ASSERT_EQ(45, big.h);
    ASSERT_EQ(0, big.x);
    ASSERT_EQ(13, big.y);

    GeomRect none = widget_scale_rect(r, -1.0f);
    ASSERT_EQ(0, none.w);
    ASSERT_EQ(0, none.h);
    PASS();
}

TEST mask_bits(void)
{
    ASSERT(widget_mask_has(1, 0));
    ASSERT(!widget_mask_has(1, 1));
    ASSERT(widget_mask_has(1ull << 39, 39));
    ASSERT(widget_mask_has(1ull << 63, 63));
    ASSERT(!widget_mask_has(~0ull, 64));
    ASSERT(!widget_mask_has(~0ull, -1));
    PASS();
}

SUITE(widgets_suite)
{
    RUN_TEST(press_is_one_pixel_while_held);
    RUN_TEST(press_springs_back_within_six_frames);
    RUN_TEST(press_again_during_spring_back);
    RUN_TEST(toast_fades_in_holds_and_ends);
    RUN_TEST(toast_restarts_with_new_text);
    RUN_TEST(toast_cuts_long_text_on_a_character_boundary);
    RUN_TEST(coin_counter_counts_and_sounds);
    RUN_TEST(coin_counter_never_longer_than_45_frames);
    RUN_TEST(coin_counter_restarts_from_shown_value);
    RUN_TEST(progress_fill_widths);
    RUN_TEST(board_cells_are_row_major);
    RUN_TEST(scale_rect_keeps_the_center);
    RUN_TEST(mask_bits);
}
