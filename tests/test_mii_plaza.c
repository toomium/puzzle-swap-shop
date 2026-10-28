#include "greatest.h"
#include "core/mii_plaza.h"

#define EUR 0x0004001000022800ull
#define USA 0x0004001000021800ull
#define JPN 0x0004001000020800ull

TEST save_id_rule_matches_table(void)
{
    for (int r = 0; r < PLAZA_REGION_COUNT; r++) {
        const PlazaTitle *t = mii_plaza_title((PlazaRegion)r);
        ASSERT(t != NULL);
        ASSERT_EQ(t->save_id, mii_plaza_save_id(t->title_id));
    }
    /* Seen on an EUR console: sysdata/00020228 holds meet.dat. */
    ASSERT_EQ(0x00020228u, mii_plaza_save_id(EUR));
    ASSERT_EQ(NULL, mii_plaza_title(PLAZA_REGION_COUNT));
    PASS();
}

TEST find_prefers_console_region(void)
{
    const uint64_t list[] = {0x0004001000021000ull, JPN, EUR, USA};
    ASSERT_EQ(PLAZA_USA, mii_plaza_find(list, 4, 1)->region);
    ASSERT_EQ(PLAZA_JPN, mii_plaza_find(list, 4, 0)->region);
    ASSERT_EQ(PLAZA_EUR, mii_plaza_find(list, 4, 2)->region);
    /* AUS consoles run the EUR title. */
    ASSERT_EQ(PLAZA_EUR, mii_plaza_find(list, 4, 3)->region);
    PASS();
}

TEST find_falls_back_and_reports_missing(void)
{
    const uint64_t only_jpn[] = {JPN};
    const uint64_t other[] = {0x0004001000022000ull, 0x0004003000008f02ull};
    ASSERT_EQ(PLAZA_JPN, mii_plaza_find(only_jpn, 1, 2)->region);
    ASSERT_EQ(PLAZA_JPN, mii_plaza_find(only_jpn, 1, 6)->region);
    ASSERT_EQ(NULL, mii_plaza_find(other, 2, 2));
    ASSERT_EQ(NULL, mii_plaza_find(NULL, 0, 2));
    PASS();
}

SUITE(mii_plaza_suite)
{
    RUN_TEST(save_id_rule_matches_table);
    RUN_TEST(find_prefers_console_region);
    RUN_TEST(find_falls_back_and_reports_missing);
}
