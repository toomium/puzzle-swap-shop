#include <stdlib.h>
#include <string.h>

#include "test_prices.h"
#include "greatest.h"
#include "fake_storage.h"
#include "fixture.h"
#include "core/meet_save.h"
#include "core/shop.h"
#include "platform/fault_storage.h"
#include "platform/journal.h"
#include "platform/transaction.h"

static uint8_t *meet, *coin;
static size_t meet_len, coin_len;
static FakeStorage fake;
static PurchasePlan plan;

static void setup(void *arg)
{
    (void)arg;
    meet = fixture_load("meet_sample.dat", &meet_len);
    coin = fixture_load("gamecoin_120.dat", &coin_len);
    fake_storage_init(&fake, meet, meet_len, coin, coin_len);

    PanelState p;
    memset(&p, 0, sizeof(p));
    p.id = 9;
    p.available = true;
    p.piece_count = 40;
    meet_save_get_owned(meet, meet_len, 9, &p.owned);
    ShopPrices prices = test_prices();
    shop_plan_build(&p, (uint64_t)1 << 12, 120, &prices, &plan);
}

static void teardown(void *arg)
{
    (void)arg;
    fake_storage_free(&fake);
    fixture_free(meet);
    fixture_free(coin);
}

static TxnResult buy_with(FaultScenario scenario, FaultStorage *f, TxnReport *rep)
{
    TxnEnv env;
    memset(&env, 0, sizeof(env));
    fault_storage_init(f, &fake.storage, scenario);
    env.storage = &f->storage;
    env.title_id = 0x0004001000022800ull;
    env.now = (Timestamp){2026, 10, 26, 20, 0, 0};
    env.app_version = "0.1.0";
    return transaction_purchase(&env, &plan, rep);
}

static bool game_unchanged(void)
{
    size_t ml, cl;
    const uint8_t *m = fake_game(&fake, GAME_MEET, &ml);
    const uint8_t *c = fake_game(&fake, GAME_COIN, &cl);
    return ml == meet_len && cl == coin_len && memcmp(m, meet, ml) == 0 && memcmp(c, coin, cl) == 0;
}

TEST every_scenario_ends_as_documented(void)
{
    static const struct {
        FaultScenario scenario;
        TxnResult result;
        bool journal_left;
    } cases[] = {
        {FAULT_BACKUP_WRITE, TXN_ERR_BACKUP, false}, {FAULT_JOURNAL_WRITE, TXN_ERR_JOURNAL, false},
        {FAULT_MEET_WRITE, TXN_ROLLED_BACK, false},  {FAULT_MEET_VERIFY, TXN_ROLLED_BACK, false},
        {FAULT_COIN_WRITE, TXN_ROLLED_BACK, false},  {FAULT_COIN_VERIFY, TXN_ROLLED_BACK, false},
        {FAULT_ROLLBACK, TXN_ERR_ROLLBACK, true},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        FaultStorage f;
        TxnReport rep;
        fake_storage_free(&fake);
        fake_storage_init(&fake, meet, meet_len, coin, coin_len);

        ASSERT_EQ_FMT(cases[i].result, buy_with(cases[i].scenario, &f, &rep), "%d");
        ASSERT(f.fired > 0);
        ASSERT(game_unchanged());
        ASSERT_EQ(cases[i].journal_left, fake_find(&fake, JOURNAL_FILE) != NULL);

        /* A kept journal is only a leftover, since nothing was changed. */
        TxnEnv env;
        RecoveryInfo info;
        memset(&env, 0, sizeof(env));
        env.storage = &fake.storage;
        RecoveryStatus expected = cases[i].journal_left ? RECOVERY_CLEANED : RECOVERY_NONE;
        ASSERT_EQ(expected, transaction_check_recovery(&env, &info));
    }
    PASS();
}

TEST no_fault_buys(void)
{
    FaultStorage f;
    TxnReport rep;
    ASSERT_EQ(TXN_OK, buy_with(FAULT_NONE, &f, &rep));
    ASSERT_EQ(0, f.fired);
    ASSERT_FALSE(game_unchanged());
    ASSERT_STR_EQ("kein Fehler", fault_scenario_name(FAULT_NONE));
    ASSERT_STR_EQ("?", fault_scenario_name(FAULT_COUNT));
    PASS();
}

SUITE(fault_storage_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(every_scenario_ends_as_documented);
    RUN_TEST(no_fault_buys);
}
