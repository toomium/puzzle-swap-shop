#include "core/shop.h"

#include "core/gamecoin.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"

ShopPrices shop_default_prices(void)
{
    ShopPrices prices = {SHOP_PRICE_PER_PIECE, SHOP_PRICE_PER_RARE_PIECE};
    return prices;
}

uint64_t shop_missing(const PanelState *panel)
{
    return panel_bits_all(panel->piece_count) & ~panel->owned;
}

uint64_t shop_buyable(const PanelState *panel, const ShopPrices *prices)
{
    if (!panel->available)
        return 0;
    uint64_t missing = shop_missing(panel);
    if (prices->per_rare_piece == 0)
        missing &= ~panel->rare;
    return missing;
}

uint16_t shop_cost(const PanelState *panel, uint64_t pieces, const ShopPrices *prices)
{
    unsigned rare = panel_bits_count(pieces & panel->rare);
    unsigned normal = panel_bits_count(pieces & ~panel->rare);
    return (uint16_t)(normal * prices->per_piece + rare * prices->per_rare_piece);
}

ShopResult shop_plan_build(const PanelState *panel, uint64_t pieces, uint16_t coins,
                           const ShopPrices *prices, PurchasePlan *out)
{
    if (pieces == 0)
        return SHOP_ERR_EMPTY;
    if (!panel->available)
        return SHOP_ERR_UNAVAILABLE;
    if (pieces & ~panel_bits_all(panel->piece_count))
        return SHOP_ERR_OUT_OF_RANGE;
    if (pieces & panel->owned)
        return SHOP_ERR_ALREADY_OWNED;
    if ((pieces & panel->rare) && prices->per_rare_piece == 0)
        return SHOP_ERR_RARE_NOT_FOR_SALE;

    uint16_t cost = shop_cost(panel, pieces, prices);
    if (cost > coins)
        return SHOP_ERR_NOT_ENOUGH_COINS;

    out->panel_id = panel->id;
    out->pieces = pieces;
    out->rare_count = (uint8_t)panel_bits_count(pieces & panel->rare);
    out->normal_count = (uint8_t)panel_bits_count(pieces & ~panel->rare);
    out->cost = cost;
    out->coins_before = coins;
    return SHOP_OK;
}

ShopResult shop_plan_apply(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len,
                           const PurchasePlan *plan)
{
    uint64_t owned;
    uint16_t coins;

    if (meet_save_validate(meet, meet_len) != MEET_OK)
        return SHOP_ERR_DATA;
    if (gamecoin_validate(coin, coin_len) != COIN_OK)
        return SHOP_ERR_DATA;
    if (meet_save_get_owned(meet, meet_len, plan->panel_id, &owned) != MEET_OK)
        return SHOP_ERR_DATA;
    gamecoin_get(coin, coin_len, &coins);

    if (plan->pieces == 0)
        return SHOP_ERR_EMPTY;
    if ((plan->pieces & owned) || coins != plan->coins_before)
        return SHOP_ERR_STALE;
    if (plan->cost > coins)
        return SHOP_ERR_NOT_ENOUGH_COINS;

    /* add_pieces checks before it writes, so on failure nothing has changed yet. */
    if (meet_save_add_pieces(meet, meet_len, plan->panel_id, plan->pieces) != MEET_OK)
        return SHOP_ERR_DATA;
    /* Cannot fail: size, range and balance were checked above. */
    gamecoin_spend(coin, coin_len, plan->cost);
    return SHOP_OK;
}
