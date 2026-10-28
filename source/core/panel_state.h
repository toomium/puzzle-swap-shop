#ifndef PSS_CORE_PANEL_STATE_H
#define PSS_CORE_PANEL_STATE_H

#include <stdbool.h>
#include <stdint.h>

#define PANEL_NAME_MAX 64

/* What the app knows about one panel on this console. */
typedef struct {
    uint8_t id;
    bool available; /* standard panels always; SpotPass panels if downloaded */
    uint8_t piece_count;
    uint64_t owned; /* bit i: piece i owned */
    uint64_t rare;  /* bit i: piece i is a rare (pink) piece, if known */
    char name[PANEL_NAME_MAX];
} PanelState;

#endif
