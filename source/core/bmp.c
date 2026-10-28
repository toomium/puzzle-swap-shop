#include "core/bmp.h"

#define BMP_HEADER 54u
#define BMP_MAX    4096u

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

size_t bmp_size(unsigned width, unsigned height)
{
    if (width == 0 || height == 0 || width > BMP_MAX || height > BMP_MAX)
        return 0;
    size_t row = ((size_t)width * 3 + 3) & ~(size_t)3;
    return BMP_HEADER + row * height;
}

size_t bmp_encode(unsigned width, unsigned height, const uint8_t *rgba, uint8_t *out, size_t cap)
{
    size_t total = bmp_size(width, height);
    if (total == 0 || cap < total)
        return 0;
    size_t row = ((size_t)width * 3 + 3) & ~(size_t)3;

    for (size_t i = 0; i < BMP_HEADER; i++)
        out[i] = 0;
    out[0] = 'B';
    out[1] = 'M';
    put32(out + 2, (uint32_t)total);
    put32(out + 10, BMP_HEADER);
    put32(out + 14, 40);
    put32(out + 18, width);
    put32(out + 22, height); /* positive: rows are stored bottom first */
    out[26] = 1;
    out[28] = 24;
    put32(out + 34, (uint32_t)(total - BMP_HEADER));

    for (unsigned y = 0; y < height; y++) {
        uint8_t *d = out + BMP_HEADER + row * (height - 1 - y);
        const uint8_t *s = rgba + (size_t)y * width * 4;
        for (unsigned x = 0; x < width; x++) {
            d[x * 3] = s[x * 4 + 2];
            d[x * 3 + 1] = s[x * 4 + 1];
            d[x * 3 + 2] = s[x * 4];
        }
        for (size_t k = (size_t)width * 3; k < row; k++)
            d[k] = 0;
    }
    return total;
}
