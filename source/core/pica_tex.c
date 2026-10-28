#include "core/pica_tex.h"

#include <string.h>

static unsigned bits_per_pixel(unsigned format)
{
    switch (format) {
    case PICA_RGBA8:
        return 32;
    case PICA_RGB8:
        return 24;
    case PICA_RGBA5551:
    case PICA_RGB565:
    case PICA_RGBA4:
    case PICA_LA8:
        return 16;
    case PICA_L8:
    case PICA_A8:
    case PICA_LA4:
    case PICA_ETC1A4:
        return 8;
    case PICA_L4:
    case PICA_A4:
    case PICA_ETC1:
        return 4;
    default:
        return 0;
    }
}

static int dims_ok(unsigned w, unsigned h)
{
    return w >= 8 && h >= 8 && w <= PICA_TEX_MAX_SIDE && h <= PICA_TEX_MAX_SIDE && w % 8 == 0 &&
           h % 8 == 0;
}

size_t pica_tex_size(unsigned format, unsigned width, unsigned height)
{
    unsigned bpp = bits_per_pixel(format);
    if (bpp == 0 || !dims_ok(width, height))
        return 0;
    return (size_t)width * height * bpp / 8;
}

/* Position inside an 8x8 tile: the bits of x and y alternate, x lowest. */
static void tile_xy(unsigned i, unsigned *x, unsigned *y)
{
    *x = (i & 1) | ((i >> 1) & 2) | ((i >> 2) & 4);
    *y = ((i >> 1) & 1) | ((i >> 2) & 2) | ((i >> 3) & 4);
}

static uint32_t le16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint8_t ext4(unsigned v)
{
    return (uint8_t)(v * 17);
}

static uint8_t ext5(unsigned v)
{
    return (uint8_t)((v << 3) | (v >> 2));
}

static uint8_t ext6(unsigned v)
{
    return (uint8_t)((v << 2) | (v >> 4));
}

static void put(uint8_t *px, unsigned r, unsigned g, unsigned b, unsigned a)
{
    px[0] = (uint8_t)r;
    px[1] = (uint8_t)g;
    px[2] = (uint8_t)b;
    px[3] = (uint8_t)a;
}

/* One pixel of the formats that store each pixel on its own. `i` is the pixel's index in the data.
 */
static void decode_pixel(unsigned format, const uint8_t *data, size_t i, uint8_t *px)
{
    unsigned v;
    switch (format) {
    case PICA_RGBA8:
        put(px, data[i * 4 + 3], data[i * 4 + 2], data[i * 4 + 1], data[i * 4]);
        break;
    case PICA_RGB8:
        put(px, data[i * 3 + 2], data[i * 3 + 1], data[i * 3], 255);
        break;
    case PICA_RGBA5551:
        v = le16(data + i * 2);
        put(px, ext5(v >> 11), ext5((v >> 6) & 31), ext5((v >> 1) & 31), (v & 1) ? 255 : 0);
        break;
    case PICA_RGB565:
        v = le16(data + i * 2);
        put(px, ext5(v >> 11), ext6((v >> 5) & 63), ext5(v & 31), 255);
        break;
    case PICA_RGBA4:
        v = le16(data + i * 2);
        put(px, ext4(v >> 12), ext4((v >> 8) & 15), ext4((v >> 4) & 15), ext4(v & 15));
        break;
    case PICA_LA8:
        put(px, data[i * 2 + 1], data[i * 2 + 1], data[i * 2 + 1], data[i * 2]);
        break;
    case PICA_L8:
        put(px, data[i], data[i], data[i], 255);
        break;
    case PICA_A8:
        put(px, 255, 255, 255, data[i]);
        break;
    case PICA_LA4:
        v = ext4(data[i] >> 4);
        put(px, v, v, v, ext4(data[i] & 15));
        break;
    case PICA_L4:
        v = ext4((data[i / 2] >> ((i & 1) * 4)) & 15);
        put(px, v, v, v, 255);
        break;
    default: /* PICA_A4 */
        put(px, 255, 255, 255, ext4((data[i / 2] >> ((i & 1) * 4)) & 15));
        break;
    }
}

static uint8_t clamp8(int v)
{
    return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
}

static const int etc1_modifier[8][4] = {
    {2, 8, -2, -8},     {5, 17, -5, -17},   {9, 29, -9, -29},     {13, 42, -13, -42},
    {18, 60, -18, -60}, {24, 80, -24, -80}, {33, 106, -33, -106}, {47, 183, -47, -183},
};

/*
 * One 4x4 block. The GPU stores the 8 bytes of the standard ETC1 block
 * (big endian) in reverse order, so byte 7 here is byte 0 of the standard.
 * out is the top-left pixel of the block in a width-wide RGBA image.
 */
