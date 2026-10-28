#include <stdlib.h>
#include <string.h>

#include "greatest.h"
#include "fake_storage.h"
#include "fixture.h"
#include "app/model.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"
#include "platform/fault_storage.h"
#include "platform/journal.h"

#define BIT(i) ((uint64_t)1 << (i))

static uint8_t *meet, *coin;
static size_t meet_len, coin_len;
static FakeStorage fake;
static ModelEnv env;
static Model model;

static bool fixed_now(Timestamp *out)
{
    *out = (Timestamp){2026, 10, 26, 21, 0, 0};
    return true;
}

static void setup(void *arg)
{
    (void)arg;
    meet = fixture_load("meet_sample.dat", &meet_len);
    coin = fixture_load("gamecoin_120.dat", &coin_len);
    fake_storage_init(&fake, meet, meet_len, coin, coin_len);
    env = (ModelEnv){&fake.storage, mii_plaza_title(PLAZA_EUR), fixed_now, "0.1.0", 3};
    memset(&model, 0, sizeof(model));
}

static void teardown(void *arg)
{
    (void)arg;
    model_free(&model);
    fake_storage_free(&fake);
    fixture_free(meet);
    fixture_free(coin);
}

/* Loads, creating the original backup if needed, and expects MODEL_READY. */
static enum greatest_test_res load_ready(void)
{
    if (model_load(&model, &env) == MODEL_NEED_ORIGINAL)
        ASSERT_EQ(BACKUP_OK, model_create_original(&model));
    ASSERT_EQ(MODEL_READY, model.status);
    PASS();
}

TEST first_start_needs_original(void)
{
    ASSERT_EQ(MODEL_NEED_ORIGINAL, model_load(&model, &env));
    ASSERT(model.has_data);
    ASSERT_EQ(120, model.coins);

    /* No purchase before the original backup exists. */
    PurchasePlan plan;
    ASSERT_EQ(SHOP_OK, model_plan(&model, 9, BIT(12), &plan));
    ASSERT_EQ(TXN_ERR_REJECTED, model_buy(&model, &plan));
    ASSERT_EQ(NULL, fake_find(&fake, "backups/20261026-210000"));

    ASSERT_EQ(BACKUP_OK, model_create_original(&model));
    ASSERT_EQ(MODEL_READY, model.status);
    BackupInfo list[2];
    ASSERT_EQ(1, model_backups(&model, list, 2));
    ASSERT_STR_EQ(BACKUP_ORIGINAL, list[0].name);
    ASSERT(list[0].manifest.has_coins && list[0].manifest.coins == 120);

    /* The next start goes straight to the gallery. */
    model_free(&model);
    ASSERT_EQ(MODEL_READY, model_load(&model, &env));
    PASS();
}

TEST panels_and_gallery_order(void)
{
    CHECK_CALL(load_ready());
    const PanelState *p = model_panel(&model, 9);
    ASSERT_EQ(40, p->piece_count);
    ASSERT_EQ(28, panel_bits_count(p->owned));
    ASSERT(p->available);
    ASSERT_STR_EQ("Super Mario 3D Land", p->name);
    ASSERT_FALSE(model_panel(&model, 8)->available); /* SpotPass without pieces */
    ASSERT(model_panel(&model, 1)->available);       /* standard, even when empty */
    ASSERT_EQ(NULL, model_panel(&model, PANEL_COUNT));

    /* Incomplete 0,1,2,4,5,6,9; complete 3,7; then the absent SpotPass panels. */
    static const uint8_t head[] = {0, 1, 2, 4, 5, 6, 9, 3, 7, 8, 10, 11};
    for (size_t i = 0; i < sizeof(head); i++)
        ASSERT_EQ(head[i], model.gallery[i]);
    ASSERT_EQ(62, model.gallery[PANEL_COUNT - 1]);

    ASSERT_EQ(BIT(14), model_buyable(&model, 0));
    ASSERT_EQ(0, model_buyable(&model, 3));
    PASS();
}

TEST panel_presence_rule(void)
{
    ASSERT(model_panel_present(0, 0));       /* standard, even when empty */
    ASSERT(model_panel_present(9, BIT(3)));  /* SpotPass with a piece */
    ASSERT_FALSE(model_panel_present(9, 0)); /* SpotPass without any */
    ASSERT_FALSE(model_panel_present(PANEL_COUNT, BIT(0)));
    PASS();
}

