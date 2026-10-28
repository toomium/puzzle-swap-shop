#ifndef PSS_CORE_CGFX_H
#define PSS_CORE_CGFX_H

#include <stddef.h>
#include <stdint.h>

/*
 * Picture of a SpotPass puzzle: the image block of a PNEL entry is a
 * CGFX file with one texture. Read only, never written.
 *
 * Hypothesis: the layout of the
 * texture object below. It is kept in cgfx_find_texture() and nowhere else.
 * Every field is checked against the file, so a wrong guess ends in
 * CGFX_ERR_LAYOUT and the app draws its placeholder.
 */

typedef enum {
    CGFX_OK = 0,
    CGFX_ERR_MAGIC, /* not a CGFX file */
    CGFX_ERR_NO_TEXTURE,
    CGFX_ERR_LAYOUT, /* texture object does not fit what we expect */
    CGFX_ERR_FORMAT, /* pixel format not supported */
} CgfxResult;

typedef struct {
    uint32_t object_offset; /* start of the texture object (its flags word) */
    uint32_t width, height;
    uint32_t format; /* PicaFormat */
    uint32_t data_offset, data_size;
} CgfxTexture;

/* Offsets inside the texture object, from its first word (hypothesis). */
#define CGFX_TXOB_HEIGHT      0x18u
#define CGFX_TXOB_WIDTH       0x1Cu
#define CGFX_TXOB_FORMAT      0x34u
#define CGFX_TXOB_DATA_SIZE   0x44u
#define CGFX_TXOB_DATA_OFFSET 0x48u /* relative to its own position */
#define CGFX_TXOB_MIN_SIZE    0x4Cu

/*
 * The picture inside the texture. On panel 7 the texture is 512 x 256
 * and the picture of 400 x 240 pixels fills its left side and its lower 240
 * rows (the first 16 rows are black); two columns beyond it are bleed from the
 * compression. Hypothesis for the other panels. Coordinates are in the
 * decoded, top-down image; a texture too small for it is used whole.
 */
#define CGFX_PICTURE_W 400u
#define CGFX_PICTURE_H 240u

typedef struct {
    uint32_t x, y, w, h;
} CgfxRect;

CgfxRect cgfx_picture_rect(const CgfxTexture *tex);

CgfxResult cgfx_find_texture(const uint8_t *buf, size_t len, CgfxTexture *out);

/* Decodes the first level to top-down RGBA (width * height * 4 bytes at `rgba`). */
CgfxResult cgfx_decode(const uint8_t *buf, size_t len, const CgfxTexture *tex, uint8_t *rgba);

#endif
