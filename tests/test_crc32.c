#include "greatest.h"
#include "core/crc32.h"

TEST known_vectors(void)
{
    ASSERT_EQ(0x00000000u, crc32("", 0));
    ASSERT_EQ(0xCBF43926u, crc32("123456789", 9));
    ASSERT_EQ(0x414FA339u, crc32("The quick brown fox jumps over the lazy dog", 43));
    PASS();
}

TEST incremental_matches_one_shot(void)
{
    uint32_t crc = crc32_update(0, "12345", 5);
    crc = crc32_update(crc, "6789", 4);
    ASSERT_EQ(0xCBF43926u, crc);
    PASS();
}

SUITE(crc32_suite)
{
    RUN_TEST(known_vectors);
    RUN_TEST(incremental_matches_one_shot);
}
