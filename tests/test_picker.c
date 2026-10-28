#include <string.h>

#include "test_prices.h"
#include "greatest.h"
#include "core/gallery_nav.h"
#include "core/picker.h"

#define BIT(i) ((uint64_t)1 << (i))

static PanelState make_panel(unsigned count, uint64_t owned, uint64_t rare)
{
    PanelState p;
    memset(&p, 0, sizeof(p));
    p.id = 7;
    p.available = true;
    p.piece_count = (uint8_t)count;
    p.owned = owned;
    p.rare = rare;
    return p;
}

TEST gallery_pages_round_up(void)
{
    ASSERT_EQ(1, gallery_pages(0));
    ASSERT_EQ(1, gallery_pages(6));
    ASSERT_EQ(2, gallery_pages(7));
    ASSERT_EQ(11, gallery_pages(63));
    ASSERT_EQ(3, gallery_page_items(63, 10));
    ASSERT_EQ(6, gallery_page_items(63, 0));
    ASSERT_EQ(0, gallery_page_items(63, 11));
    PASS();
}

TEST gallery_page_step_keeps_place(void)
{
    unsigned i = 4; /* page 0, row 1, middle column */
    ASSERT(gallery_page_step(&i, 1, 63));
    ASSERT_EQ(10, i);
    ASSERT(gallery_page_step(&i, -1, 63));
    ASSERT_EQ(4, i);
    ASSERT_FALSE(gallery_page_step(&i, -1, 63));
    ASSERT_EQ(4, i);
    PASS();
}

TEST gallery_page_step_clamps_to_short_last_page(void)
{
    unsigned i = 59; /* page 9, row 1, last column */
    ASSERT(gallery_page_step(&i, 1, 63));
    ASSERT_EQ(62, i); /* page 10 has the cards 60, 61, 62 */
    ASSERT_FALSE(gallery_page_step(&i, 1, 63));
    PASS();
}

TEST gallery_move_stays_on_the_page(void)
{
    unsigned i = 0;
    ASSERT_EQ(FOCUS_MOVED, gallery_move(&i, FOCUS_RIGHT, 63));
    ASSERT_EQ(1, i);
    ASSERT_EQ(FOCUS_MOVED, gallery_move(&i, FOCUS_DOWN, 63));
    ASSERT_EQ(4, i);
    ASSERT_EQ(FOCUS_BLOCKED, gallery_move(&i, FOCUS_DOWN, 63));
    i = 5;
    ASSERT_EQ(FOCUS_BLOCKED, gallery_move(&i, FOCUS_RIGHT, 63));
    ASSERT_EQ(5, i);
    i = 6;
    ASSERT_EQ(FOCUS_BLOCKED, gallery_move(&i, FOCUS_LEFT, 63));
    ASSERT_EQ(6, i);
    PASS();
}

TEST gallery_move_on_short_last_page(void)
{
    unsigned i = 61;
    ASSERT_EQ(FOCUS_MOVED, gallery_move(&i, FOCUS_RIGHT, 63));
    ASSERT_EQ(62, i);
    ASSERT_EQ(FOCUS_BLOCKED, gallery_move(&i, FOCUS_RIGHT, 63));
    PASS();
}

TEST summary_counts_cost_and_affordability(void)
{
    PanelState p = make_panel(40, 0x0FFFFFFFFFull, 0);
    ShopPrices prices = test_prices();
    PickerSummary s = picker_summary(&p, BIT(36) | BIT(37) | BIT(38), &prices, 6);
    ASSERT_EQ(3, s.count);
    ASSERT_EQ(6, s.cost);
    ASSERT(s.affordable);
    ASSERT(s.can_buy);
    ASSERT_EQ(36 + 3, s.owned_after);
    s = picker_summary(&p, BIT(36) | BIT(37) | BIT(38), &prices, 5);
    ASSERT_FALSE(s.affordable);
    ASSERT_FALSE(s.can_buy);
    s = picker_summary(&p, 0, &prices, 100);
    ASSERT_FALSE(s.can_buy);
    PASS();
}

TEST summary_prices_rare_pieces(void)
{
    PanelState p = make_panel(15, 0, BIT(3));
    ShopPrices prices = test_prices();
    PickerSummary s = picker_summary(&p, BIT(3) | BIT(4), &prices, 12);
    ASSERT_EQ(12, s.cost);
    ASSERT(s.can_buy);
    PASS();
}

TEST toggle_selects_and_deselects(void)
{
    PanelState p = make_panel(15, BIT(0), 0);
    ShopPrices prices = test_prices();
    uint64_t sel = 0;
    ASSERT_EQ(PICK_ON, picker_toggle(&sel, &p, &prices, 5));
    ASSERT_EQ(BIT(5), sel);
    ASSERT_EQ(PICK_OFF, picker_toggle(&sel, &p, &prices, 5));
    ASSERT_EQ(0, sel);
    PASS();
}

