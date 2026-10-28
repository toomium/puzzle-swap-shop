#include <string.h>

#include "greatest.h"
#include "core/lz11.h"

TEST detects_header(void)
{
    const uint8_t packed[] = {0x11, 0x03, 0x00, 0x00, 0x00, 'a', 'b', 'c'};
    ASSERT(lz11_is_packed(packed, sizeof(packed)));
    ASSERT_EQ(3u, lz11_unpacked_size(packed, sizeof(packed)));
    ASSERT(!lz11_is_packed((const uint8_t *)"CGFX", 4));
    ASSERT(!lz11_is_packed(packed, 3));
    ASSERT(!lz11_is_packed(NULL, 0));
    const uint8_t zero[] = {0x11, 0, 0, 0};
    ASSERT(!lz11_is_packed(zero, sizeof(zero))); /* size 0 needs the extended field */
    const uint8_t ext[] = {0x11, 0, 0, 0, 0x00, 0x00, 0x01, 0x00};
    ASSERT_EQ(0x10000u, lz11_unpacked_size(ext, sizeof(ext)));
    PASS();
}

/* The start of the real block of panel 7: 11 00 01 01, flag 00, then "CGFX" as literals. */
TEST literals(void)
{
    const uint8_t packed[] = {0x11, 0x08, 0x00, 0x00, 0x00, 'C', 'G',
                              'F',  'X',  0xFF, 0xFE, 0x14, 0x00};
    uint8_t out[8];
    ASSERT_EQ(8u, lz11_unpack(packed, sizeof(packed), out, sizeof(out)));
    ASSERT_MEM_EQ("CGFX\xFF\xFE\x14\x00", out, 8);
    PASS();
}

TEST copies_in_all_three_lengths(void)
{
    /* 'A' once, then: kind >= 2 (2 bytes): count 4 from distance 1 -> AAAAA;
     * kind 0 (3 bytes): count 0x11 from distance 1; kind 1 (4 bytes): count 0x111. */
    uint8_t packed[32];
    size_t n = 0;
    packed[n++] = 0x11;
    size_t total = 1 + 4 + 0x11 + 0x111;
    packed[n++] = (uint8_t)total;
    packed[n++] = (uint8_t)(total >> 8);
    packed[n++] = 0;
    packed[n++] = 0x70; /* flags: literal, copy, copy, copy */
    packed[n++] = 'A';
    packed[n++] = 0x30 | 0x0; /* kind 3: count 4, distance (0<<8 | 0) + 1 */
    packed[n++] = 0x00;
    packed[n++] = 0x00; /* kind 0: count (0<<4 | 0) + 0x11, distance (0<<8 | 0) + 1 */
    packed[n++] = 0x00;
    packed[n++] = 0x00;
    packed[n++] = 0x10; /* kind 1: count ((0<<12) | (0<<4) | 0) + 0x111, distance 1 */
    packed[n++] = 0x00;
    packed[n++] = 0x00;
    packed[n++] = 0x00;
    uint8_t out[1 + 4 + 0x11 + 0x111];
    ASSERT_EQ(sizeof(out), lz11_unpack(packed, n, out, sizeof(out)));
    for (size_t i = 0; i < sizeof(out); i++)
        ASSERT_EQ('A', out[i]);
    PASS();
}

TEST copies_overlapping_pattern(void)
{
    /* "ab", then copy 6 bytes from distance 2: ababababab... */
    const uint8_t packed[] = {0x11, 0x08, 0x00, 0x00, 0x20, 'a', 'b', 0x50, 0x01};
    uint8_t out[8];
    ASSERT_EQ(8u, lz11_unpack(packed, sizeof(packed), out, sizeof(out)));
    ASSERT_MEM_EQ("abababab", out, 8);
    PASS();
}

TEST rejects_damaged_data(void)
{
    uint8_t out[16];
    const uint8_t cut[] = {0x11, 0x08, 0x00, 0x00, 0x00, 'C', 'G'};
    ASSERT_EQ(0u, lz11_unpack(cut, sizeof(cut), out, sizeof(out)));
    /* A copy from before the start. */
    const uint8_t before[] = {0x11, 0x08, 0x00, 0x00, 0x80, 0x50, 0x05};
    ASSERT_EQ(0u, lz11_unpack(before, sizeof(before), out, sizeof(out)));
    /* A copy that runs past the announced size. */
    const uint8_t over[] = {0x11, 0x03, 0x00, 0x00, 0x40, 'a', 0x50, 0x00};
    ASSERT_EQ(0u, lz11_unpack(over, sizeof(over), out, sizeof(out)));
    /* Does not fit the buffer. */
    const uint8_t lit[] = {0x11, 0x08, 0x00, 0x00, 0x00, 1, 2, 3, 4, 5, 6, 7, 8};
    ASSERT_EQ(0u, lz11_unpack(lit, sizeof(lit), out, 4));
    ASSERT_EQ(0u, lz11_unpack((const uint8_t *)"CGFX", 4, out, sizeof(out)));
    PASS();
}

SUITE(lz11_suite)
{
    RUN_TEST(detects_header);
    RUN_TEST(literals);
    RUN_TEST(copies_in_all_three_lengths);
    RUN_TEST(copies_overlapping_pattern);
    RUN_TEST(rejects_damaged_data);
}
