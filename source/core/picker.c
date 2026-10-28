#include "core/picker.h"

#include "core/panel_bits.h"

PickerSummary picker_summary(const PanelState *panel, uint64_t selected, const ShopPrices *prices,
                             uint16_t coins)
{
    PickerSummary s;
    s.count = panel_bits_count(selected);
    s.cost = shop_cost(panel, selected, prices);
    s.affordable = s.cost <= coins;
    s.can_buy = s.count > 0 && s.affordable;
    s.owned_after = panel_bits_count(panel->owned | selected);
    return s;
}

PickResult picker_toggle(uint64_t *selected, const PanelState *panel, const ShopPrices *prices,
                         unsigned index)
{
    if (index >= panel->piece_count)
        return PICK_REFUSED;
    uint64_t bit = (uint64_t)1 << index;
    if (!(shop_buyable(panel, prices) & bit))
        return PICK_REFUSED;
    *selected ^= bit;
    return (*selected & bit) ? PICK_ON : PICK_OFF;
}

bool picker_select_all(uint64_t *selected, const PanelState *panel, const ShopPrices *prices)
{
    uint64_t all = shop_buyable(panel, prices);
    if (all == 0) {
        *selected = 0;
        return false;
    }
    if ((*selected & all) == all) {
        *selected = 0;
        return false;
    }
    *selected = all;
    return true;
}

void picker_focus_init(PickerFocus *f, int index)
{
    f->index = index;
    f->bar = PICKER_BAR_BUY;
    f->last = index;
}

bool picker_focus_in_bar(const PickerFocus *f)
{
    return f->index < 0;
}

FocusMove picker_focus_move(PickerFocus *f, FocusDir dir, int cols, int count)
{
    if (picker_focus_in_bar(f)) {
        if (dir == FOCUS_LEFT && f->bar != PICKER_BAR_BACK) {
            f->bar = PICKER_BAR_BACK;
            return FOCUS_MOVED;
        }
        if (dir == FOCUS_RIGHT && f->bar != PICKER_BAR_BUY) {
            f->bar = PICKER_BAR_BUY;
            return FOCUS_MOVED;
        }
        if (dir == FOCUS_UP) {
            f->index = f->last;
            return FOCUS_MOVED;
        }
        return FOCUS_BLOCKED;
    }

    int index = f->index;
    FocusMove r = focus_grid_move(&index, cols, count, dir, FOCUS_EXITS(FOCUS_DOWN));
    if (r == FOCUS_EXIT_DOWN) {
        f->last = f->index;
        f->bar = (f->index % cols) < cols / 2 ? PICKER_BAR_BACK : PICKER_BAR_BUY;
        f->index = -1;
        return FOCUS_MOVED;
    }
    f->index = index;
    return r;
}
