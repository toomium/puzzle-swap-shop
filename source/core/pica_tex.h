#ifndef PSS_CORE_PICA_TEX_H
#define PSS_CORE_PICA_TEX_H

#include <stddef.h>
#include <stdint.h>

/*
 * Textures in the 3DS GPU's own layout (3dbrew, "GPU/Texture formats"): 8x8
 * tiles, inside a tile in Z order. The rows run top to bottom, also on the
 * console (encoding them bottom first showed the picture upside down).
 * Pure logic with host
 * tests; ui/art_source.c uploads the result.
 */

typedef enum {
    PICA_RGBA8 = 0,
    PICA_RGB8,
    PICA_RGBA5551,
    PICA_RGB565,
    PICA_RGBA4,
    PICA_LA8,
    PICA_HILO8, /* not supported */
    PICA_L8,
    PICA_A8,
    PICA_LA4,
    PICA_L4,
    PICA_A4,
    PICA_ETC1,
    PICA_ETC1A4,
    PICA_FORMAT_COUNT,
} PicaFormat;

#define PICA_TEX_MAX_SIDE 1024u

/* Bytes of the first level; 0 for an unsupported format or a size that is not a multiple of 8. */
size_t pica_tex_size(unsigned format, unsigned width, unsigned height);

/*
 * Decodes the first level to RGBA, 4 bytes per pixel in the order R, G, B, A,
 * first row of the data first. Returns 0 if the format is unsupported, the size is not a
 * multiple of 8 or larger than PICA_TEX_MAX_SIDE, or `len` is too short.
 * `rgba` needs width * height * 4 bytes.
 */
int pica_tex_decode(unsigned format, unsigned width, unsigned height, const uint8_t *data,
                    size_t len, uint8_t *rgba);

/*
 * Packs top-down RGBA into what the GPU reads as GPU_RGBA8 for citro2d: last
 * row first (the GPU counts rows from the bottom), tiled and Z-ordered, 4 bytes per pixel in the
 * order A, B, G, R. `out` needs width * height * 4 bytes. Width and height must be multiples of 8.
 */
int pica_tex_encode_rgba8(unsigned width, unsigned height, const uint8_t *rgba, uint8_t *out);

#endif
