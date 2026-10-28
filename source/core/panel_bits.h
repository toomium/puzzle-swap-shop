#ifndef PSS_CORE_PANEL_BITS_H
#define PSS_CORE_PANEL_BITS_H

#include <stdbool.h>
#include <stdint.h>

#include "core/panel_catalog.h"

/*
 * Translation between the piece bitfields stored in meet.dat and an "owned"
 * mask where bit i is piece i. This file is the only place that knows the
 * storage bit order.
 */

#define PANEL_BITS_MAX_BYTES 5

/* Bytes used by the bitfield of a panel with piece_count pieces. */
unsigned panel_bits_field_size(unsigned piece_count);

/* Mask with the lowest piece_count bits set. */
uint64_t panel_bits_all(unsigned piece_count);

unsigned panel_bits_count(uint64_t mask);

uint64_t panel_bits_decode(const uint8_t *field, PanelKind kind, unsigned piece_count);

/* Writes owned into field; bits that belong to no piece keep their value. */
void panel_bits_encode(uint8_t *field, PanelKind kind, unsigned piece_count, uint64_t owned);

/* True if every bit that belongs to no piece is 0. */
bool panel_bits_unused_clear(const uint8_t *field, PanelKind kind, unsigned piece_count);

#endif
