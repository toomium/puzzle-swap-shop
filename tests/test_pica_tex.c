#include <string.h>

#include "greatest.h"
#include "core/pica_tex.h"

static const uint8_t *px(const uint8_t *rgba, unsigned w, unsigned x, unsigned y)
{
    return rgba + ((size_t)y * w + x) * 4;
}

#define ASSERT_PIXEL(rgba, w, x, y, r, g, b, a)                                                    \
    do {                                                                                           \
        const uint8_t *p_ = px((rgba), (w), (x), (y));                                             \
        ASSERT_EQ_FMT((unsigned)(r), (unsigned)p_[0], "%u");                                       \
        ASSERT_EQ_FMT((unsigned)(g), (unsigned)p_[1], "%u");                                       \
        ASSERT_EQ_FMT((unsigned)(b), (unsigned)p_[2], "%u");                                       \
        ASSERT_EQ_FMT((unsigned)(a), (unsigned)p_[3], "%u");                                       \
    } while (0)

TEST sizes(void)
{
    ASSERT_EQ(512u * 256 * 4, pica_tex_size(PICA_RGBA8, 512, 256));
    ASSERT_EQ(512u * 256 * 2, pica_tex_size(PICA_RGB565, 512, 256));
    ASSERT_EQ(512u * 256 / 2, pica_tex_size(PICA_ETC1, 512, 256));
    ASSERT_EQ(512u * 256, pica_tex_size(PICA_ETC1A4, 512, 256));
    ASSERT_EQ(32u, pica_tex_size(PICA_L4, 8, 8));
    ASSERT_EQ(0u, pica_tex_size(PICA_HILO8, 8, 8));
    ASSERT_EQ(0u, pica_tex_size(PICA_FORMAT_COUNT, 8, 8));
    ASSERT_EQ(0u, pica_tex_size(PICA_RGBA8, 12, 8)); /* not a multiple of 8 */
    ASSERT_EQ(0u, pica_tex_size(PICA_RGBA8, 0, 8));
    ASSERT_EQ(0u, pica_tex_size(PICA_RGBA8, 2048, 8));
    PASS();
}

TEST rejects_short_data_and_bad_format(void)
{
    uint8_t data[128] = {0}, rgba[8 * 8 * 4];
    ASSERT_EQ(0, pica_tex_decode(PICA_RGB565, 8, 8, data, 127, rgba));
    ASSERT_EQ(1, pica_tex_decode(PICA_RGB565, 8, 8, data, 128, rgba));
    ASSERT_EQ(0, pica_tex_decode(PICA_HILO8, 8, 8, data, 128, rgba));
    ASSERT_EQ(0, pica_tex_decode(PICA_RGB565, 9, 8, data, 128, rgba));
    PASS();
}

/* The first byte of the data is the lowest row's first pixel; the Z order puts pixel 1 to its
 * right. */
