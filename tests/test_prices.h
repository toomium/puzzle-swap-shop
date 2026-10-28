#ifndef PSS_TESTS_TEST_PRICES_H
#define PSS_TESTS_TEST_PRICES_H

#include "core/shop.h"

/* The tests count with fixed prices, so that changing the default does not touch them. */
static inline ShopPrices test_prices(void)
{
    ShopPrices prices = {2, 10};
    return prices;
}

#endif
