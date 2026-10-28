#ifndef PSS_CORE_SHOP_H
#define PSS_CORE_SHOP_H

#include <stddef.h>
#include <stdint.h>

#include "core/panel_state.h"

typedef struct {
    uint16_t per_piece;      /* default 5 (decision D-3, changed 28.10.2026) */
    uint16_t per_rare_piece; /* default 10; 0 = not for sale */
} ShopPrices;

#define SHOP_PRICE_PER_PIECE      5
#define SHOP_PRICE_PER_RARE_PIECE 10

typedef enum {
    SHOP_OK = 0,
    SHOP_ERR_EMPTY,             /* nothing selected */
    SHOP_ERR_UNAVAILABLE,       /* panel not on this console */
    SHOP_ERR_OUT_OF_RANGE,      /* piece beyond the panel's piece count */
    SHOP_ERR_ALREADY_OWNED,     /* piece already owned */
    SHOP_ERR_RARE_NOT_FOR_SALE, /* rare piece while per_rare_piece is 0 */
    SHOP_ERR_NOT_ENOUGH_COINS,
    SHOP_ERR_STALE, /* save data changed since the plan was built */
    SHOP_ERR_DATA,  /* save data invalid */
} ShopResult;

typedef struct {
    uint8_t panel_id;
    uint64_t pieces;
    uint8_t normal_count;
    uint8_t rare_count;
    uint16_t cost;
    uint16_t coins_before;
} PurchasePlan;

ShopPrices shop_default_prices(void);

uint64_t shop_missing(const PanelState *panel);

/* Missing pieces that can be bought with the given prices. */
uint64_t shop_buyable(const PanelState *panel, const ShopPrices *prices);

/* Price of the given pieces; rare pieces count at per_rare_piece. */
uint16_t shop_cost(const PanelState *panel, uint64_t pieces, const ShopPrices *prices);

ShopResult shop_plan_build(const PanelState *panel, uint64_t pieces, uint16_t coins,
                           const ShopPrices *prices, PurchasePlan *out);

/*
 * Applies a plan to in-memory copies of meet.dat and gamecoin.dat. Checks
 * first that the data still matches the plan (pieces missing, same coins);
 * on any error neither buffer is changed.
 */
ShopResult shop_plan_apply(uint8_t *meet, size_t meet_len, uint8_t *coin, size_t coin_len,
                           const PurchasePlan *plan);

#endif
