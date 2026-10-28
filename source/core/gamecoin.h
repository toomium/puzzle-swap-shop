#ifndef PSS_CORE_GAMECOIN_H
#define PSS_CORE_GAMECOIN_H

#include <stddef.h>
#include <stdint.h>

/* gamecoin.dat in shared extdata 0xF000000B. */

#define GAMECOIN_SIZE   0x14u
#define GAMECOIN_OFFSET 0x04u
#define GAMECOIN_MAX    300u

typedef enum {
    COIN_OK = 0,
    COIN_ERR_SIZE,  /* buffer is not GAMECOIN_SIZE bytes */
    COIN_ERR_RANGE, /* value above GAMECOIN_MAX, or not enough coins to spend */
} CoinResult;

CoinResult gamecoin_validate(const uint8_t *buf, size_t len);

CoinResult gamecoin_get(const uint8_t *buf, size_t len, uint16_t *coins);

/* Writes only the two coin bytes. */
CoinResult gamecoin_set(uint8_t *buf, size_t len, uint16_t coins);

/* Subtracts amount; fails without change if fewer coins are available. */
CoinResult gamecoin_spend(uint8_t *buf, size_t len, uint16_t amount);

#endif
