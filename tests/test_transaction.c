#include <stdlib.h>
#include <string.h>

#include "test_prices.h"
#include "greatest.h"
#include "fake_storage.h"
#include "fixture.h"
#include "core/gamecoin.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"
#include "core/shop.h"
#include "platform/backup.h"
#include "platform/journal.h"
#include "platform/transaction.h"

#define BIT(i) ((uint64_t)1 << (i))

static uint8_t *meet0, *coin0; /* game data before */
static uint8_t *meet1, *coin1; /* expected after the purchase */
static size_t meet_len, coin_len;
static FakeStorage fake;
static TxnEnv env;
static PurchasePlan plan;

static void reset_fake(void)
{
    fake_storage_free(&fake);
    fake_storage_init(&fake, meet0, meet_len, coin0, coin_len);
    env.storage = &fake.storage;
}

static void setup(void *arg)
{
    (void)arg;
    meet0 = fixture_load("meet_sample.dat", &meet_len);
    coin0 = fixture_load("gamecoin_120.dat", &coin_len);
    meet1 = malloc(meet_len);
    coin1 = malloc(coin_len);
    memcpy(meet1, meet0, meet_len);
    memcpy(coin1, coin0, coin_len);

    PanelState p;
    memset(&p, 0, sizeof(p));
    p.id = 9;
    p.available = true;
    p.piece_count = 40;
    meet_save_get_owned(meet0, meet_len, 9, &p.owned);
    ShopPrices prices = test_prices();
    shop_plan_build(&p, BIT(12) | BIT(14) | BIT(22), 120, &prices, &plan);
    shop_plan_apply(meet1, meet_len, coin1, coin_len, &plan);

    memset(&env, 0, sizeof(env));
    env.title_id = 0x0004001000022800ull;
    env.now = (Timestamp){2026, 10, 26, 19, 30, 0};
    env.app_version = "0.1.0";
    memset(&fake, 0, sizeof(fake));
    reset_fake();
}

static void teardown(void *arg)
{
    (void)arg;
    fake_storage_free(&fake);
    fixture_free(meet0);
    fixture_free(coin0);
    free(meet1);
    free(coin1);
}

static bool game_is(const uint8_t *meet, const uint8_t *coin)
{
    size_t ml, cl;
    const uint8_t *m = fake_game(&fake, GAME_MEET, &ml);
    const uint8_t *c = fake_game(&fake, GAME_COIN, &cl);
    return ml == meet_len && cl == coin_len && memcmp(m, meet, ml) == 0 && memcmp(c, coin, cl) == 0;
}

static bool journal_exists(void)
{
    return fake_find(&fake, JOURNAL_FILE) != NULL;
}

TEST purchase_happy_path(void)
{
    TxnReport rep;
    ASSERT(meet0 != NULL && coin0 != NULL);
    ASSERT_EQ(TXN_OK, transaction_purchase(&env, &plan, &rep));
    ASSERT(game_is(meet1, coin1));
    ASSERT(rep.meet_written && rep.coin_written);
    ASSERT_STR_EQ("20261026-193000", rep.backup);
    ASSERT_FALSE(journal_exists());

    uint16_t coins;
    gamecoin_get(fake_game(&fake, GAME_COIN, NULL), coin_len, &coins);
    ASSERT_EQ(114, coins);

    /* The backup holds the state before the purchase. */
    uint8_t *m = malloc(MEET_SAVE_SIZE);
    uint8_t c[GAMECOIN_SIZE];
    size_t l1, l2;
    Manifest man;
    ASSERT_EQ(BACKUP_OK, backup_load(&fake.storage, rep.backup, &man, m, MEET_SAVE_SIZE, &l1, c,
                                     sizeof(c), &l2));
    ASSERT_EQ(0, memcmp(m, meet0, meet_len));
    ASSERT_EQ(0, memcmp(c, coin0, coin_len));
    ASSERT_EQ(BACKUP_REASON_PURCHASE, man.reason);
    ASSERT(man.has_coins && man.coins == 120);
    ASSERT(man.has_purchase && man.panel == 9 && man.pieces == 3);
    free(m);
    PASS();
}

