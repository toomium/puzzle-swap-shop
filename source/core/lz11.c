#include "core/lz11.h"

/* Header: marker 0x11, then the size in 3 bytes; if that is 0, 4 more bytes follow. */
static size_t read_header(const uint8_t *src, size_t len, size_t *size, size_t *start)
{
    if (src == NULL || len < 4 || src[0] != 0x11)
        return 0;
    size_t n = (size_t)src[1] | ((size_t)src[2] << 8) | ((size_t)src[3] << 16);
    *start = 4;
    if (n == 0) {
        if (len < 8)
            return 0;
        n = (size_t)src[4] | ((size_t)src[5] << 8) | ((size_t)src[6] << 16) |
            ((size_t)src[7] << 24);
        *start = 8;
    }
    *size = n;
    return n;
}

int lz11_is_packed(const uint8_t *src, size_t len)
{
    size_t size, start;
    return read_header(src, len, &size, &start) != 0;
}

size_t lz11_unpacked_size(const uint8_t *src, size_t len)
{
    size_t size, start;
    return read_header(src, len, &size, &start);
}

size_t lz11_unpack(const uint8_t *src, size_t len, uint8_t *dst, size_t cap)
{
    size_t size, in;
    if (read_header(src, len, &size, &in) == 0 || size > cap)
        return 0;

    size_t out = 0;
    unsigned flags = 0, bits = 0;
    while (out < size) {
        if (bits == 0) {
            if (in >= len)
                return 0;
            flags = src[in++];
            bits = 8;
        }
        int packed = (flags & 0x80) != 0;
        flags <<= 1;
        bits--;
        if (!packed) {
            if (in >= len)
                return 0;
            dst[out++] = src[in++];
            continue;
        }
        if (in >= len)
            return 0;
        unsigned b0 = src[in++];
        size_t count, disp;
        unsigned kind = b0 >> 4;
        if (kind == 0) {
            if (in + 2 > len)
                return 0;
            count = (((size_t)(b0 & 15) << 4) | (src[in] >> 4)) + 0x11;
            disp = (((size_t)(src[in] & 15) << 8) | src[in + 1]) + 1;
            in += 2;
        } else if (kind == 1) {
            if (in + 3 > len)
                return 0;
            count =
                (((size_t)(b0 & 15) << 12) | ((size_t)src[in] << 4) | (src[in + 1] >> 4)) + 0x111;
            disp = (((size_t)(src[in + 1] & 15) << 8) | src[in + 2]) + 1;
            in += 3;
        } else {
            if (in >= len)
                return 0;
            count = kind + 1;
            disp = (((size_t)(b0 & 15) << 8) | src[in]) + 1;
            in++;
        }
        if (disp > out || count > size - out)
            return 0;
        for (size_t i = 0; i < count; i++, out++)
            dst[out] = dst[out - disp];
    }
    return out;
}
