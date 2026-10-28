#include "core/cgfx.h"

#include <string.h>

#include "core/pica_tex.h"

static uint32_t read_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

CgfxResult cgfx_find_texture(const uint8_t *buf, size_t len, CgfxTexture *out)
{
    if (buf == NULL || len < 8 || memcmp(buf, "CGFX", 4) != 0)
        return CGFX_ERR_MAGIC;

    /* The texture object's magic sits in its second word; look for the first one. */
    size_t at = 0;
    int found = 0;
    for (size_t p = 4; p + 4 <= len; p += 4) {
        if (memcmp(buf + p, "TXOB", 4) == 0) {
            at = p - 4;
            found = 1;
            break;
        }
    }
    if (!found)
        return CGFX_ERR_NO_TEXTURE;
    if (at + CGFX_TXOB_MIN_SIZE > len)
        return CGFX_ERR_LAYOUT;

    const uint8_t *o = buf + at;
    CgfxTexture t;
    t.object_offset = (uint32_t)at;
    t.height = read_u32le(o + CGFX_TXOB_HEIGHT);
    t.width = read_u32le(o + CGFX_TXOB_WIDTH);
    t.format = read_u32le(o + CGFX_TXOB_FORMAT);
    t.data_size = read_u32le(o + CGFX_TXOB_DATA_SIZE);
    uint64_t data_at = (uint64_t)at + CGFX_TXOB_DATA_OFFSET + read_u32le(o + CGFX_TXOB_DATA_OFFSET);

    if (t.format >= PICA_FORMAT_COUNT)
        return CGFX_ERR_LAYOUT;
    if (t.format == PICA_HILO8)
        return CGFX_ERR_FORMAT;
    size_t need = pica_tex_size(t.format, t.width, t.height);
    if (need == 0)
        return CGFX_ERR_LAYOUT;
    if (t.data_size < need || data_at > len || t.data_size > len - data_at)
        return CGFX_ERR_LAYOUT;
    t.data_offset = (uint32_t)data_at;
    *out = t;
    return CGFX_OK;
}

CgfxResult cgfx_decode(const uint8_t *buf, size_t len, const CgfxTexture *tex, uint8_t *rgba)
{
    if ((uint64_t)tex->data_offset + tex->data_size > len)
        return CGFX_ERR_LAYOUT;
    if (!pica_tex_decode(tex->format, tex->width, tex->height, buf + tex->data_offset,
                         tex->data_size, rgba))
        return CGFX_ERR_FORMAT;
    return CGFX_OK;
}

CgfxRect cgfx_picture_rect(const CgfxTexture *tex)
{
    if (tex->width < CGFX_PICTURE_W || tex->height < CGFX_PICTURE_H)
        return (CgfxRect){0, 0, tex->width, tex->height};
    return (CgfxRect){0, tex->height - CGFX_PICTURE_H, CGFX_PICTURE_W, CGFX_PICTURE_H};
}
