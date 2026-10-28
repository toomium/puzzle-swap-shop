#include "core/rare_pieces.h"

#define PINK_NONE    0
#define PINK_PRESENT 1

/*
 * Confirmed on a console by hand: the pink pieces of a 40-piece panel with the
 * flag 01 are always these eight (row by row from the top left, 8 columns),
 * read off two panels with different pictures (Chibi-Robo, Captain Toad).
 * The pattern is not in the PNEL file; Mii Plaza seems to hold it itself.
 */
#define PINK_MASK_40                                                                               \
    ((1ull << 11) | (1ull << 12) | (1ull << 18) | (1ull << 19) | (1ull << 20) | (1ull << 21) |     \
     (1ull << 27) | (1ull << 28))

/*
 * The pink pieces of a 24-piece panel are the four in the middle (6 columns,
 * row by row from the top left: rows 1 and 2, columns 2 and 3).
 */
#define PINK_MASK_24 ((1ull << 8) | (1ull << 9) | (1ull << 14) | (1ull << 15))

bool rare_mask_from_pnel(uint8_t pink, unsigned piece_count, uint64_t *mask)
{
    *mask = 0;
    if (pink == PINK_NONE)
        return true; /* panels with 00 show no pink pieces in Mii Plaza */
    if (pink == PINK_PRESENT && piece_count == 40) {
        *mask = PINK_MASK_40;
        return true;
    }
    if (pink == PINK_PRESENT && piece_count == 24) {
        *mask = PINK_MASK_24;
        return true;
    }
    return false; /* other sizes: pattern not known yet */
}
