#include <string.h>

#include "greatest.h"
#include "fixture.h"
#include "core/gamecoin.h"

TEST reads_fixture_values(void)
{
    size_t len;
    uint16_t coins;
    uint8_t *buf = fixture_load("gamecoin_120.dat", &len);
    ASSERT(buf != NULL);
    ASSERT_EQ(COIN_OK, gamecoin_validate(buf, len));
    ASSERT_EQ(COIN_OK, gamecoin_get(buf, len, &coins));
    ASSERT_EQ(120, coins);
    fixture_free(buf);

    buf = fixture_load("gamecoin_300.dat", &len);
    ASSERT(buf != NULL);
    ASSERT_EQ(COIN_OK, gamecoin_validate(buf, len));
    fixture_free(buf);

    buf = fixture_load("gamecoin_bad_301.dat", &len);
    ASSERT(buf != NULL);
    ASSERT_EQ(COIN_ERR_RANGE, gamecoin_validate(buf, len));
    fixture_free(buf);
    PASS();
}

TEST spend_changes_only_coin_bytes(void)
{
    size_t len;
    uint8_t before[GAMECOIN_SIZE];
    uint8_t *buf = fixture_load("gamecoin_120.dat", &len);
    ASSERT(buf != NULL);
    memcpy(before, buf, sizeof(before));

    ASSERT_EQ(COIN_OK, gamecoin_spend(buf, len, 6));
    uint16_t coins;
    gamecoin_get(buf, len, &coins);
    ASSERT_EQ(114, coins);
    for (size_t i = 0; i < len; i++) {
        if (i == GAMECOIN_OFFSET || i == GAMECOIN_OFFSET + 1)
            continue;
        ASSERT_EQ(before[i], buf[i]);
    }
    fixture_free(buf);
    PASS();
}

TEST limits(void)
{
    uint8_t buf[GAMECOIN_SIZE] = {0};
    uint16_t coins;
    ASSERT_EQ(COIN_OK, gamecoin_set(buf, sizeof(buf), 300));
    ASSERT_EQ(COIN_ERR_RANGE, gamecoin_set(buf, sizeof(buf), 301));
    gamecoin_get(buf, sizeof(buf), &coins);
    ASSERT_EQ(300, coins);
    ASSERT_EQ(0x2C, buf[4]);
    ASSERT_EQ(0x01, buf[5]);

    ASSERT_EQ(COIN_OK, gamecoin_spend(buf, sizeof(buf), 300));
    gamecoin_get(buf, sizeof(buf), &coins);
    ASSERT_EQ(0, coins);
    ASSERT_EQ(COIN_ERR_RANGE, gamecoin_spend(buf, sizeof(buf), 1));
    gamecoin_get(buf, sizeof(buf), &coins);
    ASSERT_EQ(0, coins);
    PASS();
}

TEST wrong_size(void)
{
    uint8_t buf[GAMECOIN_SIZE + 1] = {0};
    uint16_t coins;
    ASSERT_EQ(COIN_ERR_SIZE, gamecoin_get(buf, sizeof(buf), &coins));
    ASSERT_EQ(COIN_ERR_SIZE, gamecoin_set(buf, 4, 1));
    ASSERT_EQ(COIN_ERR_SIZE, gamecoin_validate(NULL, GAMECOIN_SIZE));
    PASS();
}

SUITE(gamecoin_suite)
{
    RUN_TEST(reads_fixture_values);
    RUN_TEST(spend_changes_only_coin_bytes);
    RUN_TEST(limits);
    RUN_TEST(wrong_size);
}
