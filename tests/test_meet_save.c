#include <stdlib.h>
#include <string.h>

#include "greatest.h"
#include "fixture.h"
#include "core/meet_save.h"
#include "core/panel_bits.h"

static uint8_t *s_sample;
static size_t s_len;

static void setup(void *arg)
{
    (void)arg;
    s_sample = fixture_load("meet_sample.dat", &s_len);
}

static void teardown(void *arg)
{
    (void)arg;
    fixture_free(s_sample);
    s_sample = NULL;
}

/* Owned pieces of panel 9 in tools/make_fixture.py SAMPLE_OWNED. */
static uint64_t sample_panel9(void)
{
    static const unsigned ranges[][2] = {{0, 6},   {8, 11},  {13, 13}, {16, 19}, {21, 21},
                                         {24, 27}, {29, 29}, {32, 33}, {35, 35}, {37, 39}};
    uint64_t mask = 0;
    for (size_t r = 0; r < sizeof(ranges) / sizeof(ranges[0]); r++)
        for (unsigned i = ranges[r][0]; i <= ranges[r][1]; i++)
            mask |= (uint64_t)1 << i;
    return mask;
}

TEST offsets_match_documentation(void)
{
    ASSERT_EQ(0x044BDCu, meet_save_panel_offset(0));
    ASSERT_EQ(0x044BE8u, meet_save_panel_offset(6));
    ASSERT_EQ(0x045578u, meet_save_panel_offset(7));
    ASSERT_EQ(0x0455BCu, meet_save_panel_offset(8));
    ASSERT_EQ(0x045578u + 55u * 0x44u, meet_save_panel_offset(62));
    ASSERT_EQ(0u, meet_save_panel_offset(63));
    PASS();
}

TEST validate_sizes(void)
{
    ASSERT_EQ(MEET_OK, meet_save_validate(s_sample, s_len));
    ASSERT_EQ(MEET_ERR_SIZE, meet_save_validate(s_sample, s_len - 1));
    ASSERT_EQ(MEET_ERR_SIZE, meet_save_validate(NULL, MEET_SAVE_SIZE));
    PASS();
}

TEST validate_rejects_unused_bits(void)
{
    size_t len;
    uint8_t *bad = fixture_load("meet_bad_unused.dat", &len);
    ASSERT(bad != NULL);
    ASSERT_EQ(MEET_ERR_UNUSED_BITS, meet_save_validate(bad, len));
    fixture_free(bad);
    PASS();
}

TEST reads_sample_panels(void)
{
    uint64_t owned;
    ASSERT_EQ(MEET_OK, meet_save_get_owned(s_sample, s_len, 0, &owned));
    ASSERT_EQ(panel_bits_all(14), owned);
    ASSERT_EQ(MEET_OK, meet_save_get_owned(s_sample, s_len, 3, &owned));
    ASSERT_EQ(panel_bits_all(15), owned);
    ASSERT_EQ(MEET_OK, meet_save_get_owned(s_sample, s_len, 7, &owned));
    ASSERT_EQ(panel_bits_all(24), owned);
    ASSERT_EQ(MEET_OK, meet_save_get_owned(s_sample, s_len, 9, &owned));
    ASSERT_EQ(sample_panel9(), owned);
    ASSERT_EQ(MEET_OK, meet_save_get_owned(s_sample, s_len, 1, &owned));
    ASSERT_EQ(0, owned);
    ASSERT_EQ(MEET_ERR_PANEL, meet_save_get_owned(s_sample, s_len, 63, &owned));
    PASS();
}