TEST toggle_refuses_what_cannot_be_bought(void)
{
    PanelState p = make_panel(15, BIT(0), BIT(2));
    ShopPrices prices = test_prices();
    prices.per_rare_piece = 0;
    uint64_t sel = 0;
    ASSERT_EQ(PICK_REFUSED, picker_toggle(&sel, &p, &prices, 0));  /* owned */
    ASSERT_EQ(PICK_REFUSED, picker_toggle(&sel, &p, &prices, 2));  /* rare, not for sale */
    ASSERT_EQ(PICK_REFUSED, picker_toggle(&sel, &p, &prices, 15)); /* beyond the panel */
    ASSERT_EQ(PICK_REFUSED, picker_toggle(&sel, &p, &prices, 99));
    ASSERT_EQ(0, sel);
    p.available = false;
    ASSERT_EQ(PICK_REFUSED, picker_toggle(&sel, &p, &prices, 5));
    PASS();
}

TEST select_all_toggles(void)
{
    PanelState p = make_panel(15, BIT(0) | BIT(1), 0);
    ShopPrices prices = test_prices();
    uint64_t all = 0x7FFFull & ~(BIT(0) | BIT(1));
    uint64_t sel = BIT(4);
    ASSERT(picker_select_all(&sel, &p, &prices));
    ASSERT_EQ(all, sel);
    ASSERT_FALSE(picker_select_all(&sel, &p, &prices));
    ASSERT_EQ(0, sel);
    PASS();
}

TEST select_all_skips_pieces_not_for_sale(void)
{
    PanelState p = make_panel(15, 0, BIT(14));
    ShopPrices prices = test_prices();
    prices.per_rare_piece = 0;
    uint64_t sel = 0;
    ASSERT(picker_select_all(&sel, &p, &prices));
    ASSERT_EQ(0x3FFFull, sel);
    PASS();
}

TEST select_all_on_complete_panel_selects_nothing(void)
{
    PanelState p = make_panel(15, 0x7FFF, 0);
    ShopPrices prices = test_prices();
    uint64_t sel = 0;
    ASSERT_FALSE(picker_select_all(&sel, &p, &prices));
    ASSERT_EQ(0, sel);
    PASS();
}

TEST focus_enters_the_bar_from_the_last_row(void)
{
    PickerFocus f;
    picker_focus_init(&f, 0);
    /* 15 pieces, 5 columns: row 2 is 10..14 */
    f.index = 11;
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_DOWN, 5, 15));
    ASSERT(picker_focus_in_bar(&f));
    ASSERT_EQ(PICKER_BAR_BACK, f.bar); /* column 1 is in the left half */
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_RIGHT, 5, 15));
    ASSERT_EQ(PICKER_BAR_BUY, f.bar);
    ASSERT_EQ(FOCUS_BLOCKED, picker_focus_move(&f, FOCUS_RIGHT, 5, 15));
    ASSERT_EQ(FOCUS_BLOCKED, picker_focus_move(&f, FOCUS_DOWN, 5, 15));
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_UP, 5, 15));
    ASSERT_EQ(11, f.index);
    PASS();
}

TEST focus_right_half_lands_on_buy(void)
{
    PickerFocus f;
    picker_focus_init(&f, 14);
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_DOWN, 5, 15));
    ASSERT_EQ(PICKER_BAR_BUY, f.bar);
    PASS();
}

TEST focus_moves_in_the_grid(void)
{
    PickerFocus f;
    picker_focus_init(&f, 0);
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_RIGHT, 8, 40));
    ASSERT_EQ(1, f.index);
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_DOWN, 8, 40));
    ASSERT_EQ(9, f.index);
    ASSERT_EQ(FOCUS_MOVED, picker_focus_move(&f, FOCUS_UP, 8, 40));
    ASSERT_EQ(1, f.index);
    ASSERT_EQ(FOCUS_BLOCKED, picker_focus_move(&f, FOCUS_UP, 8, 40));
    ASSERT_EQ(1, f.index);
    PASS();
}

SUITE(picker_suite)
{
    RUN_TEST(gallery_pages_round_up);
    RUN_TEST(gallery_page_step_keeps_place);
    RUN_TEST(gallery_page_step_clamps_to_short_last_page);
    RUN_TEST(gallery_move_stays_on_the_page);
    RUN_TEST(gallery_move_on_short_last_page);
    RUN_TEST(summary_counts_cost_and_affordability);
    RUN_TEST(summary_prices_rare_pieces);
    RUN_TEST(toggle_selects_and_deselects);
    RUN_TEST(toggle_refuses_what_cannot_be_bought);
    RUN_TEST(select_all_toggles);
    RUN_TEST(select_all_skips_pieces_not_for_sale);
    RUN_TEST(select_all_on_complete_panel_selects_nothing);
    RUN_TEST(focus_enters_the_bar_from_the_last_row);
    RUN_TEST(focus_right_half_lands_on_buy);
    RUN_TEST(focus_moves_in_the_grid);
}
