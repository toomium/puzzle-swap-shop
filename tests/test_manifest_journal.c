#include <string.h>

#include "greatest.h"
#include "core/journal_record.h"
#include "core/manifest.h"

static Manifest sample_manifest(void)
{
    Manifest m;
    memset(&m, 0, sizeof(m));
    strcpy(m.app_version, "0.1.0");
    m.created = (Timestamp){2026, 10, 26, 19, 24, 5};
    m.reason = BACKUP_REASON_PURCHASE;
    m.title_id = 0x0004001000022800ull;
    m.meet_size = 0x60000;
    m.meet_crc = 0x9db8d5a4;
    m.coin_size = 0x14;
    m.coin_crc = 0x0000beef;
    return m;
}

TEST manifest_round_trip(void)
{
    char text[MANIFEST_TEXT_MAX];
    size_t len;
    Manifest m = sample_manifest(), back;
    ASSERT_EQ(MANIFEST_OK, manifest_format(&m, text, sizeof(text), &len));
    ASSERT_EQ(strlen(text), len);
    ASSERT(strstr(text, "created=2026-10-26 19:24:05\n") != NULL);
    ASSERT(strstr(text, "title_id=0004001000022800\n") != NULL);
    ASSERT(strstr(text, "meet_crc=9db8d5a4\n") != NULL);
    ASSERT_EQ(MANIFEST_OK, manifest_parse(text, len, &back));
    ASSERT_EQ(0, memcmp(&m, &back, sizeof(m)));

    for (int r = 0; r < BACKUP_REASON_COUNT; r++) {
        m.reason = (BackupReason)r;
        ASSERT_EQ(MANIFEST_OK, manifest_format(&m, text, sizeof(text), &len));
        ASSERT_EQ(MANIFEST_OK, manifest_parse(text, len, &back));
        ASSERT_EQ(r, (int)back.reason);
    }
    PASS();
}

TEST manifest_optional_fields(void)
{
    char text[MANIFEST_TEXT_MAX];
    size_t len;
    Manifest m = sample_manifest(), back;
    m.has_coins = true;
    m.coins = 120;
    m.has_purchase = true;
    m.panel = 9;
    m.pieces = 3;
    ASSERT_EQ(MANIFEST_OK, manifest_format(&m, text, sizeof(text), &len));
    ASSERT(strstr(text, "coins=120\npanel=9\npieces=3\n") != NULL);
    ASSERT_EQ(MANIFEST_OK, manifest_parse(text, len, &back));
    ASSERT_EQ(0, memcmp(&m, &back, sizeof(m)));

    /* Without them, or with broken values, they read as absent. */
    m = sample_manifest();
    ASSERT_EQ(MANIFEST_OK, manifest_format(&m, text, sizeof(text), &len));
    ASSERT(strstr(text, "coins=") == NULL);
    strcat(text, "coins=x\npanel=9\n");
    ASSERT_EQ(MANIFEST_OK, manifest_parse(text, strlen(text), &back));
    ASSERT_FALSE(back.has_coins);
    ASSERT_FALSE(back.has_purchase);
    PASS();
}

TEST manifest_rejects(void)
{
    char text[MANIFEST_TEXT_MAX];
    size_t len;
    Manifest m = sample_manifest(), back;
    ASSERT_EQ(MANIFEST_ERR_SPACE, manifest_format(&m, text, 40, &len));
    m.created.month = 13;
    ASSERT_EQ(MANIFEST_ERR_MISSING, manifest_format(&m, text, sizeof(text), &len));

    const char *wrong_format = "format=2\n";
    ASSERT_EQ(MANIFEST_ERR_FORMAT, manifest_parse(wrong_format, strlen(wrong_format), &back));
    const char *no_crc = "format=1\napp_version=0.1.0\ncreated=2026-10-26 19:24:05\n"
                         "reason=purchase\ntitle_id=0004001000022800\nmeet_size=393216\n"
                         "coin_size=20\ncoin_crc=0000beef\n";
    ASSERT_EQ(MANIFEST_ERR_MISSING, manifest_parse(no_crc, strlen(no_crc), &back));
    ASSERT_EQ(MANIFEST_ERR_SYNTAX, manifest_parse("garbage", 7, &back));
    PASS();
}

static JournalRecord sample_record(void)
{
    JournalRecord r;
    memset(&r, 0, sizeof(r));
    r.op = JOURNAL_OP_PURCHASE;
    strcpy(r.backup, "20261026-192405");
    r.meet_crc_before = 0x11111111;
    r.coin_crc_before = 0x22222222;
    r.meet_crc_after = 0x33333333;
    r.coin_crc_after = 0x44444444;
    return r;
}

TEST journal_round_trip(void)
{
    char text[JOURNAL_TEXT_MAX];
    size_t len;
    JournalRecord r = sample_record(), back;
    for (int op = 0; op < JOURNAL_OP_COUNT; op++) {
        r.op = (JournalOp)op;
        ASSERT_EQ(JOURNAL_REC_OK, journal_record_format(&r, text, sizeof(text), &len));
        ASSERT_EQ(JOURNAL_REC_OK, journal_record_parse(text, len, &back));
        ASSERT_EQ(0, memcmp(&r, &back, sizeof(r)));
    }
    ASSERT(len > 6 && strcmp(text + len - 6, "end=1\n") == 0);
    PASS();
}

TEST journal_detects_truncation_and_bad_names(void)
{
    char text[JOURNAL_TEXT_MAX];
    size_t len;
    JournalRecord r = sample_record(), back;
    ASSERT_EQ(JOURNAL_REC_OK, journal_record_format(&r, text, sizeof(text), &len));
    /* Every cut before the final line must be rejected. */
    for (size_t cut = 0; cut < len - 1; cut++)
        ASSERT(journal_record_parse(text, cut, &back) != JOURNAL_REC_OK);

    strcpy(r.backup, "../../evil");
    ASSERT_EQ(JOURNAL_REC_ERR_MISSING, journal_record_format(&r, text, sizeof(text), &len));
    const char *evil = "format=1\nop=purchase\nbackup=../x\nmeet_crc_before=1\n"
                       "coin_crc_before=1\nmeet_crc_after=1\ncoin_crc_after=1\nend=1\n";
    ASSERT_EQ(JOURNAL_REC_ERR_MISSING, journal_record_parse(evil, strlen(evil), &back));
    PASS();
}

SUITE(manifest_journal_suite)
{
    RUN_TEST(manifest_round_trip);
    RUN_TEST(manifest_optional_fields);
    RUN_TEST(manifest_rejects);
    RUN_TEST(journal_round_trip);
    RUN_TEST(journal_detects_truncation_and_bad_names);
}
