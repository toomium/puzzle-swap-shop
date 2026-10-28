#include <stdlib.h>
#include <string.h>

#include "greatest.h"
#include "fixture.h"
#include "core/bmp.h"
#include "core/cgfx.h"
#include "core/pica_tex.h"

static uint8_t *s_buf;
static size_t s_len;

static void setup(void *arg)
{
    (void)arg;
    s_buf = fixture_load("sample_rgb565_16x8.cgfx", &s_len);
}

static void teardown(void *arg)
{
    (void)arg;
    fixture_free(s_buf);
    s_buf = NULL;
}

static void put32(size_t at, uint32_t v)
{
    memcpy(s_buf + at, &v, 4); /* the host is little endian wherever the tests run */
}

TEST finds_texture(void)
{
    CgfxTexture t;
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(s_buf, s_len, &t));
    ASSERT_EQ(16u, t.width);
    ASSERT_EQ(8u, t.height);
    ASSERT_EQ((unsigned)PICA_RGB565, t.format);
    ASSERT_EQ(16u * 8 * 2, t.data_size);
    ASSERT_EQ(s_len - t.data_size, t.data_offset);
    PASS();
}

/* The fixture generator (tools/make_fixture.py) draws the same picture independently. */
TEST decodes_the_independent_fixture(void)
{
    CgfxTexture t;
    uint8_t rgba[16 * 8 * 4];
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(s_buf, s_len, &t));
    ASSERT_EQ(CGFX_OK, cgfx_decode(s_buf, s_len, &t, rgba));
    for (unsigned y = 0; y < 8; y++) {
        for (unsigned x = 0; x < 16; x++) {
            const uint8_t *p = rgba + ((size_t)y * 16 + x) * 4;
            unsigned r5 = x % 32, g6 = y * 4 % 64, b5 = (x + y) % 32;
            ASSERT_EQ_FMT((r5 << 3) | (r5 >> 2), (unsigned)p[0], "%u");
            ASSERT_EQ_FMT((g6 << 2) | (g6 >> 4), (unsigned)p[1], "%u");
            ASSERT_EQ_FMT((b5 << 3) | (b5 >> 2), (unsigned)p[2], "%u");
            ASSERT_EQ(255u, p[3]);
        }
    }
    PASS();
}

TEST rejects_other_files(void)
{
    CgfxTexture t;
    ASSERT_EQ(CGFX_ERR_MAGIC, cgfx_find_texture(NULL, 0, &t));
    ASSERT_EQ(CGFX_ERR_MAGIC, cgfx_find_texture((const uint8_t *)"PNEL....", 8, &t));
    s_buf[0x24] = 'X'; /* TXOB gone */
    ASSERT_EQ(CGFX_ERR_NO_TEXTURE, cgfx_find_texture(s_buf, s_len, &t));
    PASS();
}

TEST rejects_layouts_that_do_not_fit(void)
{
    CgfxTexture t;
    put32(0x20 + CGFX_TXOB_FORMAT, 99);
    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_find_texture(s_buf, s_len, &t));
    put32(0x20 + CGFX_TXOB_FORMAT, PICA_HILO8);
    ASSERT_EQ(CGFX_ERR_FORMAT, cgfx_find_texture(s_buf, s_len, &t));
    put32(0x20 + CGFX_TXOB_FORMAT, PICA_RGB565);

    put32(0x20 + CGFX_TXOB_WIDTH, 12); /* not a multiple of 8 */
    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_find_texture(s_buf, s_len, &t));
    put32(0x20 + CGFX_TXOB_WIDTH, 16);

    put32(0x20 + CGFX_TXOB_DATA_SIZE, 16 * 8 * 2 - 2); /* smaller than the picture */
    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_find_texture(s_buf, s_len, &t));
    put32(0x20 + CGFX_TXOB_DATA_SIZE, 16 * 8 * 2);

    put32(0x20 + CGFX_TXOB_DATA_OFFSET, 0x1000); /* points past the end */
    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_find_texture(s_buf, s_len, &t));
    put32(0x20 + CGFX_TXOB_DATA_OFFSET, 0xFFFFFFF0u); /* would wrap around */
    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_find_texture(s_buf, s_len, &t));

    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_find_texture(s_buf, 0x20 + 8, &t)); /* cut inside the object */
    PASS();
}

TEST picture_rect(void)
{
    CgfxTexture t = {.width = 512, .height = 256};
    CgfxRect r = cgfx_picture_rect(&t);
    ASSERT_EQ(0u, r.x);
    ASSERT_EQ(16u, r.y);
    ASSERT_EQ(400u, r.w);
    ASSERT_EQ(240u, r.h);
    t.width = 16, t.height = 8; /* the small test picture is used whole */
    r = cgfx_picture_rect(&t);
    ASSERT_EQ(16u, r.w);
    ASSERT_EQ(8u, r.h);
    ASSERT_EQ(0u, r.y);
    PASS();
}

TEST decode_checks_range(void)
{
    CgfxTexture t;
    uint8_t rgba[16 * 8 * 4];
    ASSERT_EQ(CGFX_OK, cgfx_find_texture(s_buf, s_len, &t));
    ASSERT_EQ(CGFX_ERR_LAYOUT, cgfx_decode(s_buf, s_len - 1, &t, rgba));
    PASS();
}

TEST bmp_layout(void)
{
    uint8_t rgba[8 * 8 * 4], bmp[54 + 8 * 24];
    memset(rgba, 0, sizeof(rgba));
    rgba[0] = 1, rgba[1] = 2, rgba[2] = 3, rgba[3] = 9; /* top-left */
    rgba[(7 * 8 + 7) * 4] = 7;                          /* bottom-right red */
    ASSERT_EQ(sizeof(bmp), bmp_size(8, 8));
    ASSERT_EQ(sizeof(bmp), bmp_encode(8, 8, rgba, bmp, sizeof(bmp)));
    ASSERT_EQ(0u, bmp_encode(8, 8, rgba, bmp, sizeof(bmp) - 1));
    ASSERT_EQ(0u, bmp_size(0, 8));
    ASSERT_MEM_EQ("BM", bmp, 2);
    ASSERT_EQ(8, bmp[18]);
    ASSERT_EQ(8, bmp[22]);
    ASSERT_EQ(24, bmp[28]);
    /* Rows run bottom first and pixels are B, G, R. */
    ASSERT_EQ(7, bmp[54 + 7 * 3 + 2]);
    ASSERT_EQ(3, bmp[54 + 24 * 7 + 0]);
    ASSERT_EQ(2, bmp[54 + 24 * 7 + 1]);
    ASSERT_EQ(1, bmp[54 + 24 * 7 + 2]);
    PASS();
}

SUITE(cgfx_suite)
{
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(finds_texture);
    RUN_TEST(decodes_the_independent_fixture);
    RUN_TEST(rejects_other_files);
    RUN_TEST(rejects_layouts_that_do_not_fit);
    RUN_TEST(picture_rect);
    RUN_TEST(decode_checks_range);
    RUN_TEST(bmp_layout);
}
