#ifndef PSS_CORE_RARE_PIECES_H
#define PSS_CORE_RARE_PIECES_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Rare (pink) pieces of SpotPass panels (for sale at
 * 10 coins). The PNEL entry only has a flag at offset 0xC2D (00 = none, 01 =
 * has some); which pieces they are is not in any file the app reads. The
 * pattern per panel size is a hypothesis from photos, see rare_pieces.c and
 * For the standard panels
 * nothing is known, they have no PNEL entry.
 */

/*
 * Rare pieces of a SpotPass panel from the raw PNEL byte. Bit i of *mask is
 * piece i. Returns false (and sets *mask to 0) if the pattern is not known; then
 * no piece is marked, because rare pieces are marked only when sure.
 */
bool rare_mask_from_pnel(uint8_t pink, unsigned piece_count, uint64_t *mask);

#endif
