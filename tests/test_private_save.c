#include <stdio.h>

#include "greatest.h"
#include "fixture.h"
#include "core/gamecoin.h"
#include "core/meet_save.h"

/*
 * Optional check against a real save. Put a copy of your own meet.dat (and
 * gamecoin.dat) into tests/private/; that folder is git-ignored. Skipped when
 * no file is present, so CI never depends on personal data.
 */

TEST real_meet_validates(void)
{
    size_t len;
    uint8_t *buf = fixture_load_path("tests/private/meet.dat", &len);
    if (!buf)
        SKIPm("no tests/private/meet.dat");
    MeetResult res = meet_save_validate(buf, len);
    fixture_free(buf);
    ASSERT_EQ_FMT((int)MEET_OK, (int)res, "%d");
    PASS();
}

TEST real_gamecoin_validates(void)
{
    size_t len;
    uint8_t *buf = fixture_load_path("tests/private/gamecoin.dat", &len);
    if (!buf)
        SKIPm("no tests/private/gamecoin.dat");
    CoinResult res = gamecoin_validate(buf, len);
    fixture_free(buf);
    ASSERT_EQ_FMT((int)COIN_OK, (int)res, "%d");
    PASS();
}

SUITE(private_save_suite)
{
    RUN_TEST(real_meet_validates);
    RUN_TEST(real_gamecoin_validates);
}
