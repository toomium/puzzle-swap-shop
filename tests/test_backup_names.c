#include <string.h>

#include "greatest.h"
#include "core/backup_names.h"

static const Timestamp T = {2026, 10, 26, 19, 24, 5};

TEST timestamps(void)
{
    char s[20];
    Timestamp t;
    ASSERT(timestamp_format(&T, s));
    ASSERT_STR_EQ("2026-10-26 19:24:05", s);
    ASSERT(timestamp_parse(s, &t));
    ASSERT_EQ(0, memcmp(&T, &t, sizeof(t)));

    Timestamp bad = {2026, 2, 29, 0, 0, 0};
    ASSERT_FALSE(timestamp_valid(&bad));
    bad.year = 2028;
    ASSERT(timestamp_valid(&bad));
    ASSERT_FALSE(timestamp_parse("2026-10-26 19:24", &t));
    ASSERT_FALSE(timestamp_parse("2026-13-26 19:24:05", &t));
    ASSERT_FALSE(timestamp_parse("2026-10-26T19:24:05", &t));
    PASS();
}

TEST from_unix(void)
{
    Timestamp t;
    ASSERT(timestamp_from_unix(1793042645, &t));
    ASSERT_EQ(0, memcmp(&T, &t, sizeof(t)));

    static const struct {
        int64_t secs;
        Timestamp t;
    } cases[] = {
        {951782400, {2000, 2, 29, 0, 0, 0}},
        {1735689599, {2024, 12, 31, 23, 59, 59}},
        {946684800, {2000, 1, 1, 0, 0, 0}},
        {4107585600, {2100, 3, 1, 12, 0, 0}},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        ASSERT(timestamp_from_unix(cases[i].secs, &t));
        ASSERT_EQ(0, memcmp(&cases[i].t, &t, sizeof(t)));
    }
    ASSERT_FALSE(timestamp_from_unix(946684799, &t)); /* 1999 */
    ASSERT_FALSE(timestamp_from_unix(0, &t));
    ASSERT_FALSE(timestamp_from_unix(-1, &t));
    PASS();
}

TEST names(void)
{
    char n[BACKUP_NAME_MAX];
    ASSERT(backup_name_make(&T, 0, n));
    ASSERT_STR_EQ("20261026-192405", n);
    ASSERT(backup_name_make(&T, 2, n));
    ASSERT_STR_EQ("20261026-192405-2", n);
    ASSERT_FALSE(backup_name_make(&T, 1, n));
    ASSERT_FALSE(backup_name_make(&T, 10, n));

    ASSERT(backup_name_valid("000-original"));
    ASSERT(backup_name_valid("20261026-192405"));
    ASSERT(backup_name_valid("20261026-192405-9"));
    ASSERT_FALSE(backup_name_valid("20261026-192405-1"));
    ASSERT_FALSE(backup_name_valid("20261326-192405"));
    ASSERT_FALSE(backup_name_valid("../20261026-1924"));
    ASSERT_FALSE(backup_name_valid("20261026/192405"));
    ASSERT_FALSE(backup_name_valid(""));
    ASSERT(backup_name_is_original(BACKUP_ORIGINAL));
    ASSERT_FALSE(backup_name_is_original("20261026-192405"));
    PASS();
}

TEST prune_keeps_newest_and_original(void)
{
    const char *names[] = {
        "20261026-120000", "000-original",      "20261024-090000",
        "garbage",         "20261026-120000-2", "20261025-235959",
    };
    bool drop[6];
    ASSERT_EQ(2, backup_prune_select(names, 6, 2, drop));
    ASSERT_FALSE(drop[0]);
    ASSERT_FALSE(drop[1]);
    ASSERT(drop[2]);
    ASSERT_FALSE(drop[3]);
    ASSERT_FALSE(drop[4]);
    ASSERT(drop[5]);

    ASSERT_EQ(4, backup_prune_select(names, 6, 0, drop));
    ASSERT_FALSE(drop[1]);
    ASSERT_EQ(0, backup_prune_select(names, 6, 20, drop));
    PASS();
}

SUITE(backup_names_suite)
{
    RUN_TEST(timestamps);
    RUN_TEST(from_unix);
    RUN_TEST(names);
    RUN_TEST(prune_keeps_newest_and_original);
}