static void decode_etc1_block(const uint8_t *raw, const uint8_t *alpha, uint8_t *out,
                              unsigned width)
{
    uint8_t b[8];
    for (int k = 0; k < 8; k++)
        b[k] = raw[7 - k];

    int base[2][3];
    int diff = (b[3] >> 1) & 1, flip = b[3] & 1;
    if (diff) {
        for (int c = 0; c < 3; c++) {
            int v1 = b[c] >> 3;
            int d = b[c] & 7;
            if (d > 3)
                d -= 8;
            int v2 = v1 + d;
            if (v2 < 0)
                v2 = 0;
            if (v2 > 31)
                v2 = 31;
            base[0][c] = ext5((unsigned)v1);
            base[1][c] = ext5((unsigned)v2);
        }
    } else {
        for (int c = 0; c < 3; c++) {
            base[0][c] = ext4((unsigned)b[c] >> 4);
            base[1][c] = ext4((unsigned)b[c] & 15);
        }
    }
    int table[2] = {(b[3] >> 5) & 7, (b[3] >> 2) & 7};
    unsigned msb = ((unsigned)b[4] << 8) | b[5];
    unsigned lsb = ((unsigned)b[6] << 8) | b[7];

    for (unsigned i = 0; i < 16; i++) {
        unsigned x = i >> 2, y = i & 3; /* pixels run down the columns */
        int sub = flip ? (y >= 2) : (x >= 2);
        int mod = etc1_modifier[table[sub]][(((msb >> i) & 1) << 1) | ((lsb >> i) & 1)];
        uint8_t *px = out + ((size_t)y * width + x) * 4;
        put(px, clamp8(base[sub][0] + mod), clamp8(base[sub][1] + mod), clamp8(base[sub][2] + mod),
            255);
        if (alpha != NULL)
            px[3] = ext4((alpha[(x * 4 + y) / 2] >> (((x * 4 + y) & 1) * 4)) & 15);
    }
}

int pica_tex_decode(unsigned format, unsigned width, unsigned height, const uint8_t *data,
                    size_t len, uint8_t *rgba)
{
    size_t need = pica_tex_size(format, width, height);
    if (need == 0 || len < need)
        return 0;

    unsigned tiles_x = width / 8, tiles_y = height / 8;
    size_t tile_index = 0;
    for (unsigned ty = 0; ty < tiles_y; ty++) {
        for (unsigned tx = 0; tx < tiles_x; tx++, tile_index++) {
            /*
             * Rows of the data run top to bottom, as on the real picture of
             * panel 7 (decoding bottom first gave an upside-down image).
             */
            unsigned out_y0 = ty * 8;
            if (format == PICA_ETC1 || format == PICA_ETC1A4) {
                size_t block_bytes = format == PICA_ETC1 ? 8 : 16;
                const uint8_t *tile = data + tile_index * 4 * block_bytes;
                for (unsigned t = 0; t < 4; t++) {
                    const uint8_t *blk = tile + t * block_bytes;
                    unsigned bx = (t & 1) * 4, by = (t >> 1) * 4;
                    uint8_t block[16 * 4];
                    decode_etc1_block(format == PICA_ETC1 ? blk : blk + 8,
                                      format == PICA_ETC1 ? NULL : blk, block, 4);
                    for (unsigned y = 0; y < 4; y++)
                        memcpy(rgba + ((size_t)(out_y0 + by + y) * width + tx * 8 + bx) * 4,
                               block + y * 16, 16);
                }
            } else {
                for (unsigned i = 0; i < 64; i++) {
                    unsigned x, y;
                    tile_xy(i, &x, &y);
                    decode_pixel(format, data, tile_index * 64 + i,
                                 rgba + ((size_t)(out_y0 + y) * width + tx * 8 + x) * 4);
                }
            }
        }
    }
    return 1;
}

int pica_tex_encode_rgba8(unsigned width, unsigned height, const uint8_t *rgba, uint8_t *out)
{
    if (!dims_ok(width, height))
        return 0;
    size_t tile_index = 0;
    for (unsigned ty = 0; ty < height / 8; ty++) {
        for (unsigned tx = 0; tx < width / 8; tx++, tile_index++) {
            unsigned src_y0 = ty * 8;
            for (unsigned i = 0; i < 64; i++) {
                unsigned x, y;
                tile_xy(i, &x, &y);
                const uint8_t *s = rgba + ((size_t)(src_y0 + y) * width + tx * 8 + x) * 4;
                uint8_t *d = out + (tile_index * 64 + i) * 4;
                d[0] = s[3];
                d[1] = s[2];
                d[2] = s[1];
                d[3] = s[0];
            }
        }
    }
    return 1;
}