TEST add_pieces_changes_only_expected_bytes(void)
{
    uint8_t *before = malloc(s_len);
    ASSERT(before != NULL);
    memcpy(before, s_sample, s_len);

    /* Pieces 12, 14 and 22 of panel 9. */
    uint64_t pieces = ((uint64_t)1 << 12) | ((uint64_t)1 << 14) | ((uint64_t)1 << 22);
    ASSERT_EQ(MEET_OK, meet_save_add_pieces(s_sample, s_len, 9, pieces));

    size_t field = meet_save_panel_offset(9);
    for (size_t i = 0; i < s_len; i++) {
        if (i >= field && i < field + 5)
            continue;
        if (before[i] != s_sample[i]) {
            free(before);
            FAILm("byte outside the panel bitfield changed");
        }
    }
    uint64_t owned;
    meet_save_get_owned(s_sample, s_len, 9, &owned);
    ASSERT_EQ(sample_panel9() | pieces, owned);
    ASSERT_EQ(MEET_OK, meet_save_validate(s_sample, s_len));
    free(before);
    PASS();
}

TEST add_pieces_rejects_and_leaves_buffer(void)
{
    uint8_t *before = malloc(s_len);
    ASSERT(before != NULL);
    memcpy(before, s_sample, s_len);

    ASSERT_EQ(MEET_ERR_OWNED, meet_save_add_pieces(s_sample, s_len, 9, 1));
    ASSERT_EQ(MEET_ERR_RANGE, meet_save_add_pieces(s_sample, s_len, 0, (uint64_t)1 << 15));
    ASSERT_EQ(MEET_ERR_PANEL, meet_save_add_pieces(s_sample, s_len, 70, 1));
    ASSERT_EQ(MEET_ERR_SIZE, meet_save_add_pieces(s_sample, 10, 0, 1));
    ASSERT_EQ(0, memcmp(before, s_sample, s_len));
    free(before);
    PASS();
}

TEST completes_standard_panel(void)
{
    ASSERT_EQ(MEET_OK, meet_save_add_pieces(s_sample, s_len, 0, (uint64_t)1 << 14));
    size_t off = meet_save_panel_offset(0);
    ASSERT_EQ(0xFF, s_sample[off]);
    ASSERT_EQ(0x7F, s_sample[off + 1]);
    PASS();
}

TEST remove_pieces_round_trips(void)
{
    uint8_t *before = malloc(s_len);
    ASSERT(before != NULL);
    memcpy(before, s_sample, s_len);

    uint64_t pieces = ((uint64_t)1 << 0) | ((uint64_t)1 << 39);
    ASSERT_EQ(MEET_OK, meet_save_remove_pieces(s_sample, s_len, 9, pieces));
    uint64_t owned;
    meet_save_get_owned(s_sample, s_len, 9, &owned);
    ASSERT_EQ(sample_panel9() & ~pieces, owned);
    ASSERT_EQ(MEET_OK, meet_save_validate(s_sample, s_len));
    ASSERT_EQ(MEET_OK, meet_save_add_pieces(s_sample, s_len, 9, pieces));
    ASSERT_EQ(0, memcmp(before, s_sample, s_len));

    ASSERT_EQ(MEET_ERR_NOT_OWNED, meet_save_remove_pieces(s_sample, s_len, 9, (uint64_t)1 << 12));
    ASSERT_EQ(MEET_ERR_RANGE, meet_save_remove_pieces(s_sample, s_len, 0, (uint64_t)1 << 15));
    ASSERT_EQ(MEET_ERR_PANEL, meet_save_remove_pieces(s_sample, s_len, 70, 1));
    ASSERT_EQ(MEET_ERR_SIZE, meet_save_remove_pieces(s_sample, 10, 0, 1));
    ASSERT_EQ(0, memcmp(before, s_sample, s_len));
    free(before);
    PASS();
}

SUITE(meet_save_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(offsets_match_documentation);
    RUN_TEST(validate_sizes);
    RUN_TEST(validate_rejects_unused_bits);
    RUN_TEST(reads_sample_panels);
    RUN_TEST(add_pieces_changes_only_expected_bytes);
    RUN_TEST(add_pieces_rejects_and_leaves_buffer);
    RUN_TEST(completes_standard_panel);
    RUN_TEST(remove_pieces_round_trips);
}
