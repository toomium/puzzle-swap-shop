#ifndef PSS_CORE_PNEL_H
#define PSS_CORE_PNEL_H

#include <stddef.h>
#include <stdint.h>

/*
 * PNEL: SpotPass puzzle container.
 * Read-only. All integers are little endian.
 */

#define PNEL_HEADER_SIZE 0x14u
#define PNEL_ENTRY_SIZE  0xC30u
#define PNEL_NAME_OFFSET 0x2Cu
#define PNEL_NAME_SIZE   0xC00u
/* Panel 46: 16 UTF-16LE fields of 0xC0 bytes. Slot 0 is Japanese, 1..4 are the
 * Americas editions (EN, FR, ES, PT), 5..9 the European ones (EN, FR, ES, DE, IT); slots
 * 10..15 hold English on that console. Callers pass a CFG_Language code, see slot_for_lang. */
#define PNEL_NAME_LANGS  16u
#define PNEL_NAME_STRIDE (PNEL_NAME_SIZE / PNEL_NAME_LANGS)

typedef enum {
    PNEL_OK = 0,
    PNEL_ERR_SIZE,  /* buffer too short for what the header promises */
    PNEL_ERR_MAGIC, /* not a PNEL file */
    PNEL_ERR_CRC,   /* checksum mismatch */
    PNEL_ERR_INDEX, /* entry index out of range */
    PNEL_ERR_RANGE, /* a data block points outside the buffer */
    PNEL_ERR_LANG,  /* unknown language index */
} PnelResult;

typedef struct {
    uint32_t count;
} PnelHeader;

typedef struct {
    uint32_t order; /* panel id; the first SpotPass panel is 7 */
    uint32_t image_offset, settings_offset, model_offset;
    uint32_t image_size, settings_size, model_size;
    uint32_t image_crc, settings_crc, model_crc;
    uint8_t piece_count_id; /* see pnel_piece_count() */
    uint8_t pink;           /* rare pieces; format still open */
} PnelEntry;

/* Checks magic, header CRC and that all entries fit into the buffer. */
PnelResult pnel_read_header(const uint8_t *buf, size_t len, PnelHeader *out);

/* Checks the entry CRC and that all three data blocks lie within the buffer. */
PnelResult pnel_read_entry(const uint8_t *buf, size_t len, uint32_t index, PnelEntry *out);

/* Checks the CRCs of image, settings and model data of an entry. */
PnelResult pnel_verify_blocks(const uint8_t *buf, size_t len, const PnelEntry *entry);

/* Puzzle name as UTF-8, truncated at a character boundary to fit out_size. */
PnelResult pnel_entry_name(const uint8_t *buf, size_t len, uint32_t index, unsigned lang, char *out,
                           size_t out_size);

/*
 * The same checks for a file that is not held in memory as a whole (the file
 * on the console can be several MB). pnel_read_head needs the first
 * PNEL_HEADER_SIZE bytes and the file's size; pnel_read_entry_at needs the
 * PNEL_ENTRY_SIZE bytes of one entry and checks its blocks against file_len;
 * pnel_entry_name_at needs the same bytes.
 */
PnelResult pnel_read_head(const uint8_t *head, size_t head_len, size_t file_len, PnelHeader *out);
PnelResult pnel_read_entry_at(const uint8_t *entry, size_t file_len, PnelEntry *out);
PnelResult pnel_entry_name_at(const uint8_t *entry, unsigned lang, char *out, size_t out_size);

/* 0 -> 15, 1 -> 24, 2 -> 40, 3 -> 540; 0 for unknown ids. */
unsigned pnel_piece_count(uint8_t piece_count_id);

#endif
