#include <string.h>

#include "greatest.h"
#include "core/backup_list.h"
#include "core/list_nav.h"
#include "core/strings.h"

static void reset_language(void *arg)
{
    (void)arg;
    strings_set_language(LANG_DE);
}

static Manifest make_manifest(BackupReason reason)
{
    Manifest m;
    memset(&m, 0, sizeof(m));
    m.reason = reason;
    m.created = (Timestamp){2026, 10, 26, 18, 4, 9};
    return m;
}

TEST list_move_scrolls_with_focus(void)
{
    ListNav l;
    list_nav_init(&l);
    ASSERT_FALSE(list_nav_move(&l, -1, 8, 3));
    ASSERT(list_nav_move(&l, 1, 8, 3));
    ASSERT(list_nav_move(&l, 1, 8, 3));
    ASSERT_EQ(0, l.first);
    ASSERT(list_nav_move(&l, 1, 8, 3));
    ASSERT_EQ(3, l.index);
    ASSERT_EQ(1, l.first);
    ASSERT(list_nav_move(&l, -1, 8, 3));
    ASSERT_EQ(1, l.first); /* still inside the window */
    ASSERT(list_nav_move(&l, -1, 8, 3));
    ASSERT_EQ(1, l.index);
    ASSERT_EQ(1, l.first);
    ASSERT(list_nav_move(&l, -1, 8, 3));
    ASSERT_EQ(0, l.first); /* the window follows the focus up */
    PASS();
}

TEST list_move_stops_at_the_ends(void)
{
    ListNav l = {7, 5};
    ASSERT_FALSE(list_nav_move(&l, 1, 8, 3));
    ASSERT_EQ(7, l.index);
    ASSERT_EQ(5, l.first);
    ASSERT_FALSE(list_nav_move(&l, 1, 0, 3));
    PASS();
}

TEST list_page_jumps_a_window(void)
{
    ListNav l;
    list_nav_init(&l);
    ASSERT(list_nav_page(&l, 1, 8, 3));
    ASSERT_EQ(3, l.index);
    ASSERT_EQ(3, l.first);
    ASSERT(list_nav_page(&l, 1, 8, 3));
    ASSERT_EQ(6, l.index);
    ASSERT_EQ(5, l.first); /* no empty rows at the bottom */
    ASSERT(list_nav_page(&l, 1, 8, 3));
    ASSERT_EQ(7, l.index);
    ASSERT_FALSE(list_nav_page(&l, 1, 8, 3));
    ASSERT(list_nav_page(&l, -1, 8, 3));
    ASSERT_EQ(4, l.index);
    ASSERT(list_nav_page(&l, -1, 8, 3));
    ASSERT(list_nav_page(&l, -1, 8, 3));
    ASSERT_EQ(0, l.index);
    ASSERT_EQ(0, l.first);
    ASSERT_FALSE(list_nav_page(&l, -1, 8, 3));
    PASS();
}

TEST list_clamp_after_the_list_shrank(void)
{
    ListNav l = {9, 7};
    list_nav_clamp(&l, 4, 3);
    ASSERT_EQ(3, l.index);
    ASSERT_EQ(1, l.first);
    list_nav_clamp(&l, 2, 3);
    ASSERT_EQ(1, l.index);
    ASSERT_EQ(0, l.first);
    list_nav_clamp(&l, 0, 3);
    ASSERT_EQ(0, l.index);
    PASS();
}

TEST list_more_rows_indicators(void)
{
    ListNav l = {0, 0};
    ASSERT_FALSE(list_nav_more_above(&l));
    ASSERT(list_nav_more_below(&l, 5, 3));
    l.first = 2;
    ASSERT(list_nav_more_above(&l));
    ASSERT_FALSE(list_nav_more_below(&l, 5, 3));
    ASSERT_FALSE(list_nav_more_below(&l, 2, 3));
    PASS();
}

