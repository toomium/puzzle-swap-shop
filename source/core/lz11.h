#ifndef PSS_CORE_LZ11_H
#define PSS_CORE_LZ11_H

#include <stddef.h>
#include <stdint.h>

/*
 * LZ11, the compression the 3DS uses for many files (3dbrew, "LZ11"). The
 * picture blocks of the PNEL files are packed this way (* the block of panel 7 starts with 11 00 01 01, which unpacks to 0x10100
 * bytes, a 512 x 256 ETC1 texture plus 0x100 bytes of CGFX header).
 */

/* True if the data starts with the LZ11 marker byte 0x11 and a plausible size field. */
int lz11_is_packed(const uint8_t *src, size_t len);

/* Size of the unpacked data from the header; 0 if the header is not LZ11. */
size_t lz11_unpacked_size(const uint8_t *src, size_t len);

/*
 * Unpacks into dst (cap bytes). Returns the unpacked size, or 0 if the data
 * is damaged (read past the end, copy from before the start) or does not fit.
 */
size_t lz11_unpack(const uint8_t *src, size_t len, uint8_t *dst, size_t cap);

#endif
