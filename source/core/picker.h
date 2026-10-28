#ifndef PSS_CORE_PICKER_H
#define PSS_CORE_PICKER_H

#include <stdbool.h>
#include <stdint.h>

#include "core/focus.h"
#include "core/panel_state.h"
#include "core/shop.h"

/*
 * Logic of S3 "Teile auswählen": which pieces
 * can be selected, what the selection costs and where the focus goes
 * between the piece grid and the button bar.
 */

typedef struct {
    unsigned count;       /* pieces selected */
    uint16_t cost;        /* in play coins */
    bool affordable;      /* cost <= coins */
    bool can_buy;         /* something selected and affordable */
    unsigned owned_after; /* pieces owned once the selection is bought */
} PickerSummary;

PickerSummary picker_summary(const PanelState *panel, uint64_t selected, const ShopPrices *prices,
                             uint16_t coins);

typedef enum {
    PICK_ON,
    PICK_OFF,
    PICK_REFUSED, /* owned, not for sale or out of range: nothing changes */
} PickResult;

/* Taps one piece (0-based). */
PickResult picker_toggle(uint64_t *selected, const PanelState *panel, const ShopPrices *prices,
                         unsigned index);

/* Y: selects every buyable piece; if all of them are selected already, clears. Returns whether
 * pieces are selected afterwards. */
bool picker_select_all(uint64_t *selected, const PanelState *panel, const ShopPrices *prices);

/* Focus over the piece grid and the two buttons of the bar. */
typedef enum {
    PICKER_BAR_BACK,
    PICKER_BAR_BUY,
} PickerBar;

typedef struct {
    int index; /* piece, or -1 while the focus is in the bar */
    int bar;   /* PickerBar, valid while index is -1 */
    int last;  /* piece to return to from the bar */
} PickerFocus;

void picker_focus_init(PickerFocus *f, int index);
bool picker_focus_in_bar(const PickerFocus *f);

/*
 * One step. Down from the last row enters the bar (the left half of the grid
 * lands on Back, the right half on Buy), up from the bar returns to the
 * piece it came from. FOCUS_BLOCKED means nothing changed.
 */
FocusMove picker_focus_move(PickerFocus *f, FocusDir dir, int cols, int count);

#endif