TEST purchase_refuses_stale_plan_and_bad_data(void)
{
    TxnReport rep;
    PurchasePlan stale = plan;
    stale.coins_before = 100;
    ASSERT_EQ(TXN_ERR_REJECTED, transaction_purchase(&env, &stale, &rep));
    ASSERT_EQ(SHOP_ERR_STALE, rep.reject);
    ASSERT(game_is(meet0, coin0));
    ASSERT_EQ(0, backup_list(&fake.storage, NULL, 0));

    size_t bad_len;
    uint8_t *bad = fixture_load("meet_bad_unused.dat", &bad_len);
    ASSERT(bad != NULL);
    fake_storage_free(&fake);
    fake_storage_init(&fake, bad, bad_len, coin0, coin_len);
    ASSERT_EQ(TXN_ERR_INVALID, transaction_purchase(&env, &plan, &rep));
    ASSERT_EQ(TXN_STEP_CHECK, rep.step);
    ASSERT_EQ(0, backup_list(&fake.storage, NULL, 0));
    fixture_free(bad);
    PASS();
}

static int set_unused_bit(uint8_t *meet, size_t meet_len_, uint8_t *coin, size_t coin_len_,
                          void *arg)
{
    (void)meet_len_, (void)coin, (void)coin_len_, (void)arg;
    meet[MEET_STANDARD_PANELS + 1] |= 0x80; /* panel 0 has 15 pieces: bit 15 is unused */
    return 0;
}

static int no_change(uint8_t *meet, size_t ml, uint8_t *coin, size_t cl, void *arg)
{
    (void)meet, (void)ml, (void)coin, (void)cl, (void)arg;
    return 0;
}

TEST change_is_checked_before_writing(void)
{
    TxnReport rep;
    ASSERT_EQ(TXN_ERR_INVALID, transaction_change(&env, set_unused_bit, NULL, &rep));
    ASSERT(game_is(meet0, coin0));
    ASSERT_EQ(TXN_NOTHING_TO_DO, transaction_change(&env, no_change, NULL, &rep));
    ASSERT_EQ(0, backup_list(&fake.storage, NULL, 0));
    ASSERT_FALSE(journal_exists());
    PASS();
}

/* Number of storage calls in a purchase without faults. */
static unsigned calls_of_good_purchase(void)
{
    TxnReport rep;
    reset_fake();
    transaction_purchase(&env, &plan, &rep);
    unsigned n = fake.calls;
    reset_fake();
    return n;
}

TEST any_single_failure_leaves_consistent_data(void)
{
    unsigned total = calls_of_good_purchase();
    unsigned rolled_back = 0;
    ASSERT(total > 10);

    for (unsigned n = 1; n <= total; n++) {
        TxnReport rep;
        RecoveryInfo info;
        reset_fake();
        fake.fail_call = n;
        TxnResult r = transaction_purchase(&env, &plan, &rep);
        fake_storage_reboot(&fake);

        if (r == TXN_OK) {
            ASSERT(game_is(meet1, coin1));
        } else {
            ASSERT(r == TXN_ERR_READ || r == TXN_ERR_BACKUP || r == TXN_ERR_JOURNAL ||
                   r == TXN_ROLLED_BACK);
            ASSERT(game_is(meet0, coin0));
            ASSERT_FALSE(journal_exists());
            rolled_back += r == TXN_ROLLED_BACK;
        }
        RecoveryStatus rs = transaction_check_recovery(&env, &info);
        ASSERT(rs == RECOVERY_NONE || rs == RECOVERY_CLEANED);
        ASSERT_FALSE(journal_exists());
    }
    /* Write and read-back failures of both files were among them. */
    ASSERT(rolled_back >= 4);
    PASS();
}

