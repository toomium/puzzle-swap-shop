/*
 * S3 Teile auswählen. The grid at the
 * bottom mirrors the board at the top, piece by piece. A tap or A selects
 * a missing piece, Y selects all buyable ones, X buys.
 */
#include <stdio.h>
#include <string.h>

#include "core/panel_bits.h"
#include "core/picker.h"
#include "core/press.h"
#include "core/strings.h"
#include "core/toast.h"
#include "core/widget_math.h"
#include "platform/home_lock.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/confirm_dialog.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/art.h"
#include "ui/widgets/board.h"
#include "ui/widgets/button.h"
#include "ui/widgets/chip.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/coin.h"
#include "ui/widgets/pill.h"
#include "ui/widgets/progress.h"
#include "ui/widgets/tile.h"
#include "ui/widgets/toast_view.h"

enum {
    T_BACK,
    T_BUY,
    T_ALL,
    T_TILE0,
    T_COUNT = T_TILE0 + PANEL_MAX_PIECES,
};

typedef struct {
    Scene base;
    unsigned panel_id;
    uint64_t selected;
    FocusState vis; /* only `visible` is used; the position is in `pos` */
    PickerFocus pos;
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT];
    int ids[T_COUNT];
    int rect_count;
    unsigned bump_start[PANEL_MAX_PIECES];
    bool bumping[PANEL_MAX_PIECES];
    Toast toast;
    bool start_visible;
    ConfirmDialog confirm; /* S4 and S5 */
    TxnResult result;      /* of the purchase S5 ran */
} PickerScene;

static const PanelState *panel_of(const PickerScene *s)
{
    return model_panel(scenes_model(), s->panel_id);
}

