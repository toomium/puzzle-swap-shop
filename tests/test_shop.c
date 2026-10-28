#include <stdlib.h>
#include <string.h>

#include "test_prices.h"
#include "greatest.h"
#include "fixture.h"
#include "core/gamecoin.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"
#include "core/shop.h"

#define BIT(i) ((uint64_t)1 << (i))

static PanelState make_panel(uint8_t id, uint8_t pieces, uint64_t owned, uint64_t rare)
{
    PanelState p;
    memset(&p, 0, sizeof(p));
    p.id = id;
    p.available = true;
    p.piece_count = pieces;
    p.owned = owned;
    p.rare = rare;
    return p;
}

TEST default_prices_follow_decisions(void)
{
    ShopPrices prices = shop_default_prices();
    ASSERT_EQ(5, prices.per_piece);
    ASSERT_EQ(10, prices.per_rare_piece);
    PASS();
}

TEST missing_and_buyable(void)
{
    ShopPrices prices = test_prices();
    PanelState p = make_panel(0, 15, panel_bits_all(14), BIT(14));
    ASSERT_EQ(BIT(14), shop_missing(&p));
    ASSERT_EQ(BIT(14), shop_buyable(&p, &prices));

    prices.per_rare_piece = 0;
    ASSERT_EQ(0, shop_buyable(&p, &prices));

    prices = test_prices();
    p.available = false;
    ASSERT_EQ(0, shop_buyable(&p, &prices));
    PASS();
}

TEST cost_counts_rare_pieces(void)
{
    ShopPrices prices = test_prices();
    PanelState p = make_panel(9, 40, 0, BIT(30));
    ASSERT_EQ(6, shop_cost(&p, BIT(12) | BIT(14) | BIT(22), &prices));
    ASSERT_EQ(14, shop_cost(&p, BIT(12) | BIT(14) | BIT(30), &prices));
    PASS();
}

TEST plan_ok(void)
{
    ShopPrices prices = test_prices();
    PanelState p = make_panel(9, 40, 0, BIT(30));
    PurchasePlan plan;
    ASSERT_EQ(SHOP_OK, shop_plan_build(&p, BIT(12) | BIT(30), 120, &prices, &plan));
    ASSERT_EQ(9, plan.panel_id);
    ASSERT_EQ(BIT(12) | BIT(30), plan.pieces);
    ASSERT_EQ(1, plan.normal_count);
    ASSERT_EQ(1, plan.rare_count);
    ASSERT_EQ(12, plan.cost);
    ASSERT_EQ(120, plan.coins_before);
    PASS();
}

TEST plan_rejections(void)
{
    ShopPrices prices = test_prices();
    PanelState p = make_panel(0, 15, BIT(0), BIT(5));
    PurchasePlan plan;

    ASSERT_EQ(SHOP_ERR_EMPTY, shop_plan_build(&p, 0, 100, &prices, &plan));
    ASSERT_EQ(SHOP_ERR_OUT_OF_RANGE, shop_plan_build(&p, BIT(15), 100, &prices, &plan));
    ASSERT_EQ(SHOP_ERR_ALREADY_OWNED, shop_plan_build(&p, BIT(0), 100, &prices, &plan));
    ASSERT_EQ(SHOP_ERR_NOT_ENOUGH_COINS, shop_plan_build(&p, BIT(1), 1, &prices, &plan));
    ASSERT_EQ(SHOP_ERR_NOT_ENOUGH_COINS, shop_plan_build(&p, BIT(5), 9, &prices, &plan));
    ASSERT_EQ(SHOP_OK, shop_plan_build(&p, BIT(5), 10, &prices, &plan));

    prices.per_rare_piece = 0;
    ASSERT_EQ(SHOP_ERR_RARE_NOT_FOR_SALE, shop_plan_build(&p, BIT(5), 100, &prices, &plan));

    p.available = false;
    ASSERT_EQ(SHOP_ERR_UNAVAILABLE, shop_plan_build(&p, BIT(1), 100, &prices, &plan));
    PASS();
}

TEST apply_on_fixtures(void)
{
    size_t meet_len, coin_len;
    uint8_t *meet = fixture_load("meet_sample.dat", &meet_len);
    uint8_t *coin = fixture_load("gamecoin_120.dat", &coin_len);
    ASSERT(meet != NULL && coin != NULL);

    ShopPrices prices = test_prices();
    PanelState p = make_panel(9, 40, 0, 0);
    meet_save_get_owned(meet, meet_len, 9, &p.owned);

    PurchasePlan plan;
    uint64_t pieces = BIT(12) | BIT(14) | BIT(22);
    ASSERT_EQ(SHOP_OK, shop_plan_build(&p, pieces, 120, &prices, &plan));
    ASSERT_EQ(SHOP_OK, shop_plan_apply(meet, meet_len, coin, coin_len, &plan));

    uint64_t owned;
    uint16_t coins;
    meet_save_get_owned(meet, meet_len, 9, &owned);
    gamecoin_get(coin, coin_len, &coins);
    ASSERT_EQ(p.owned | pieces, owned);
    ASSERT_EQ(114, coins);

    /* Applying the same plan again must fail: the data has moved on. */
    ASSERT_EQ(SHOP_ERR_STALE, shop_plan_apply(meet, meet_len, coin, coin_len, &plan));

    fixture_free(meet);
    fixture_free(coin);
    PASS();
}

TEST apply_rejects_stale_coins_without_change(void)
{
    size_t meet_len, coin_len;
    uint8_t *meet = fixture_load("meet_sample.dat", &meet_len);
    uint8_t *coin = fixture_load("gamecoin_120.dat", &coin_len);
    ASSERT(meet != NULL && coin != NULL);
    uint8_t *meet_before = malloc(meet_len);
    ASSERT(meet_before != NULL);
    memcpy(meet_before, meet, meet_len);

    PurchasePlan plan = {
        .panel_id = 1, .pieces = BIT(0), .normal_count = 1, .cost = 2, .coins_before = 100};
    ASSERT_EQ(SHOP_ERR_STALE, shop_plan_apply(meet, meet_len, coin, coin_len, &plan));
    ASSERT_EQ(0, memcmp(meet_before, meet, meet_len));

    uint16_t coins;
    gamecoin_get(coin, coin_len, &coins);
    ASSERT_EQ(120, coins);

    free(meet_before);
    fixture_free(meet);
    fixture_free(coin);
    PASS();
}

TEST apply_rejects_invalid_data(void)
{
    size_t meet_len, coin_len;
    uint8_t *meet = fixture_load("meet_bad_unused.dat", &meet_len);
    uint8_t *coin = fixture_load("gamecoin_120.dat", &coin_len);
    ASSERT(meet != NULL && coin != NULL);

    PurchasePlan plan = {
        .panel_id = 1, .pieces = BIT(0), .normal_count = 1, .cost = 2, .coins_before = 120};
    ASSERT_EQ(SHOP_ERR_DATA, shop_plan_apply(meet, meet_len, coin, coin_len, &plan));

    fixture_free(meet);
    fixture_free(coin);
    PASS();
}

SUITE(shop_suite)
{
    RUN_TEST(default_prices_follow_decisions);
    RUN_TEST(missing_and_buyable);
    RUN_TEST(cost_counts_rare_pieces);
    RUN_TEST(plan_ok);
    RUN_TEST(plan_rejections);
    RUN_TEST(apply_on_fixtures);
    RUN_TEST(apply_rejects_stale_coins_without_change);
    RUN_TEST(apply_rejects_invalid_data);
}
