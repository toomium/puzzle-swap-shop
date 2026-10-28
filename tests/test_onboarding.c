#include "greatest.h"
#include "app/route.h"
#include "core/onboarding.h"
#include "core/size_text.h"

static void run_frames(Onboarding *o, unsigned n)
{
    for (unsigned i = 0; i < n; i++)
        onboarding_step(o);
}

TEST backup_runs_after_the_working_delay(void)
{
    Onboarding o;
    onboarding_init(&o);
    ASSERT_FALSE(onboarding_work_due(&o));
    ASSERT(onboarding_start(&o));
    ASSERT(onboarding_busy(&o));
    run_frames(&o, ONB_WORK_DELAY - 1);
    ASSERT_FALSE(onboarding_work_due(&o));
    run_frames(&o, 1);
    ASSERT(onboarding_work_due(&o));
    PASS();
}

TEST dots_turn_green_one_after_the_other(void)
{
    Onboarding o;
    onboarding_init(&o);
    onboarding_start(&o);
    run_frames(&o, ONB_WORK_DELAY);
    onboarding_finish(&o, true);
    ASSERT_EQ(ONB_REVEAL, o.phase);
    ASSERT_FALSE(onboarding_dot_green(&o, 0));
    run_frames(&o, ONB_DOT_STEP);
    ASSERT(onboarding_dot_green(&o, 0));
    ASSERT_FALSE(onboarding_dot_green(&o, 1));
    run_frames(&o, ONB_DOT_STEP);
    ASSERT(onboarding_dot_green(&o, 1));
    ASSERT_EQ(ONB_REVEAL, o.phase);
    run_frames(&o, ONB_HOLD);
    ASSERT_EQ(ONB_DONE, o.phase);
    ASSERT(onboarding_dot_green(&o, 0) && onboarding_dot_green(&o, 1));
    ASSERT_FALSE(onboarding_busy(&o));
    PASS();
}

TEST failure_leaves_dots_grey_and_allows_retry(void)
{
    Onboarding o;
    onboarding_init(&o);
    onboarding_start(&o);
    run_frames(&o, ONB_WORK_DELAY);
    onboarding_finish(&o, false);
    ASSERT_EQ(ONB_FAILED, o.phase);
    run_frames(&o, 100);
    ASSERT_FALSE(onboarding_dot_green(&o, 0));
    ASSERT_FALSE(onboarding_busy(&o));
    ASSERT(onboarding_start(&o));
    ASSERT_EQ(ONB_WORKING, o.phase);
    PASS();
}

TEST start_is_ignored_while_busy(void)
{
    Onboarding o;
    onboarding_init(&o);
    ASSERT(onboarding_start(&o));
    ASSERT_FALSE(onboarding_start(&o));
    onboarding_finish(&o, true);
    ASSERT_FALSE(onboarding_start(&o));
    PASS();
}

TEST finish_only_counts_while_working(void)
{
    Onboarding o;
    onboarding_init(&o);
    onboarding_finish(&o, true);
    ASSERT_EQ(ONB_WAIT, o.phase);
    PASS();
}

TEST size_text_units(void)
{
    char buf[16];
    size_text_format(buf, sizeof(buf), 20);
    ASSERT_STR_EQ("20 B", buf);
    size_text_format(buf, sizeof(buf), 0x60000);
    ASSERT_STR_EQ("384 KB", buf);
    size_text_format(buf, sizeof(buf), 1023);
    ASSERT_STR_EQ("1023 B", buf);
    size_text_format(buf, sizeof(buf), 1025);
    ASSERT_STR_EQ("2 KB", buf);
    size_text_format(buf, sizeof(buf), 3u * 1024u * 1024u);
    ASSERT_STR_EQ("3 MB", buf);
    PASS();
}

TEST route_follows_model_status(void)
{
    ASSERT_EQ(ROUTE_GALLERY, route_after_load(MODEL_READY));
    ASSERT_EQ(ROUTE_ONBOARDING, route_after_load(MODEL_NEED_ORIGINAL));
    ASSERT_EQ(ROUTE_E03, route_after_load(MODEL_READ_ONLY));
    ASSERT_EQ(ROUTE_E06, route_after_load(MODEL_INTERRUPTED));
    ASSERT_EQ(ROUTE_E01, route_after_load(MODEL_ERR_READ));
    PASS();
}

SUITE(onboarding_suite)
{
    RUN_TEST(backup_runs_after_the_working_delay);
    RUN_TEST(dots_turn_green_one_after_the_other);
    RUN_TEST(failure_leaves_dots_grey_and_allows_retry);
    RUN_TEST(start_is_ignored_while_busy);
    RUN_TEST(finish_only_counts_while_working);
    RUN_TEST(size_text_units);
    RUN_TEST(route_follows_model_status);
}