TEST first_start_writes_default_settings(void)
{
    uint8_t buf[SETTINGS_TEXT_MAX];
    size_t len = 0;
    const Storage *st = &fake.storage;
    ASSERT_EQ(STORE_ERR_NOT_FOUND, st->sd_read(st->ctx, "settings.ini", buf, sizeof(buf), &len));
    CHECK_CALL(load_ready());
    ASSERT_EQ(STORE_OK, st->sd_read(st->ctx, "settings.ini", buf, sizeof(buf), &len));
    Settings s;
    unsigned bad = 0;
    ASSERT(settings_parse((const char *)buf, len, &s, &bad));
    ASSERT_EQ(5, s.prices.per_piece);
    ASSERT_EQ(10, s.prices.per_rare_piece);
    ASSERT_EQ(0u, bad);
    PASS();
}

TEST buy_updates_everything(void)
{
    PurchasePlan plan;
    CHECK_CALL(load_ready());
    ASSERT_EQ(SHOP_OK, model_plan(&model, 0, BIT(14), &plan));
    ASSERT_EQ(5, plan.cost);
    ASSERT_EQ(TXN_OK, model_buy(&model, &plan));
    ASSERT_STR_EQ("20261026-210000", model.last.backup);
    ASSERT_EQ(115, model.coins);
    ASSERT_EQ(panel_bits_all(15), model_panel(&model, 0)->owned);

    /* Panel 0 is complete now and moves behind the incomplete ones. */
    ASSERT_EQ(1, model.gallery[0]);

    /* A plan built before the purchase no longer fits the data. */
    ASSERT_EQ(TXN_ERR_REJECTED, model_buy(&model, &plan));
    ASSERT_EQ(SHOP_ERR_STALE, model.last.reject);
    PASS();
}

TEST prices_come_from_settings(void)
{
    const char *ini = "price_per_piece=3\nprice_rare_piece=500\n";
    ASSERT(fake_put(&fake, SETTINGS_FILE, ini, strlen(ini)));
    CHECK_CALL(load_ready());
    ASSERT_EQ(3, model.settings.prices.per_piece);
    ASSERT_EQ(10, model.settings.prices.per_rare_piece);
    ASSERT_EQ(1, model.settings_bad);

    PurchasePlan plan;
    ASSERT_EQ(SHOP_OK, model_plan(&model, 9, BIT(12) | BIT(14), &plan));
    ASSERT_EQ(6, plan.cost);

    Settings s = model.settings;
    s.sound = false;
    ASSERT(model_save_settings(&model, &s));
    model_free(&model);
    ASSERT_EQ(MODEL_READY, model_load(&model, &env));
    ASSERT_FALSE(model.settings.sound);
    ASSERT_EQ(3, model.settings.prices.per_piece);
    ASSERT_EQ(0, model.settings_bad);
    PASS();
}

TEST unknown_save_is_read_only(void)
{
    size_t bad_len;
    uint8_t *bad = fixture_load("meet_bad_unused.dat", &bad_len);
    ASSERT(bad != NULL);
    fake_storage_free(&fake);
    fake_storage_init(&fake, bad, bad_len, coin, coin_len);
    fixture_free(bad);

    ASSERT_EQ(MODEL_READ_ONLY, model_load(&model, &env));
    ASSERT(model.has_data);
    /* Viewing still works; the original backup may be made, buying not. */
    ASSERT_EQ(14, panel_bits_count(model_panel(&model, 0)->owned));
    PurchasePlan plan = {0};
    ASSERT_EQ(SHOP_ERR_DATA, model_plan(&model, 9, BIT(12), &plan));
    ASSERT_EQ(TXN_ERR_REJECTED, model_buy(&model, &plan));
    ASSERT_EQ(BACKUP_OK, model_create_original(&model));
    ASSERT_EQ(MODEL_READ_ONLY, model.status);
    PASS();
}

TEST unreadable_save_is_an_error(void)
{
    fake.fail_op = FAKE_GAME_READ;
    fake.fail_op_nth = 1;
    ASSERT_EQ(MODEL_ERR_READ, model_load(&model, &env));
    ASSERT_EQ(TXN_ERR_REJECTED, model_restore(&model, BACKUP_ORIGINAL));
    PASS();
}

