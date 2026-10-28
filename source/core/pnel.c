#include "core/pnel.h"

#include <string.h>

#include "core/crc32.h"

static uint32_t read_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* CRC over a block with its own 4-byte CRC field treated as zero. */
static uint32_t crc_with_zeroed_field(const uint8_t *block, size_t size, size_t field)
{
    static const uint8_t zero[4] = {0};
    uint32_t crc = crc32_update(0, block, field);
    crc = crc32_update(crc, zero, 4);
    return crc32_update(crc, block + field + 4, size - field - 4);
}

static int block_in_range(uint32_t offset, uint32_t size, size_t len)
{
    return offset <= len && size <= len - offset;
}

PnelResult pnel_read_header(const uint8_t *buf, size_t len, PnelHeader *out)
{
    return pnel_read_head(buf, len, len, out);
}

static PnelResult entry_ptr(const uint8_t *buf, size_t len, uint32_t index, const uint8_t **out)
{
    PnelHeader header;
    PnelResult res = pnel_read_header(buf, len, &header);
    if (res != PNEL_OK)
        return res;
    if (index >= header.count)
        return PNEL_ERR_INDEX;
    *out = buf + PNEL_HEADER_SIZE + (size_t)index * PNEL_ENTRY_SIZE;
    return PNEL_OK;
}

PnelResult pnel_read_entry_at(const uint8_t *e, size_t file_len, PnelEntry *out)
{
    if (crc_with_zeroed_field(e, PNEL_ENTRY_SIZE, 0x04) != read_u32le(e + 0x04))
        return PNEL_ERR_CRC;

    PnelEntry entry = {
        .order = read_u32le(e + 0x00),
        .image_offset = read_u32le(e + 0x08),
        .settings_offset = read_u32le(e + 0x0C),
        .model_offset = read_u32le(e + 0x10),
        .image_size = read_u32le(e + 0x14),
        .settings_size = read_u32le(e + 0x18),
        .model_size = read_u32le(e + 0x1C),
        .image_crc = read_u32le(e + 0x20),
        .settings_crc = read_u32le(e + 0x24),
        .model_crc = read_u32le(e + 0x28),
        .piece_count_id = e[0xC2C],
        .pink = e[0xC2D],
    };
    if (!block_in_range(entry.image_offset, entry.image_size, file_len) ||
        !block_in_range(entry.settings_offset, entry.settings_size, file_len) ||
        !block_in_range(entry.model_offset, entry.model_size, file_len))
        return PNEL_ERR_RANGE;

    *out = entry;
    return PNEL_OK;
}

PnelResult pnel_read_entry(const uint8_t *buf, size_t len, uint32_t index, PnelEntry *out)
{
    const uint8_t *e;
    PnelResult res = entry_ptr(buf, len, index, &e);
    if (res != PNEL_OK)
        return res;
    return pnel_read_entry_at(e, len, out);
}

PnelResult pnel_read_head(const uint8_t *head, size_t head_len, size_t file_len, PnelHeader *out)
{
    if (!head || head_len < PNEL_HEADER_SIZE || file_len < PNEL_HEADER_SIZE)
        return PNEL_ERR_SIZE;
    if (memcmp(head, "PNEL", 4) != 0)
        return PNEL_ERR_MAGIC;
    if (crc_with_zeroed_field(head, PNEL_HEADER_SIZE, 0x10) != read_u32le(head + 0x10))
        return PNEL_ERR_CRC;

    uint32_t count = read_u32le(head + 0x08);
    if (count > (file_len - PNEL_HEADER_SIZE) / PNEL_ENTRY_SIZE)
        return PNEL_ERR_SIZE;
    out->count = count;
    return PNEL_OK;
}

PnelResult pnel_verify_blocks(const uint8_t *buf, size_t len, const PnelEntry *entry)
{
    if (!block_in_range(entry->image_offset, entry->image_size, len) ||
        !block_in_range(entry->settings_offset, entry->settings_size, len) ||
        !block_in_range(entry->model_offset, entry->model_size, len))
        return PNEL_ERR_RANGE;
    if (crc32(buf + entry->image_offset, entry->image_size) != entry->image_crc ||
        crc32(buf + entry->settings_offset, entry->settings_size) != entry->settings_crc ||
        crc32(buf + entry->model_offset, entry->model_size) != entry->model_crc)
        return PNEL_ERR_CRC;
    return PNEL_OK;
}

/* Appends one code point as UTF-8; returns bytes written or 0 if it does not fit. */
static size_t put_utf8(char *out, size_t room, uint32_t cp)
{
    if (cp < 0x80 && room >= 1) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp < 0x800 && room >= 2) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp >= 0x800 && cp < 0x10000 && room >= 3) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    if (cp >= 0x10000 && room >= 4) {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

/* CFG_Language -> name slot. Languages without a slot of their own use English. */
static unsigned slot_for_lang(unsigned lang)
{
    static const uint8_t slot[12] = {0, 5, 6, 8, 9, 7, 5, 5, 5, 4, 5, 5};
    return slot[lang];
}

PnelResult pnel_entry_name_at(const uint8_t *e, unsigned lang, char *out, size_t out_size)
{
    if (!out || out_size == 0)
        return PNEL_ERR_SIZE;
    out[0] = '\0';
    if (lang >= 12)
        return PNEL_ERR_LANG;

    const uint8_t *s = e + PNEL_NAME_OFFSET + slot_for_lang(lang) * PNEL_NAME_STRIDE;
    size_t units = PNEL_NAME_STRIDE / 2;
    size_t pos = 0;
    for (size_t i = 0; i < units; i++) {
        uint32_t cp = (uint32_t)(s[2 * i] | (s[2 * i + 1] << 8));
        if (cp == 0)
            break;
        if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < units) {
            uint32_t lo = (uint32_t)(s[2 * i + 2] | (s[2 * i + 3] << 8));
            if (lo >= 0xDC00 && lo < 0xE000) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i++;
            }
        }
        if (cp >= 0xD800 && cp < 0xE000)
            cp = 0xFFFD; /* lone surrogate */
        size_t n = put_utf8(out + pos, out_size - 1 - pos, cp);
        if (n == 0)
            break;
        pos += n;
    }
    out[pos] = '\0';
    return PNEL_OK;
}

PnelResult pnel_entry_name(const uint8_t *buf, size_t len, uint32_t index, unsigned lang, char *out,
                           size_t out_size)
{
    if (!out || out_size == 0)
        return PNEL_ERR_SIZE;
    out[0] = '\0';
    const uint8_t *e;
    PnelResult res = entry_ptr(buf, len, index, &e);
    if (res != PNEL_OK)
        return res;
    return pnel_entry_name_at(e, lang, out, out_size);
}

unsigned pnel_piece_count(uint8_t piece_count_id)
{
    switch (piece_count_id) {
    case 0:
        return 15;
    case 1:
        return 24;
    case 2:
        return 40;
    case 3:
        return 540;
    default:
        return 0;
    }
}