TEST reason_of_each_kind(void)
{
    char out[64];
    Manifest m = make_manifest(BACKUP_REASON_ORIGINAL);
    backup_row_reason(BACKUP_ORIGINAL, &m, NULL, out, sizeof(out));
    ASSERT_STR_EQ("Ursicherung", out);

    m = make_manifest(BACKUP_REASON_PURCHASE);
    m.has_purchase = true;
    m.panel = 7;
    m.pieces = 3;
    backup_row_reason("20261026-180409", &m, "Sommerwiese", out, sizeof(out));
    ASSERT_STR_EQ("vor Kauf: Sommerwiese, 3 Teile", out);
    m.pieces = 1;
    backup_row_reason("20261026-180409", &m, "Sommerwiese", out, sizeof(out));
    ASSERT_STR_EQ("vor Kauf: Sommerwiese, 1 Teil", out);
    backup_row_reason("20261026-180409", &m, NULL, out, sizeof(out));
    ASSERT_STR_EQ("vor Kauf", out);
    m.has_purchase = false;
    backup_row_reason("20261026-180409", &m, "Sommerwiese", out, sizeof(out));
    ASSERT_STR_EQ("vor Kauf", out);

    m = make_manifest(BACKUP_REASON_RESTORE);
    backup_row_reason("20261026-180409", &m, NULL, out, sizeof(out));
    ASSERT_STR_EQ("vor Wiederherstellung", out);

    m = make_manifest(BACKUP_REASON_DIAG);
    backup_row_reason("20261026-180409", &m, NULL, out, sizeof(out));
    ASSERT_STR_EQ("vor Änderung", out);
    PASS();
}

TEST reason_in_english(void)
{
    char out[64];
    strings_set_language(LANG_EN);
    Manifest m = make_manifest(BACKUP_REASON_PURCHASE);
    m.has_purchase = true;
    m.pieces = 2;
    backup_row_reason("20261026-180409", &m, "Meadow", out, sizeof(out));
    ASSERT_STR_EQ("before purchase: Meadow, 2 pieces", out);
    PASS();
}

TEST date_from_name_or_manifest(void)
{
    char out[24];
    Manifest m = make_manifest(BACKUP_REASON_PURCHASE);
    ASSERT(backup_row_date("20261026-180409", &m, true, out, sizeof(out)));
    ASSERT_STR_EQ("26.10.2026, 18:04", out);
    ASSERT(backup_row_date("20261026-180409", &m, false, out, sizeof(out)));
    ASSERT_STR_EQ("2026-10-26 18:04", out);

    /* The original has no date in its name: the manifest tells. */
    m = make_manifest(BACKUP_REASON_ORIGINAL);
    ASSERT(backup_row_date(BACKUP_ORIGINAL, &m, true, out, sizeof(out)));
    ASSERT_STR_EQ("26.10.2026, 18:04", out);
    ASSERT(backup_row_date(BACKUP_ORIGINAL, &m, false, out, sizeof(out)));
    ASSERT_STR_EQ("2026-10-26 18:04", out);

    m.created.month = 13;
    ASSERT_FALSE(backup_row_date(BACKUP_ORIGINAL, &m, true, out, sizeof(out)));
    PASS();
}

TEST only_the_original_is_locked(void)
{
    ASSERT(backup_row_locked(BACKUP_ORIGINAL));
    ASSERT_FALSE(backup_row_locked("20261026-180409"));
    PASS();
}

SUITE(backup_list_suite)
{
    SET_TEARDOWN(reset_language, NULL);
    RUN_TEST(list_move_scrolls_with_focus);
    RUN_TEST(list_move_stops_at_the_ends);
    RUN_TEST(list_page_jumps_a_window);
    RUN_TEST(list_clamp_after_the_list_shrank);
    RUN_TEST(list_more_rows_indicators);
    RUN_TEST(reason_of_each_kind);
    RUN_TEST(reason_in_english);
    RUN_TEST(date_from_name_or_manifest);
    RUN_TEST(only_the_original_is_locked);
}