TEST pixel_positions_in_tile(void)
{
    uint8_t data[8 * 8], rgba[8 * 8 * 4];
    memset(data, 0, sizeof(data));
    data[0] = 10;  /* i = 0: x 0, y 0 */
    data[1] = 20;  /* i = 1: x 1, y 0 */
    data[2] = 30;  /* i = 2: x 0, y 1 */
    data[4] = 40;  /* i = 4: x 2, y 0 */
    data[8] = 50;  /* i = 8: x 0, y 2 */
    data[16] = 60; /* i = 16: x 4, y 0 */
    data[32] = 70; /* i = 32: x 0, y 4 */
    data[63] = 80; /* i = 63: x 7, y 7 */
    ASSERT_EQ(1, pica_tex_decode(PICA_L8, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 10, 10, 10, 255);
    ASSERT_PIXEL(rgba, 8, 1, 0, 20, 20, 20, 255);
    ASSERT_PIXEL(rgba, 8, 0, 1, 30, 30, 30, 255);
    ASSERT_PIXEL(rgba, 8, 2, 0, 40, 40, 40, 255);
    ASSERT_PIXEL(rgba, 8, 0, 2, 50, 50, 50, 255);
    ASSERT_PIXEL(rgba, 8, 4, 0, 60, 60, 60, 255);
    ASSERT_PIXEL(rgba, 8, 0, 4, 70, 70, 70, 255);
    ASSERT_PIXEL(rgba, 8, 7, 7, 80, 80, 80, 255);
    PASS();
}

TEST tiles_run_from_the_top(void)
{
    /* 16 x 16 L8: tile 0 top-left, 1 top-right, 2 bottom-left, 3 bottom-right. */
    uint8_t data[16 * 16], rgba[16 * 16 * 4];
    memset(data, 0, sizeof(data));
    for (unsigned t = 0; t < 4; t++)
        data[t * 64] = (uint8_t)(100 + t);
    ASSERT_EQ(1, pica_tex_decode(PICA_L8, 16, 16, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 16, 0, 0, 100, 100, 100, 255);
    ASSERT_PIXEL(rgba, 16, 8, 0, 101, 101, 101, 255);
    ASSERT_PIXEL(rgba, 16, 0, 8, 102, 102, 102, 255);
    ASSERT_PIXEL(rgba, 16, 8, 8, 103, 103, 103, 255);
    PASS();
}

TEST simple_formats(void)
{
    uint8_t data[8 * 8 * 4], rgba[8 * 8 * 4];
    memset(data, 0, sizeof(data));

    /* RGBA8 is stored A, B, G, R. */
    data[0] = 4, data[1] = 3, data[2] = 2, data[3] = 1;
    ASSERT_EQ(1, pica_tex_decode(PICA_RGBA8, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 1, 2, 3, 4);

    memset(data, 0, sizeof(data));
    data[0] = 3, data[1] = 2, data[2] = 1; /* RGB8: B, G, R */
    ASSERT_EQ(1, pica_tex_decode(PICA_RGB8, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 1, 2, 3, 255);

    memset(data, 0, sizeof(data));
    data[0] = 0x00, data[1] = 0xF8; /* RGB565 0xF800: red */
    data[2] = 0xE0, data[3] = 0x07; /* 0x07E0: green */
    ASSERT_EQ(1, pica_tex_decode(PICA_RGB565, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 255, 0, 0, 255);
    ASSERT_PIXEL(rgba, 8, 1, 0, 0, 255, 0, 255);

    memset(data, 0, sizeof(data));
    data[0] = 0x3F, data[1] = 0x00; /* RGBA5551: B 31 (bits 1-5), A 1 */
    data[0] = 0x3F;
    ASSERT_EQ(1, pica_tex_decode(PICA_RGBA5551, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 0, 0, 255, 255);

    memset(data, 0, sizeof(data));
    data[0] = 0xF1, data[1] = 0x8F; /* RGBA4 0x8FF1: R 8, G 15, B 15, A 1 */
    ASSERT_EQ(1, pica_tex_decode(PICA_RGBA4, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 8 * 17, 255, 255, 17);

    memset(data, 0, sizeof(data));
    data[0] = 0x40, data[1] = 0x90; /* LA8: alpha first, then luminance */
    ASSERT_EQ(1, pica_tex_decode(PICA_LA8, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 0x90, 0x90, 0x90, 0x40);

    memset(data, 0, sizeof(data));
    data[0] = 77;
    ASSERT_EQ(1, pica_tex_decode(PICA_A8, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 255, 255, 255, 77);

    memset(data, 0, sizeof(data));
    data[0] = 0xA5; /* LA4: luminance in the high nibble */
    ASSERT_EQ(1, pica_tex_decode(PICA_LA4, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 170, 170, 170, 85);

    memset(data, 0, sizeof(data));
    data[0] = 0xA5; /* L4: two pixels per byte, low nibble first */
    ASSERT_EQ(1, pica_tex_decode(PICA_L4, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 85, 85, 85, 255);
    ASSERT_PIXEL(rgba, 8, 1, 0, 170, 170, 170, 255);

    ASSERT_EQ(1, pica_tex_decode(PICA_A4, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 255, 255, 255, 85);
    ASSERT_PIXEL(rgba, 8, 1, 0, 255, 255, 255, 170);
    PASS();
}

/* Standard ETC1 block: individual mode, no flip; see the comments at the asserts. */
static const uint8_t etc1_block[8] = {0x8F, 0x00, 0x00, 0x04, 0x20, 0x01, 0x00, 0x01};

static void put_reversed(uint8_t *dst, const uint8_t *standard)
{
    for (int i = 0; i < 8; i++)
        dst[i] = standard[7 - i];
}

TEST etc1_block_decodes(void)
{
    uint8_t data[32], rgba[8 * 8 * 4];
    memset(data, 0, sizeof(data));
    put_reversed(data, etc1_block); /* first block of the first tile: the top-left 4x4 */
    ASSERT_EQ(1, pica_tex_decode(PICA_ETC1, 8, 8, data, sizeof(data), rgba));
    /* Left half, table 0, index 0: base (0x88, 0, 0) + 2. */
    ASSERT_PIXEL(rgba, 8, 1, 0, 0x8A, 2, 2, 255);
    /* Right half, table 1, index 0: base (0xFF, 0, 0) + 5, clamped. */
    ASSERT_PIXEL(rgba, 8, 2, 0, 255, 5, 5, 255);
    /* Pixel (0,0) of the block has index 3: -8 with table 0. */
    ASSERT_PIXEL(rgba, 8, 0, 0, 0x80, 0, 0, 255);
    /* Pixel (3,1) has index 2: -5 with table 1; block row 1 lies one row above row 0. */
    ASSERT_PIXEL(rgba, 8, 3, 1, 250, 0, 0, 255);
    /* The other blocks are all zero: 0 + 2 on every channel. */
    ASSERT_PIXEL(rgba, 8, 5, 0, 2, 2, 2, 255);
    ASSERT_PIXEL(rgba, 8, 0, 5, 2, 2, 2, 255);
    PASS();
}

TEST etc1_differential_and_flip(void)
{
    /* diff = 1, flip = 1, base (8,8,8)/32 with offsets (+1,0,-1), tables 0 and 7. */
    const uint8_t standard[8] = {
        (8 << 3) | 1, (8 << 3) | 0, (8 << 3) | 7, (0 << 5) | (7 << 2) | 3, 0, 0, 0, 0};
    uint8_t data[32], rgba[8 * 8 * 4];
    memset(data, 0, sizeof(data));
    put_reversed(data, standard);
    ASSERT_EQ(1, pica_tex_decode(PICA_ETC1, 8, 8, data, sizeof(data), rgba));
    /* 5-bit 8 -> 66, 9 -> 74, 7 -> 57. Top half (block rows 0 and 1) uses table 0 (+2). */
    ASSERT_PIXEL(rgba, 8, 0, 0, 68, 68, 68, 255);
    ASSERT_PIXEL(rgba, 8, 3, 1, 68, 68, 68, 255);
    /* Bottom half (rows 2 and 3) uses base 2 = (9, 8, 7) with table 7 (+47). */
    ASSERT_PIXEL(rgba, 8, 0, 2, 74 + 47, 66 + 47, 57 + 47, 255);
    ASSERT_PIXEL(rgba, 8, 3, 3, 74 + 47, 66 + 47, 57 + 47, 255);
    PASS();
}

TEST etc1a4_alpha(void)
{
    uint8_t data[64], rgba[8 * 8 * 4];
    memset(data, 0, sizeof(data));
    data[0] = 0x0A; /* nibble 0: pixel (0,0) */
    data[1] = 0xC0; /* nibble 3: pixel x 0, y 3 */
    data[6] = 0x50; /* nibble 13: pixel x 3, y 1 */
    put_reversed(data + 8, etc1_block);
    ASSERT_EQ(1, pica_tex_decode(PICA_ETC1A4, 8, 8, data, sizeof(data), rgba));
    ASSERT_PIXEL(rgba, 8, 0, 0, 0x80, 0, 0, 170);
    ASSERT_PIXEL(rgba, 8, 0, 3, 0x8A, 2, 2, 204);
    ASSERT_PIXEL(rgba, 8, 3, 1, 250, 0, 0, 85);
    ASSERT_PIXEL(rgba, 8, 1, 0, 0x8A, 2, 2, 0);
    PASS();
}

/* Rows run top first, so decoding what encode wrote gives the picture back. */
TEST encode_roundtrips(void)
{
    enum { W = 16, H = 24 };
    uint8_t img[W * H * 4], gpu[W * H * 4], back[W * H * 4];
    for (unsigned y = 0; y < H; y++)
        for (unsigned x = 0; x < W; x++) {
            uint8_t *p = img + ((size_t)y * W + x) * 4;
            p[0] = (uint8_t)(x * 10), p[1] = (uint8_t)(y * 10), p[2] = (uint8_t)(x ^ y), p[3] = 200;
        }
    ASSERT_EQ(1, pica_tex_encode_rgba8(W, H, img, gpu));
    /* Byte order and position of the first pixel: the top-left pixel is the first of
     * tile (0, 0), number 0 in Z order. */
    size_t first = 0;
    ASSERT_EQ(img[3], gpu[first + 0]);
    ASSERT_EQ(img[2], gpu[first + 1]);
    ASSERT_EQ(img[1], gpu[first + 2]);
    ASSERT_EQ(img[0], gpu[first + 3]);
    ASSERT_EQ(1, pica_tex_decode(PICA_RGBA8, W, H, gpu, sizeof(gpu), back));
    for (unsigned y = 0; y < H; y++)
        ASSERT_MEM_EQ(img + (size_t)y * W * 4, back + (size_t)y * W * 4, W * 4);
    ASSERT_EQ(0, pica_tex_encode_rgba8(W, 12, img, gpu));
    PASS();
}

SUITE(pica_tex_suite)
{
    RUN_TEST(sizes);
    RUN_TEST(rejects_short_data_and_bad_format);
    RUN_TEST(pixel_positions_in_tile);
    RUN_TEST(tiles_run_from_the_top);
    RUN_TEST(simple_formats);
    RUN_TEST(etc1_block_decodes);
    RUN_TEST(etc1_differential_and_flip);
    RUN_TEST(etc1a4_alpha);
    RUN_TEST(encode_roundtrips);
}
