#ifndef PSS_CORE_MEET_SAVE_H
#define PSS_CORE_MEET_SAVE_H

#include <stddef.h>
#include <stdint.h>

/* meet.dat, the StreetPass Mii Plaza system save. */

#define MEET_SAVE_SIZE           0x60000u
#define MEET_STANDARD_PANELS     0x044BDCu
#define MEET_STANDARD_PANEL_SIZE 0x02u
#define MEET_SPOTPASS_PANELS     0x045578u
#define MEET_SPOTPASS_PANEL_SIZE 0x44u

typedef enum {
    MEET_OK = 0,
    MEET_ERR_SIZE,        /* buffer is not MEET_SAVE_SIZE bytes */
    MEET_ERR_PANEL,       /* unknown panel id */
    MEET_ERR_UNUSED_BITS, /* a bit beyond the piece count is set */
    MEET_ERR_RANGE,       /* requested piece beyond the piece count */
    MEET_ERR_OWNED,       /* requested piece is already owned */
    MEET_ERR_NOT_OWNED,   /* piece to remove is not owned */
} MeetResult;

/* Size check plus: no panel has bits set beyond its piece count. */
MeetResult meet_save_validate(const uint8_t *buf, size_t len);

/* Offset of a panel's bitfield, or 0 for an unknown panel id. */
size_t meet_save_panel_offset(unsigned panel_id);

/* owned: bit i set if piece i is owned. */
MeetResult meet_save_get_owned(const uint8_t *buf, size_t len, unsigned panel_id, uint64_t *owned);

/*
 * Marks the given pieces as owned. Every requested piece must be missing and
 * within range; on error the buffer is unchanged. Touches no other byte.
 */
MeetResult meet_save_add_pieces(uint8_t *buf, size_t len, unsigned panel_id, uint64_t pieces);

/*
 * Marks owned pieces as missing again. Only the diagnosis build uses this, to
 * find out on finished panels which bit belongs to which piece.
 */
MeetResult meet_save_remove_pieces(uint8_t *buf, size_t len, unsigned panel_id, uint64_t pieces);

#endif
