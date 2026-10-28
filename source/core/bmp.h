#ifndef PSS_CORE_BMP_H
#define PSS_CORE_BMP_H

#include <stddef.h>
#include <stdint.h>

/* Uncompressed 24-bit BMP, for the diagnosis build to show decoded pictures on a PC. */

/* Bytes bmp_encode writes for an image of this size; 0 if a side is 0 or larger than 4096. */
size_t bmp_size(unsigned width, unsigned height);

/* rgba: top-down, R G B A, 4 bytes per pixel. Alpha is dropped. Returns bytes written, or 0. */
size_t bmp_encode(unsigned width, unsigned height, const uint8_t *rgba, uint8_t *out, size_t cap);

#endif
