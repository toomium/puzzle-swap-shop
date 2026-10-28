#include <string.h>

#include "greatest.h"
#include "fixture.h"
#include "core/pnel.h"

/* Language indices in CFG_Language order. */
enum { LANG_JP = 0, LANG_EN = 1, LANG_DE = 3, LANG_PT = 9, LANG_TW = 11 };

static uint8_t *s_buf;
static size_t s_len;

static void setup(void *arg)
{
    (void)arg;
    s_buf = fixture_load("pnel_sample.dat", &s_len);
}

static void teardown(void *arg)
{
    (void)arg;
    fixture_free(s_buf);
    s_buf = NULL;
}

TEST header(void)
{
    PnelHeader h;
    ASSERT_EQ(PNEL_OK, pnel_read_header(s_buf, s_len, &h));
    ASSERT_EQ(2u, h.count);
    PASS();
}

TEST header_errors(void)
{
    PnelHeader h;
    ASSERT_EQ(PNEL_ERR_SIZE, pnel_read_header(s_buf, 4, &h));
    /* Header promises two entries but the buffer only holds one. */
    ASSERT_EQ(PNEL_ERR_SIZE, pnel_read_header(s_buf, PNEL_HEADER_SIZE + PNEL_ENTRY_SIZE, &h));

    s_buf[0x10] ^= 1;
    ASSERT_EQ(PNEL_ERR_CRC, pnel_read_header(s_buf, s_len, &h));
    s_buf[0x10] ^= 1;

    s_buf[0] = 'X';
    ASSERT_EQ(PNEL_ERR_MAGIC, pnel_read_header(s_buf, s_len, &h));
    PASS();
}

TEST entries(void)
{
    PnelEntry e;
    ASSERT_EQ(PNEL_OK, pnel_read_entry(s_buf, s_len, 0, &e));
    ASSERT_EQ(7u, e.order);
    ASSERT_EQ(24u, pnel_piece_count(e.piece_count_id));
    ASSERT_EQ(30u, e.image_size);
    ASSERT_EQ(PNEL_OK, pnel_verify_blocks(s_buf, s_len, &e));

    ASSERT_EQ(PNEL_OK, pnel_read_entry(s_buf, s_len, 1, &e));
    ASSERT_EQ(8u, e.order);
    ASSERT_EQ(40u, pnel_piece_count(e.piece_count_id));
    ASSERT_EQ(3, e.pink);
    ASSERT_EQ(PNEL_OK, pnel_verify_blocks(s_buf, s_len, &e));

    ASSERT_EQ(PNEL_ERR_INDEX, pnel_read_entry(s_buf, s_len, 2, &e));
    PASS();
}

TEST corrupted_block_detected(void)
{
    PnelEntry e;
    ASSERT_EQ(PNEL_OK, pnel_read_entry(s_buf, s_len, 0, &e));
    s_buf[e.image_offset] ^= 0xFF;
    ASSERT_EQ(PNEL_ERR_CRC, pnel_verify_blocks(s_buf, s_len, &e));
    PASS();
}

TEST corrupted_entry_detected(void)
{
    size_t len;
    uint8_t *bad = fixture_load("pnel_bad_entry_crc.dat", &len);
    ASSERT(bad != NULL);
    PnelEntry e;
    ASSERT_EQ(PNEL_ERR_CRC, pnel_read_entry(bad, len, 0, &e));
    fixture_free(bad);
    PASS();
}

TEST names_as_utf8(void)
{
    char name[64];
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 0, LANG_DE, name, sizeof(name)));
    ASSERT_STR_EQ("Sommerwiese", name);
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 0, LANG_EN, name, sizeof(name)));
    ASSERT_STR_EQ("Summer Meadow", name);
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 0, LANG_JP, name, sizeof(name)));
    ASSERT_STR_EQ("\xE5\xA4\x8F\xE3\x81\xAE\xE8\x8D\x89\xE5\x8E\x9F", name); /* 夏の草原 */
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 0, LANG_PT, name, sizeof(name)));
    ASSERT_STR_EQ("Star \xF0\x9F\x8C\x9F", name); /* surrogate pair */
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 1, LANG_DE, name, sizeof(name)));
    ASSERT_STR_EQ("Leuchtturm", name);
    /* No slot of its own: English. */
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 0, LANG_TW, name, sizeof(name)));
    ASSERT_STR_EQ("Summer Meadow", name);
    ASSERT_EQ(PNEL_ERR_LANG, pnel_entry_name(s_buf, s_len, 0, 12, name, sizeof(name)));
    PASS();
}

TEST name_truncates_at_character_boundary(void)
{
    char name[5];
    /* "夏" is 3 bytes in UTF-8, so only one character fits into 4 bytes plus NUL. */
    ASSERT_EQ(PNEL_OK, pnel_entry_name(s_buf, s_len, 0, LANG_JP, name, sizeof(name)));
    ASSERT_STR_EQ("\xE5\xA4\x8F", name);
    PASS();
}

TEST piece_count_ids(void)
{
    ASSERT_EQ(15u, pnel_piece_count(0));
    ASSERT_EQ(540u, pnel_piece_count(3));
    ASSERT_EQ(0u, pnel_piece_count(4));
    PASS();
}

SUITE(pnel_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(header);
    RUN_TEST(header_errors);
    RUN_TEST(entries);
    RUN_TEST(corrupted_block_detected);
    RUN_TEST(corrupted_entry_detected);
    RUN_TEST(names_as_utf8);
    RUN_TEST(name_truncates_at_character_boundary);
    RUN_TEST(piece_count_ids);
}