TEST power_loss_at_any_point_is_recoverable(void)
{
    unsigned total = calls_of_good_purchase();
    unsigned interrupted = 0, rollback_errors = 0;

    for (unsigned n = 1; n <= total + 1; n++) {
        TxnReport rep;
        RecoveryInfo info;
        reset_fake();
        fake.power_off_call = n;
        TxnResult r = transaction_purchase(&env, &plan, &rep);
        rollback_errors += r == TXN_ERR_ROLLBACK;

        /* Start the app again. */
        fake_storage_reboot(&fake);
        RecoveryStatus rs = transaction_check_recovery(&env, &info);
        switch (rs) {
        case RECOVERY_NONE:
            ASSERT(game_is(meet0, coin0) || (r == TXN_OK && game_is(meet1, coin1)));
            break;
        case RECOVERY_CLEANED:
            ASSERT(game_is(meet0, coin0) || game_is(meet1, coin1));
            break;
        case RECOVERY_INTERRUPTED: {
            interrupted++;
            /* Coins are never gone without the pieces. */
            ASSERT_FALSE(info.coin_is_after && !info.meet_is_after);
            TxnReport rrep;
            env.now.second = 30;
            ASSERT_EQ(TXN_OK, transaction_restore(&env, info.rec.backup, &rrep));
            env.now.second = 0;
            ASSERT(game_is(meet0, coin0));
            ASSERT_EQ(RECOVERY_NONE, transaction_check_recovery(&env, &info));
            break;
        }
        default:
            FAILm("recovery must not need the user to pick a backup");
        }
        ASSERT_FALSE(journal_exists());
    }
    ASSERT(interrupted >= 2);
    ASSERT(rollback_errors >= 2);
    PASS();
}

TEST stop_after_meet_needs_recovery(void)
{
    TxnReport rep;
    RecoveryInfo info;
    env.stop_after = TXN_STEP_MEET;
    ASSERT_EQ(TXN_STOPPED, transaction_purchase(&env, &plan, &rep));
    env.stop_after = TXN_STEP_NONE;
    ASSERT(journal_exists());

    ASSERT_EQ(RECOVERY_INTERRUPTED, transaction_check_recovery(&env, &info));
    ASSERT(info.meet_is_after && info.coin_is_before);
    ASSERT_EQ(JOURNAL_OP_PURCHASE, info.rec.op);
    ASSERT_STR_EQ(rep.backup, info.rec.backup);

    env.now.second = 1;
    ASSERT_EQ(TXN_OK, transaction_restore(&env, info.rec.backup, &rep));
    ASSERT(game_is(meet0, coin0));
    ASSERT_FALSE(journal_exists());
    PASS();
}

TEST stop_after_coin_is_complete(void)
{
    TxnReport rep;
    RecoveryInfo info;
    env.stop_after = TXN_STEP_COIN;
    ASSERT_EQ(TXN_STOPPED, transaction_purchase(&env, &plan, &rep));
    env.stop_after = TXN_STEP_NONE;
    ASSERT_EQ(RECOVERY_CLEANED, transaction_check_recovery(&env, &info));
    ASSERT(game_is(meet1, coin1));
    ASSERT_FALSE(journal_exists());
    PASS();
}

TEST restore_backs_up_current_state(void)
{
    TxnReport buy, rest;
    BackupInfo list[4];
    ASSERT_EQ(TXN_OK, transaction_purchase(&env, &plan, &buy));
    env.now.minute = 31;
    ASSERT_EQ(TXN_OK, transaction_restore(&env, buy.backup, &rest));
    ASSERT(game_is(meet0, coin0));
    ASSERT_EQ(2, backup_list(&fake.storage, list, 4));
    ASSERT_STR_EQ("20261026-193100", list[0].name);
    ASSERT_EQ(BACKUP_REASON_RESTORE, list[0].manifest.reason);
    ASSERT_STR_EQ(buy.backup, list[1].name);
    PASS();
}

TEST restore_repairs_damaged_save(void)
{
    TxnReport rep;
    Manifest m;
    memset(&m, 0, sizeof(m));
    strcpy(m.app_version, "0.1.0");
    m.created = env.now;
    m.title_id = env.title_id;
    ASSERT_EQ(BACKUP_OK, backup_create(&fake.storage, BACKUP_ORIGINAL, &m, meet0, meet_len, coin0,
                                       coin_len, NULL));

    size_t bad_len;
    uint8_t *bad = fixture_load("meet_bad_unused.dat", &bad_len);
    memcpy(fake.game[GAME_MEET], bad, bad_len);
    fixture_free(bad);

    ASSERT_EQ(TXN_OK, transaction_restore(&env, BACKUP_ORIGINAL, &rep));
    ASSERT(game_is(meet0, coin0));
    PASS();
}