/* Leaves an interrupted purchase behind: pieces written, coins not. */
static enum greatest_test_res interrupt_purchase(void)
{
    PurchasePlan plan;
    TxnEnv tenv;
    TxnReport rep;
    CHECK_CALL(load_ready());
    ASSERT_EQ(SHOP_OK, model_plan(&model, 9, BIT(12), &plan));
    memset(&tenv, 0, sizeof(tenv));
    tenv.storage = &fake.storage;
    tenv.title_id = env.title->title_id;
    tenv.now = (Timestamp){2026, 10, 26, 21, 5, 0};
    tenv.app_version = "0.1.0";
    tenv.stop_after = TXN_STEP_MEET;
    ASSERT_EQ(TXN_STOPPED, transaction_purchase(&tenv, &plan, &rep));
    model_free(&model);
    ASSERT_EQ(MODEL_INTERRUPTED, model_load(&model, &env));
    ASSERT_EQ(RECOVERY_INTERRUPTED, model.recovery);
    ASSERT_STR_EQ("20261026-210500", model.recovery_info.rec.backup);
    PASS();
}

TEST interrupted_purchase_restored(void)
{
    CHECK_CALL(interrupt_purchase());
    PurchasePlan plan = {0};
    ASSERT_EQ(TXN_ERR_REJECTED, model_buy(&model, &plan));
    ASSERT_EQ(TXN_OK, model_resolve_interrupted(&model, true));
    ASSERT_EQ(MODEL_READY, model.status);
    ASSERT_EQ(0, memcmp(model.meet, meet, meet_len));
    ASSERT_EQ(120, model.coins);
    ASSERT_EQ(NULL, fake_find(&fake, JOURNAL_FILE));
    PASS();
}

TEST interrupted_purchase_kept(void)
{
    CHECK_CALL(interrupt_purchase());
    ASSERT_EQ(TXN_OK, model_resolve_interrupted(&model, false));
    ASSERT_EQ(MODEL_READY, model.status);
    ASSERT(model_panel(&model, 9)->owned & BIT(12));
    ASSERT_EQ(120, model.coins);
    ASSERT_EQ(NULL, fake_find(&fake, JOURNAL_FILE));
    ASSERT_EQ(TXN_ERR_REJECTED, model_resolve_interrupted(&model, false));
    PASS();
}

TEST failed_rollback_blocks_buying(void)
{
    PurchasePlan plan;
    FaultStorage f;
    CHECK_CALL(load_ready());
    ASSERT_EQ(SHOP_OK, model_plan(&model, 9, BIT(12), &plan));

    /* Nothing written at all: the leftover journal is cleared right away. */
    fault_storage_init(&f, &fake.storage, FAULT_ROLLBACK);
    model.env.storage = &f.storage;
    ASSERT_EQ(TXN_ERR_ROLLBACK, model_buy(&model, &plan));
    ASSERT_EQ(MODEL_READY, model.status);
    ASSERT_EQ(NULL, fake_find(&fake, JOURNAL_FILE));

    /* Pieces written, coins failed, pieces cannot go back: mixed data. */
    fault_storage_init(&f, &fake.storage, FAULT_COIN_WRITE);
    fake.fail_op = FAKE_GAME_WRITE;
    fake.fail_op_nth = fake.op_calls[FAKE_GAME_WRITE] + 3;
    ASSERT_EQ(TXN_ERR_ROLLBACK, model_buy(&model, &plan));
    ASSERT_EQ(MODEL_INTERRUPTED, model.status);
    ASSERT_EQ(RECOVERY_INTERRUPTED, model.recovery);
    ASSERT_EQ(TXN_ERR_REJECTED, model_buy(&model, &plan));

    fake_storage_reboot(&fake);
    model.env.storage = &fake.storage;
    ASSERT_EQ(TXN_OK, model_resolve_interrupted(&model, true));
    ASSERT_EQ(MODEL_READY, model.status);
    ASSERT_EQ(0, memcmp(model.meet, meet, meet_len));
    PASS();
}

SUITE(model_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(first_start_needs_original);
    RUN_TEST(panels_and_gallery_order);
    RUN_TEST(panel_presence_rule);
    RUN_TEST(first_start_writes_default_settings);
    RUN_TEST(buy_updates_everything);
    RUN_TEST(prices_come_from_settings);
    RUN_TEST(unknown_save_is_read_only);
    RUN_TEST(unreadable_save_is_an_error);
    RUN_TEST(interrupted_purchase_restored);
    RUN_TEST(interrupted_purchase_kept);
    RUN_TEST(failed_rollback_blocks_buying);
}
