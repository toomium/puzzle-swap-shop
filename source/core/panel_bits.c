#include "core/panel_bits.h"

/*
 * Storage bit order:
 *   standard panels: LSB first, piece i -> byte i / 8, mask 1 << (i % 8)
 *   SpotPass panels: MSB first, piece i -> byte i / 8, mask 0x80 >> (i % 8)
 * SpotPass is confirmed on hardware for 15-piece panels (2026-10-26).
 * The standard order is still a hypothesis until confirmed on a console.
 */
static uint8_t bit_mask(PanelKind kind, unsigned bit)
{
    if (kind == PANEL_STANDARD)
        return (uint8_t)(1u << (bit % 8));
    return (uint8_t)(0x80u >> (bit % 8));
}

unsigned panel_bits_field_size(unsigned piece_count)
{
    return (piece_count + 7) / 8;
}

uint64_t panel_bits_all(unsigned piece_count)
{
    if (piece_count >= 64)
        return ~(uint64_t)0;
    return ((uint64_t)1 << piece_count) - 1;
}

unsigned panel_bits_count(uint64_t mask)
{
    unsigned n = 0;
    while (mask) {
        mask &= mask - 1;
        n++;
    }
    return n;
}

uint64_t panel_bits_decode(const uint8_t *field, PanelKind kind, unsigned piece_count)
{
    uint64_t owned = 0;
    for (unsigned i = 0; i < piece_count; i++) {
        if (field[i / 8] & bit_mask(kind, i))
            owned |= (uint64_t)1 << i;
    }
    return owned;
}

void panel_bits_encode(uint8_t *field, PanelKind kind, unsigned piece_count, uint64_t owned)
{
    for (unsigned i = 0; i < piece_count; i++) {
        uint8_t mask = bit_mask(kind, i);
        if (owned & ((uint64_t)1 << i))
            field[i / 8] |= mask;
        else
            field[i / 8] &= (uint8_t)~mask;
    }
}

bool panel_bits_unused_clear(const uint8_t *field, PanelKind kind, unsigned piece_count)
{
    unsigned bits = panel_bits_field_size(piece_count) * 8;
    for (unsigned i = piece_count; i < bits; i++) {
        if (field[i / 8] & bit_mask(kind, i))
            return false;
    }
    return true;
}