static void picker_on_enter(Scene *self)
{
    PickerScene *s = (PickerScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_of(s);
    s->selected = 0;
    focus_init(&s->vis, 0, s->start_visible);
    /* Start on the first piece that can be bought, if any. */
    uint64_t buyable = shop_buyable(p, &m->settings.prices);
    int first = 0;
    while (first < p->piece_count && !(buyable & ((uint64_t)1 << first)))
        first++;
    picker_focus_init(&s->pos, first < p->piece_count ? first : 0);
    hit_tap_reset(&s->tap);
    for (int i = 0; i < T_COUNT; i++)
        press_init(&s->press[i]);
    memset(s->bumping, 0, sizeof(s->bumping));
    confirm_init(&s->confirm);
    s->rect_count = 0;
    toast_widget_init(&s->toast);
}

static void select_piece(PickerScene *s, unsigned index)
{
    const Model *m = scenes_model();
    PickResult r = picker_toggle(&s->selected, panel_of(s), &m->settings.prices, index);
    if (r == PICK_REFUSED) {
        audio_play(SFX_ERROR);
        return;
    }
    s->bumping[index] = true;
    s->bump_start[index] = anim_frame();
    audio_play(r == PICK_ON ? SFX_SELECT : SFX_DESELECT);
}

static void try_buy(PickerScene *s)
{
    const Model *m = scenes_model();
    PickerSummary sum = picker_summary(panel_of(s), s->selected, &m->settings.prices, m->coins);
    if (sum.count == 0) {
        audio_play(SFX_ERROR);
    } else if (!sum.affordable) {
        audio_play(SFX_ERROR);
        toast_show(&s->toast, str(STR_NOT_ENOUGH_COINS));
    } else {
        PurchasePlan plan;
        if (model_plan(m, s->panel_id, s->selected, &plan) != SHOP_OK) {
            /* The save data changed under us or looks unknown: nothing is offered. */
            audio_play(SFX_ERROR);
            toast_show(&s->toast, str(m->status == MODEL_READ_ONLY ? STR_E03 : STR_E04));
            return;
        }
        audio_play(SFX_CONFIRM);
        confirm_open(&s->confirm, &plan);
    }
}

/* S5 is over: on to the result. */
static void show_result(PickerScene *s)
{
    switch (route_after_purchase(s->result)) {
    case PURCHASE_SUCCESS:
        scene_replace(scene_success(s->panel_id, s->confirm.plan.pieces, s->confirm.plan.cost,
                                    s->confirm.plan.coins_before, scenes_model()->last.backup));
        break;
    case PURCHASE_UNDO_FAILED:
        audio_play(SFX_ERROR);
        scene_replace(scene_notice(ROUTE_E05));
        break;
    default:
        audio_play(SFX_ERROR);
        scene_replace(scene_notice(ROUTE_E04));
        break;
    }
}

static void confirm_step(PickerScene *s, const InputFrame *in, u32 keys)
{
    ConfirmEvent ev = confirm_update(&s->confirm, in, keys);
    if (ev == CONFIRM_RUN) {
        home_lock(true);
        s->result = model_buy(scenes_model(), &s->confirm.plan);
        home_lock(false);
        confirm_ran(&s->confirm);
    }
    if (confirm_finished(&s->confirm))
        show_result(s);
}

static void picker_update(Scene *self, const InputFrame *in)
{
    PickerScene *s = (PickerScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_of(s);
    const ThemeBoard *tb = theme_board(p->piece_count);

    u32 keys = input_focus(&s->vis, in);
    if (confirm_active(&s->confirm)) {
        confirm_step(s, in, keys);
        toast_step(&s->toast);
        return;
    }

    int hit = input_tap(&s->tap, in, s->rects, s->rect_count);
    int tapped = hit >= 0 ? s->ids[hit] : -1;

    int dir = input_direction(keys);
    if (dir >= 0 && tb) {
        int before_index = s->pos.index, before_bar = s->pos.bar;
        picker_focus_move(&s->pos, (FocusDir)dir, tb->cols, p->piece_count);
        if (before_index != s->pos.index || before_bar != s->pos.bar)
            audio_play(SFX_MOVE);
    }

    int act = tapped;
    if (keys & KEY_A) {
        if (!picker_focus_in_bar(&s->pos))
            act = T_TILE0 + s->pos.index;
        else
            act = s->pos.bar == PICKER_BAR_BACK ? T_BACK : T_BUY;
    }
    if (keys & KEY_B)
        act = T_BACK;
    if (keys & KEY_X)
        act = T_BUY;
    if (keys & KEY_Y)
        act = T_ALL;

    if (act == T_BACK) {
        audio_play(SFX_BACK);
        scene_pop();
    } else if (act == T_BUY) {
        try_buy(s);
    } else if (act == T_ALL) {
        bool on = picker_select_all(&s->selected, p, &m->settings.prices);
        audio_play(on ? SFX_SELECT : SFX_DESELECT);
    } else if (act >= T_TILE0) {
        if (!(keys & KEY_A) && tapped >= T_TILE0)
            s->pos.index = act - T_TILE0; /* the ring follows a tap when it is shown again */
        select_piece(s, (unsigned)(act - T_TILE0));
    }

    toast_step(&s->toast);
    int pr = hit_tap_pressed(&s->tap);
    int pressed = pr >= 0 && pr < s->rect_count ? s->ids[pr] : -1;
    for (int i = 0; i < T_COUNT; i++)
        press_update(&s->press[i], pressed == i, ANIM_BUTTON_RELEASE);
}

/* --- Top screen -------------------------------------------------------------- */

static void picker_draw_top(Scene *self, const StereoView *view)
{
    PickerScene *s = (PickerScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_of(s);
    unsigned total = p->piece_count;
    PickerSummary sum = picker_summary(p, s->selected, &m->settings.prices, m->coins);
    unsigned owned = panel_bits_count(p->owned);

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    char count[16];
    snprintf(count, sizeof(count), "%u/%u", owned, total);
    pill_name_draw(THEME_PILL_MARGIN, THEME_PILL_Y, p->name, count);
    pill_coin_draw(THEME_TOP_W - THEME_PILL_MARGIN, THEME_PILL_Y, true, m->coins, 0);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    BoardSpec board = {(int)total, p->owned, s->selected, p->rare, 0, 0};
    art_use(p->id);
    board_draw(&board);
    art_use(ART_PLACEHOLDER);

    GeomRect track = {(THEME_TOP_W - THEME_PROGRESS_W) / 2, THEME_PROGRESS_Y, THEME_PROGRESS_W,
                      THEME_PROGRESS_H};
    progress_draw(track, sum.owned_after, total, PROGRESS_ON_SKY, THEME_ALPHA_OPAQUE);
    char left[32], right[32];
    snprintf(left, sizeof(left), str(STR_SELECTED), sum.count);
    snprintf(right, sizeof(right), str(STR_AFTER_PURCHASE), sum.owned_after, total);
    int ty = track.y + (track.h - draw_text_height(THEME_TEXT_BODY)) / 2;
    draw_text(track.x - THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2,
              THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT, left);
    draw_text(track.x + track.w + THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2,
              THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, right);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    confirm_draw_top(&s->confirm);
    stereo_layer_end();
}

/* --- Bottom screen ------------------------------------------------------------ */

static void register_target(PickerScene *s, int id, GeomRect box)
{
    if (s->rect_count >= T_COUNT)
        return;
    s->rects[s->rect_count] = (HitRect){box.x, box.y, box.w, box.h};
    s->ids[s->rect_count] = id;
    s->rect_count++;
}

static void draw_grid(PickerScene *s, const PanelState *p, const ShopPrices *prices)
{
    const ThemeBoard *b = theme_board(p->piece_count);
    if (!b)
        return;
    int w = b->cols * b->tile_w + (b->cols - 1) * THEME_TILE_GAP;
    int h = b->rows * b->tile_h + (b->rows - 1) * THEME_TILE_GAP;
    GeomRect art = {(THEME_BOTTOM_W - w) / 2, THEME_GRID_Y, w, h};
    uint64_t buyable = shop_buyable(p, prices);

    for (int i = 0; i < b->pieces; i++) {
        GeomRect box = {art.x + (i % b->cols) * (b->tile_w + THEME_TILE_GAP),
                        art.y + (i / b->cols) * (b->tile_h + THEME_TILE_GAP), b->tile_w, b->tile_h};
        bool owned = widget_mask_has(p->owned, i);
        TileState state = owned                             ? TILE_OWNED
                          : widget_mask_has(s->selected, i) ? TILE_SELECTED
                                                            : TILE_MISSING;
        float bump = 1.0f;
        if (s->bumping[i]) {
            unsigned age = anim_frame() - s->bump_start[i];
            if (age < ANIM_PIECE_SELECT)
                bump = anim_piece_select_scale(age);
            else
                s->bumping[i] = false;
        }
        TileSpec spec = {state, i + 1, widget_mask_has(p->rare, i),
                         !owned && !widget_mask_has(buyable, i), art};
        TileView view = {s->vis.visible && s->pos.index == i, bump};
        art_use(p->id);
        tile_draw(box, &spec, &view);
        art_use(ART_PLACEHOLDER);
        register_target(s, T_TILE0 + i, box);
    }
}

static void draw_summary(const PickerSummary *sum)
{
    char pieces[24], price[16];
    strings_pieces(pieces, sizeof(pieces), sum->count);
    int cx = THEME_BOTTOM_W / 2;
    draw_text(cx, THEME_BAR_Y + THEME_SUMMARY_Y, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
              DRAW_ALIGN_CENTER, pieces);
    snprintf(price, sizeof(price), "%u", (unsigned)sum->cost);
    int w = coin_size_px(COIN_LARGE) + THEME_PRICE_GAP + draw_text_width(THEME_TEXT_BUTTON, price);
    coin_price_draw(cx - w / 2, THEME_BAR_Y + THEME_SUMMARY_PRICE_Y, coin_size_px(COIN_LARGE),
                    sum->cost, sum->affordable ? THEME_INK : THEME_DANGER);
}

static void picker_draw_bottom(Scene *self)
{
    PickerScene *s = (PickerScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_of(s);
    s->rect_count = 0;

    draw_grid(s, p, &m->settings.prices);

    chrome_header_draw(str(STR_TITLE_PICKER), NULL);
    ChipSpec all = {DRAW_GLYPH_Y, str(STR_BTN_SELECT_ALL)};
    int cx = THEME_BOTTOM_W - THEME_CONTENT_PAD - chip_width(&all);
    ChipView cv = {false, false};
    GeomRect c = chip_draw(&all, cx, (THEME_HEADER_H - THEME_CHIP_H) / 2, &cv);
    register_target(s, T_ALL, c);

    PickerSummary sum = picker_summary(p, s->selected, &m->settings.prices, m->coins);
    chrome_bar_draw();
    draw_summary(&sum);
    bool in_bar = picker_focus_in_bar(&s->pos) && s->vis.visible;

    ButtonSpec back = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_BACK)};
    ButtonView bv = {false, in_bar && s->pos.bar == PICKER_BAR_BACK,
                     press_offset(&s->press[T_BACK])};
    register_target(s, T_BACK, chrome_bar_button(&back, false, &bv));

    ButtonSpec buy = {BUTTON_PRIMARY, DRAW_GLYPH_X, str(STR_BTN_BUY)};
    ButtonView uv = {!sum.can_buy, in_bar && s->pos.bar == PICKER_BAR_BUY,
                     press_offset(&s->press[T_BUY])};
    register_target(s, T_BUY, chrome_bar_button(&buy, true, &uv));

    toast_widget_draw(&s->toast);

    if (confirm_active(&s->confirm)) {
        s->rect_count = 0; /* only the dialog takes touches */
        confirm_draw(&s->confirm, s->vis.visible);
    }
}

static bool picker_blocks_start(const Scene *self)
{
    return confirm_active(&((const PickerScene *)self)->confirm);
}

static PickerScene s_picker = {.base = {"S3 Teile auswählen", picker_on_enter, NULL, picker_update,
                                        picker_draw_top, picker_draw_bottom, picker_blocks_start}};

Scene *scene_picker(unsigned panel_id, bool focus_visible)
{
    s_picker.panel_id = panel_id;
    s_picker.start_visible = focus_visible;
    return &s_picker.base;
}
