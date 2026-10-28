#include <string.h>

#include "greatest.h"
#include "core/panel_bits.h"

/* Byte patterns written by the Marc Robledo save editor. */

TEST field_sizes(void)
{
    ASSERT_EQ(2, panel_bits_field_size(15));
    ASSERT_EQ(3, panel_bits_field_size(24));
    ASSERT_EQ(5, panel_bits_field_size(40));
    PASS();
}

TEST all_and_count(void)
{
    ASSERT_EQ(0x7FFFull, panel_bits_all(15));
    ASSERT_EQ(0xFFFFFFFFFFull, panel_bits_all(40));
    ASSERT_EQ(15, panel_bits_count(panel_bits_all(15)));
    ASSERT_EQ(0, panel_bits_count(0));
    PASS();
}

TEST standard_editor_patterns(void)
{
    const uint8_t all[2] = {0xFF, 0x7F};
    const uint8_t all_but_one[2] = {0xFF, 0x3F};
    const uint8_t none[2] = {0x00, 0x00};
    ASSERT_EQ(panel_bits_all(15), panel_bits_decode(all, PANEL_STANDARD, 15));
    ASSERT_EQ(panel_bits_all(14), panel_bits_decode(all_but_one, PANEL_STANDARD, 15));
    ASSERT_EQ(0, panel_bits_decode(none, PANEL_STANDARD, 15));
    ASSERT(panel_bits_unused_clear(all, PANEL_STANDARD, 15));
    PASS();
}

TEST spotpass_editor_patterns(void)
{
    const uint8_t all15[2] = {0xFF, 0xFE};
    const uint8_t most15[2] = {0xFF, 0xFC};
    const uint8_t all24[3] = {0xFF, 0xFF, 0xFF};
    const uint8_t most24[3] = {0xFF, 0xFF, 0xFE};
    const uint8_t all40[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t most40[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFE};
    ASSERT_EQ(panel_bits_all(15), panel_bits_decode(all15, PANEL_SPOTPASS, 15));
    ASSERT_EQ(panel_bits_all(14), panel_bits_decode(most15, PANEL_SPOTPASS, 15));
    ASSERT_EQ(panel_bits_all(24), panel_bits_decode(all24, PANEL_SPOTPASS, 24));
    ASSERT_EQ(panel_bits_all(23), panel_bits_decode(most24, PANEL_SPOTPASS, 24));
    ASSERT_EQ(panel_bits_all(40), panel_bits_decode(all40, PANEL_SPOTPASS, 40));
    ASSERT_EQ(panel_bits_all(39), panel_bits_decode(most40, PANEL_SPOTPASS, 40));
    ASSERT(panel_bits_unused_clear(all15, PANEL_SPOTPASS, 15));
    PASS();
}

TEST encode_matches_editor_patterns(void)
{
    uint8_t field[5] = {0};
    panel_bits_encode(field, PANEL_STANDARD, 15, panel_bits_all(15));
    ASSERT_EQ(0xFF, field[0]);
    ASSERT_EQ(0x7F, field[1]);

    memset(field, 0, sizeof(field));
    panel_bits_encode(field, PANEL_SPOTPASS, 15, panel_bits_all(14));
    ASSERT_EQ(0xFF, field[0]);
    ASSERT_EQ(0xFC, field[1]);
    PASS();
}

TEST encode_keeps_unused_bits(void)
{
    uint8_t std[2] = {0x00, 0x80};
    panel_bits_encode(std, PANEL_STANDARD, 15, 1);
    ASSERT_EQ(0x01, std[0]);
    ASSERT_EQ(0x80, std[1]);

    uint8_t sp[2] = {0x00, 0x01};
    panel_bits_encode(sp, PANEL_SPOTPASS, 15, 1);
    ASSERT_EQ(0x80, sp[0]);
    ASSERT_EQ(0x01, sp[1]);
    PASS();
}

TEST unused_bits_detected(void)
{
    const uint8_t std_bad[2] = {0x00, 0x80};
    const uint8_t sp_bad[2] = {0x00, 0x01};
    ASSERT_FALSE(panel_bits_unused_clear(std_bad, PANEL_STANDARD, 15));
    ASSERT_FALSE(panel_bits_unused_clear(sp_bad, PANEL_SPOTPASS, 15));
    PASS();
}

TEST roundtrip_every_single_piece(void)
{
    const unsigned counts[] = {15, 24, 40};
    const PanelKind kinds[] = {PANEL_STANDARD, PANEL_SPOTPASS};
    for (unsigned k = 0; k < 2; k++) {
        for (unsigned c = 0; c < 3; c++) {
            for (unsigned i = 0; i < counts[c]; i++) {
                uint8_t field[PANEL_BITS_MAX_BYTES] = {0};
                uint64_t one = (uint64_t)1 << i;
                panel_bits_encode(field, kinds[k], counts[c], one);
                ASSERT_EQ(one, panel_bits_decode(field, kinds[k], counts[c]));
                ASSERT(panel_bits_unused_clear(field, kinds[k], counts[c]));
            }
        }
    }
    PASS();
}

SUITE(panel_bits_suite)
{
    RUN_TEST(field_sizes);
    RUN_TEST(all_and_count);
    RUN_TEST(standard_editor_patterns);
    RUN_TEST(spotpass_editor_patterns);
    RUN_TEST(encode_matches_editor_patterns);
    RUN_TEST(encode_keeps_unused_bits);
    RUN_TEST(unused_bits_detected);
    RUN_TEST(roundtrip_every_single_piece);
}
