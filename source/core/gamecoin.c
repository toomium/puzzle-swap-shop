#include "core/gamecoin.h"

static uint16_t read_u16le(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

CoinResult gamecoin_get(const uint8_t *buf, size_t len, uint16_t *coins)
{
    if (!buf || len != GAMECOIN_SIZE)
        return COIN_ERR_SIZE;
    *coins = read_u16le(buf + GAMECOIN_OFFSET);
    return COIN_OK;
}

CoinResult gamecoin_validate(const uint8_t *buf, size_t len)
{
    uint16_t coins;
    CoinResult res = gamecoin_get(buf, len, &coins);
    if (res != COIN_OK)
        return res;
    return coins <= GAMECOIN_MAX ? COIN_OK : COIN_ERR_RANGE;
}

CoinResult gamecoin_set(uint8_t *buf, size_t len, uint16_t coins)
{
    if (!buf || len != GAMECOIN_SIZE)
        return COIN_ERR_SIZE;
    if (coins > GAMECOIN_MAX)
        return COIN_ERR_RANGE;
    buf[GAMECOIN_OFFSET] = (uint8_t)(coins & 0xFF);
    buf[GAMECOIN_OFFSET + 1] = (uint8_t)(coins >> 8);
    return COIN_OK;
}

CoinResult gamecoin_spend(uint8_t *buf, size_t len, uint16_t amount)
{
    uint16_t coins;
    CoinResult res = gamecoin_get(buf, len, &coins);
    if (res != COIN_OK)
        return res;
    if (amount > coins)
        return COIN_ERR_RANGE;
    return gamecoin_set(buf, len, (uint16_t)(coins - amount));
}