TEST restore_refuses_damaged_or_unknown_backup(void)
{
    TxnReport rep;
    Manifest m;
    memset(&m, 0, sizeof(m));
    strcpy(m.app_version, "0.1.0");
    m.created = env.now;
    ASSERT_EQ(BACKUP_OK, backup_create(&fake.storage, "20261026-120000", &m, meet1, meet_len, coin1,
                                       coin_len, NULL));
    fake_find(&fake, "backups/20261026-120000/meet.dat")->data[100] ^= 0xFF;

    ASSERT_EQ(TXN_ERR_REJECTED, transaction_restore(&env, "20261026-120000", &rep));
    ASSERT_EQ(BACKUP_ERR_DAMAGED, rep.reject);
    ASSERT_EQ(TXN_ERR_REJECTED, transaction_restore(&env, "20261026-120001", &rep));
    ASSERT_EQ(BACKUP_ERR_NOT_FOUND, rep.reject);
    ASSERT_EQ(TXN_ERR_REJECTED, transaction_restore(&env, "../../x", &rep));
    ASSERT_EQ(BACKUP_ERR_NAME, rep.reject);
    ASSERT(game_is(meet0, coin0));
    PASS();
}

TEST purchase_prunes_old_backups(void)
{
    TxnReport rep;
    BackupInfo list[8];
    Manifest m;
    char name[BACKUP_NAME_MAX];
    memset(&m, 0, sizeof(m));
    strcpy(m.app_version, "0.1.0");
    m.created = env.now;
    ASSERT_EQ(BACKUP_OK, backup_create(&fake.storage, BACKUP_ORIGINAL, &m, meet0, meet_len, coin0,
                                       coin_len, NULL));
    for (unsigned d = 1; d <= 5; d++) {
        snprintf(name, sizeof(name), "202610%02u-120000", d);
        ASSERT_EQ(BACKUP_OK,
                  backup_create(&fake.storage, name, &m, meet0, meet_len, coin0, coin_len, NULL));
    }
    env.keep_backups = 3;
    ASSERT_EQ(TXN_OK, transaction_purchase(&env, &plan, &rep));
    ASSERT_EQ(4, backup_list(&fake.storage, list, 8));
    ASSERT_STR_EQ(rep.backup, list[0].name);
    ASSERT_STR_EQ("20261005-120000", list[1].name);
    ASSERT_STR_EQ("20261004-120000", list[2].name);
    ASSERT_STR_EQ(BACKUP_ORIGINAL, list[3].name);
    PASS();
}

TEST damaged_journal_is_judged_by_newest_backup(void)
{
    TxnReport rep;
    RecoveryInfo info;
    ASSERT_EQ(TXN_OK, transaction_purchase(&env, &plan, &rep));

    /* Leftover torn journal, game data equals no backup: the user must choose. */
    ASSERT(fake_put(&fake, JOURNAL_FILE, "format=1\nop=pur", 15));
    ASSERT_EQ(RECOVERY_DAMAGED, transaction_check_recovery(&env, &info));

    /* Game data equals the newest backup: nothing happened. */
    memcpy(fake.game[GAME_MEET], meet0, meet_len);
    memcpy(fake.game[GAME_COIN], coin0, coin_len);
    ASSERT_EQ(RECOVERY_CLEANED, transaction_check_recovery(&env, &info));
    ASSERT_FALSE(journal_exists());
    PASS();
}

SUITE(transaction_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(purchase_happy_path);
    RUN_TEST(purchase_refuses_stale_plan_and_bad_data);
    RUN_TEST(change_is_checked_before_writing);
    RUN_TEST(any_single_failure_leaves_consistent_data);
    RUN_TEST(power_loss_at_any_point_is_recoverable);
    RUN_TEST(stop_after_meet_needs_recovery);
    RUN_TEST(stop_after_coin_is_complete);
    RUN_TEST(restore_backs_up_current_state);
    RUN_TEST(restore_repairs_damaged_save);
    RUN_TEST(restore_refuses_damaged_or_unknown_backup);
    RUN_TEST(purchase_prunes_old_backups);
    RUN_TEST(damaged_journal_is_judged_by_newest_backup);
}
