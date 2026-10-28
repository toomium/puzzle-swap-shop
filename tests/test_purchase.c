#include <string.h>

#include "greatest.h"
#include "app/route.h"
#include "core/backup_names.h"
#include "core/confetti.h"
#include "core/purchase_flow.h"

static void run_frames(PurchaseFlow *f, unsigned n)
{
    for (unsigned i = 0; i < n; i++)
        purchase_flow_step(f);
}

TEST flow_runs_the_purchase_after_the_delay(void)
{
    PurchaseFlow f;
    purchase_flow_init(&f);
    ASSERT_FALSE(purchase_flow_work_due(&f));
    ASSERT(purchase_flow_start(&f));
    ASSERT_FALSE(purchase_flow_start(&f));
    run_frames(&f, PF_WORK_DELAY - 1);
    ASSERT_FALSE(purchase_flow_work_due(&f));
    run_frames(&f, 1);
    ASSERT(purchase_flow_work_due(&f));
    purchase_flow_ran(&f);
    ASSERT_FALSE(purchase_flow_work_due(&f)); /* once only */
    PASS();
}

TEST flow_shows_the_bar_for_a_minimum_time(void)
{
    PurchaseFlow f;
    purchase_flow_init(&f);
    purchase_flow_start(&f);
    run_frames(&f, PF_WORK_DELAY);
    purchase_flow_ran(&f);
    ASSERT_FALSE(purchase_flow_done(&f));
    run_frames(&f, PF_MIN_FRAMES - PF_WORK_DELAY);
    ASSERT(purchase_flow_done(&f));
    ASSERT(purchase_flow_progress(&f) >= 1.0f);
    PASS();
}

TEST flow_waits_for_the_purchase_even_after_the_minimum(void)
{
    PurchaseFlow f;
    purchase_flow_init(&f);
    purchase_flow_start(&f);
    run_frames(&f, PF_MIN_FRAMES + 10);
    ASSERT_FALSE(purchase_flow_done(&f));
    purchase_flow_ran(&f);
    ASSERT(purchase_flow_done(&f));
    PASS();
}

TEST flow_progress_grows(void)
{
    PurchaseFlow f;
    purchase_flow_init(&f);
    ASSERT_EQ(0.0f, purchase_flow_progress(&f));
    purchase_flow_start(&f);
    float last = 0.0f;
    for (unsigned i = 0; i < PF_MIN_FRAMES; i++) {
        float p = purchase_flow_progress(&f);
        ASSERT(p >= last && p <= 1.0f);
        last = p;
        purchase_flow_step(&f);
    }
    PASS();
}

TEST outcome_follows_the_transaction_result(void)
{
    ASSERT_EQ(PURCHASE_SUCCESS, route_after_purchase(TXN_OK));
    ASSERT_EQ(PURCHASE_UNDO_FAILED, route_after_purchase(TXN_ERR_ROLLBACK));
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ERR_INVALID)); /* nothing was written */
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ROLLED_BACK));
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ERR_BACKUP));
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ERR_JOURNAL));
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ERR_REJECTED));
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ERR_READ));
    ASSERT_EQ(PURCHASE_FAILED, route_after_purchase(TXN_ERR_MEMORY));
    PASS();
}

TEST backup_label_in_both_styles(void)
{
    char out[24];
    ASSERT(backup_name_label("20261026-180400", true, out, sizeof(out)));
    ASSERT_STR_EQ("26.10.2026, 18:04", out);
    ASSERT(backup_name_label("20261026-180400-2", false, out, sizeof(out)));
    ASSERT_STR_EQ("2026-10-26 18:04", out);
    PASS();
}

TEST backup_label_refuses_original_and_garbage(void)
{
    char out[24];
    ASSERT_FALSE(backup_name_label(BACKUP_ORIGINAL, true, out, sizeof(out)));
    ASSERT_FALSE(backup_name_label("../etc", true, out, sizeof(out)));
    ASSERT_FALSE(backup_name_label("20261326-180400", true, out, sizeof(out)));
    ASSERT_FALSE(backup_name_label("20261026-180400", true, out, 8)); /* too small */
    PASS();
}

TEST confetti_stays_on_the_screen_vertically(void)
{
    ConfettiBit b;
    for (unsigned f = 0; f < confetti_frames(240) + 5; f++) {
        for (unsigned i = 0; i < CONFETTI_COUNT; i++) {
            if (confetti_at(f, i, 400, 240, &b)) {
                ASSERT(b.y < 240 && b.y >= -5);
                ASSERT(b.size >= 3 && b.size <= 5);
                ASSERT(b.color >= 0 && b.color < CONFETTI_COLORS);
                ASSERT(b.x >= -6 && b.x < 400 + 6);
            }
        }
    }
    PASS();
}

TEST confetti_is_over_after_its_frames(void)
{
    ConfettiBit b;
    for (unsigned i = 0; i < CONFETTI_COUNT; i++)
        ASSERT_FALSE(confetti_at(confetti_frames(240) + 1, i, 400, 240, &b));
    PASS();
}

TEST confetti_starts_staggered_and_is_deterministic(void)
{
    ConfettiBit a, b;
    unsigned visible_at_0 = 0, visible_at_20 = 0;
    for (unsigned i = 0; i < CONFETTI_COUNT; i++) {
        visible_at_0 += confetti_at(0, i, 400, 240, &a);
        visible_at_20 += confetti_at(20, i, 400, 240, &a);
    }
    ASSERT(visible_at_0 < visible_at_20);
    ASSERT(visible_at_20 > 0);
    ASSERT(confetti_at(60, 3, 400, 240, &a));
    ASSERT(confetti_at(60, 3, 400, 240, &b));
    ASSERT(memcmp(&a, &b, sizeof(a)) == 0);
    PASS();
}

SUITE(purchase_suite)
{
    RUN_TEST(flow_runs_the_purchase_after_the_delay);
    RUN_TEST(flow_shows_the_bar_for_a_minimum_time);
    RUN_TEST(flow_waits_for_the_purchase_even_after_the_minimum);
    RUN_TEST(flow_progress_grows);
    RUN_TEST(outcome_follows_the_transaction_result);
    RUN_TEST(backup_label_in_both_styles);
    RUN_TEST(backup_label_refuses_original_and_garbage);
    RUN_TEST(confetti_stays_on_the_screen_vertically);
    RUN_TEST(confetti_is_over_after_its_frames);
    RUN_TEST(confetti_starts_staggered_and_is_deterministic);
}
