/*
 * S7 Sicherungen. Bottom: the backups as cards, newest
 * first, the original last. Top: the focused backup in detail. A or a tap
 * asks "Diesen Stand wiederherstellen?"; the model backs up the current state
 * before it restores.
 */
#include <stdio.h>
#include <string.h>

#include "core/backup_list.h"
#include "core/list_nav.h"
#include "core/press.h"
#include "core/purchase_flow.h"
#include "core/strings.h"
#include "core/toast.h"
#include "core/tween.h"
#include "platform/home_lock.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/button.h"
#include "ui/widgets/card.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/coin.h"
#include "ui/widgets/dialog.h"
#include "ui/widgets/lock.h"
#include "ui/widgets/pill.h"
#include "ui/widgets/progress.h"
#include "ui/widgets/toast_view.h"

#define BACKUPS_MAX 32 /* the app keeps 20 and the original; a few more are shown if they exist */

enum {
    T_BACK,
    T_RESTORE,
    T_ROW0,
    T_COUNT = T_ROW0 + THEME_BACKUP_ROWS + 1, /* a row can be partly visible while scrolling */
};

enum { D_CANCEL, D_RESTORE };

typedef struct {
    char name[BACKUP_NAME_MAX];
    char reason[64];
    char date[24];
    bool has_date;
    bool locked;
    bool has_coins;
    uint16_t coins;
} Row;

typedef struct {
    Scene base;
    Row rows[BACKUPS_MAX];
    unsigned count;
    ListNav nav;
    FocusState focus;
    Tween scroll; /* row offset of the window top */
    float scroll_from;
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT];
    int ids[T_COUNT];
    int rect_count;

    /* The restore dialog. */
    Dialog dialog;
    PurchaseFlow flow; /* the same timing as S5: show "working", run once, hold briefly */
    TxnResult result;
    int dialog_focus;
    HitTap dialog_tap;
    Press dialog_press[2];
    HitRect dialog_rects[2];
    Toast toast;
} BackupsScene;

/* --- Rows ------------------------------------------------------------------- */

static void reload(BackupsScene *s)
{
    static BackupInfo list[BACKUPS_MAX];
    const Model *m = scenes_model();
    unsigned total = model_backups(m, list, BACKUPS_MAX);
    s->count = total < BACKUPS_MAX ? total : BACKUPS_MAX;
    bool day_first = strings_language() == LANG_DE;
    for (unsigned i = 0; i < s->count; i++) {
        Row *r = &s->rows[i];
        const Manifest *mf = &list[i].manifest;
        memcpy(r->name, list[i].name, sizeof(r->name));
        const char *panel = NULL;
        if (mf->has_purchase && mf->panel < PANEL_COUNT)
            panel = model_panel(m, mf->panel)->name;
        backup_row_reason(r->name, mf, panel, r->reason, sizeof(r->reason));
        r->has_date = backup_row_date(r->name, mf, day_first, r->date, sizeof(r->date));
        r->locked = backup_row_locked(r->name);
        r->has_coins = mf->has_coins;
        r->coins = mf->coins;
    }
    list_nav_clamp(&s->nav, s->count, THEME_BACKUP_ROWS);
}

static void scroll_to(BackupsScene *s, unsigned from_first)
{
    s->scroll_from = (float)from_first;
    tween_start(&s->scroll, (float)from_first, (float)s->nav.first, ANIM_PAGE, 0, EASE_OUT_CUBIC);
}

static void backups_on_enter(Scene *self)
{
    BackupsScene *s = (BackupsScene *)self;
    list_nav_init(&s->nav);
    reload(s);
    focus_init(&s->focus, 0, true);
    tween_start(&s->scroll, 0.0f, 0.0f, 0, 0, EASE_OUT_CUBIC);
    hit_tap_reset(&s->tap);
    for (int i = 0; i < T_COUNT; i++)
        press_init(&s->press[i]);
    dialog_init(&s->dialog);
    purchase_flow_init(&s->flow);
    hit_tap_reset(&s->dialog_tap);
    for (int i = 0; i < 2; i++)
        press_init(&s->dialog_press[i]);
    s->rect_count = 0;
    toast_widget_init(&s->toast);
}

/* --- Restore ---------------------------------------------------------------- */

static void open_dialog(BackupsScene *s)
{
    if (s->count == 0) {
        audio_play(SFX_ERROR);
        return;
    }
    audio_play(SFX_CONFIRM);
    s->dialog_focus = D_RESTORE;
    purchase_flow_init(&s->flow);
    hit_tap_reset(&s->dialog_tap);
    dialog_open(&s->dialog);
}

static void restore_done(BackupsScene *s)
{
    dialog_close(&s->dialog);
    purchase_flow_init(&s->flow);
    switch (route_after_purchase(s->result)) {
    case PURCHASE_SUCCESS:
        audio_play(SFX_CONFIRM);
        list_nav_init(&s->nav); /* the backup of the state before is now the newest */
        scroll_to(s, 0);
        reload(s);
        toast_show(&s->toast, str(STR_RESTORED));
        break;
    case PURCHASE_UNDO_FAILED:
        audio_play(SFX_ERROR);
        scene_push(scene_notice(ROUTE_E05));
        break;
    default:
        audio_play(SFX_ERROR);
        toast_show(&s->toast, str(STR_RESTORE_FAILED));
        break;
    }
}

static void dialog_update(BackupsScene *s, const InputFrame *in, u32 keys)
{
    if (s->flow.phase == PF_RUNNING) {
        if (purchase_flow_work_due(&s->flow)) {
            home_lock(true);
            s->result = model_restore(scenes_model(), s->rows[s->nav.index].name);
            home_lock(false);
            purchase_flow_ran(&s->flow);
        }
        purchase_flow_step(&s->flow);
        if (purchase_flow_done(&s->flow))
            restore_done(s);
        return;
    }

    dialog_step(&s->dialog);
    int hit = input_tap(&s->dialog_tap, in, s->dialog_rects, 2);
    if (dialog_ready(&s->dialog)) {
        int dir = input_direction(keys);
        if (dir == FOCUS_LEFT && s->dialog_focus != D_CANCEL) {
            s->dialog_focus = D_CANCEL;
            audio_play(SFX_MOVE);
        } else if (dir == FOCUS_RIGHT && s->dialog_focus != D_RESTORE) {
            s->dialog_focus = D_RESTORE;
            audio_play(SFX_MOVE);
        }
        int act = hit;
        if (keys & KEY_A)
            act = s->dialog_focus;
        if (keys & KEY_B)
            act = D_CANCEL;
        if (act == D_CANCEL) {
            audio_play(SFX_BACK);
            dialog_close(&s->dialog);
        } else if (act == D_RESTORE && purchase_flow_start(&s->flow)) {
            audio_play(SFX_CONFIRM);
        }
    }
    int pressed = hit_tap_pressed(&s->dialog_tap);
    for (int i = 0; i < 2; i++)
        press_update(&s->dialog_press[i], pressed == i, ANIM_BUTTON_RELEASE);
}

/* --- Update ----------------------------------------------------------------- */

static void backups_update(Scene *self, const InputFrame *in)
{
    BackupsScene *s = (BackupsScene *)self;
    u32 keys = input_focus(&s->focus, in);
    toast_step(&s->toast);

    if (s->dialog.open) {
        dialog_update(s, in, keys);
        return;
    }

    int hit = input_tap(&s->tap, in, s->rects, s->rect_count);
    int tapped = hit >= 0 ? s->ids[hit] : -1;

    unsigned first_before = s->nav.first;
    int dir = input_direction(keys);
    bool moved = false;
    if (dir == FOCUS_UP)
        moved = list_nav_move(&s->nav, -1, s->count, THEME_BACKUP_ROWS);
    else if (dir == FOCUS_DOWN)
        moved = list_nav_move(&s->nav, 1, s->count, THEME_BACKUP_ROWS);
    if (keys & KEY_L)
        moved = list_nav_page(&s->nav, -1, s->count, THEME_BACKUP_ROWS) || moved;
    if (keys & KEY_R)
        moved = list_nav_page(&s->nav, 1, s->count, THEME_BACKUP_ROWS) || moved;
    if (moved)
        audio_play(SFX_MOVE);
    else if (dir == FOCUS_UP || dir == FOCUS_DOWN)
        audio_play(SFX_ERROR);

    int act = tapped;
    if (keys & KEY_A)
        act = T_RESTORE;
    if (keys & KEY_B)
        act = T_BACK;

    if (act == T_BACK) {
        audio_play(SFX_BACK);
        scene_pop();
    } else if (act == T_RESTORE) {
        open_dialog(s);
    } else if (act >= T_ROW0) {
        unsigned row = s->nav.first + (unsigned)(act - T_ROW0);
        if (row < s->count) {
            s->nav.index = row;
            list_nav_clamp(&s->nav, s->count, THEME_BACKUP_ROWS);
            open_dialog(s);
        }
    }

    if (s->nav.first != first_before)
        scroll_to(s, first_before);
    tween_step(&s->scroll);

    int p = hit_tap_pressed(&s->tap);
    int pressed = p >= 0 && p < s->rect_count ? s->ids[p] : -1;
    for (int i = 0; i < T_COUNT; i++)
        press_update(&s->press[i], pressed == i, ANIM_BUTTON_RELEASE);
}

/* --- Top screen ------------------------------------------------------------- */

static void backups_draw_top(Scene *self, const StereoView *view)
{
    BackupsScene *s = (BackupsScene *)self;
    const Model *m = scenes_model();

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    char count[16];
    snprintf(count, sizeof(count), "%u/%u", s->count ? s->nav.index + 1 : 0, s->count);
    pill_name_draw(THEME_PILL_MARGIN, THEME_PILL_Y, str(STR_TITLE_BACKUPS), count);
    pill_coin_draw(THEME_TOP_W - THEME_PILL_MARGIN, THEME_PILL_Y, true, m->coins, 0);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    GeomRect card = {(THEME_TOP_W - THEME_BACKUP_CARD_W) / 2, THEME_BACKUP_CARD_Y,
                     THEME_BACKUP_CARD_W, THEME_BACKUP_CARD_H};
    CardView cv = {false, true, THEME_ALPHA_OPAQUE};
    card_draw(card, &cv);
    int cx = card.x + card.w / 2;
    int max_w = card.w - 2 * THEME_BACKUP_CARD_PAD;
    int y = card.y + THEME_BACKUP_CARD_PAD;
    if (s->count == 0) {
        draw_text_vcenter(cx, card.y, card.h, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                          DRAW_ALIGN_CENTER, str(STR_BACKUPS_EMPTY));
    } else {
        const Row *r = &s->rows[s->nav.index];
        draw_text_fit(cx, y, max_w, THEME_TEXT_HEADER, THEME_INK, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_CENTER, r->reason);
        y += draw_text_height(THEME_TEXT_HEADER) + THEME_BACKUP_LINE_GAP;
        if (r->has_date)
            draw_text(cx, y, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
                      r->date);
        y += draw_text_height(THEME_TEXT_BODY) + THEME_BACKUP_CARD_PAD;
        if (r->has_coins) {
            char num[12];
            snprintf(num, sizeof(num), "%u", r->coins);
            int coin = coin_size_px(COIN_LARGE);
            int w = coin + THEME_TEXT_GAP + draw_text_width(THEME_TEXT_TITLE, num);
            coin_draw(COIN_LARGE, cx - w / 2, y, 0);
            draw_text_vcenter(cx - w / 2 + coin + THEME_TEXT_GAP, y, coin, THEME_TEXT_TITLE,
                              THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, num);
        }
        y += coin_size_px(COIN_LARGE) + THEME_BACKUP_LINE_GAP;
        if (r->locked) {
            int note_w = draw_text_width(THEME_TEXT_BODY, str(STR_BACKUP_LOCKED));
            int total = THEME_LOCK_W + THEME_TEXT_GAP + note_w;
            lock_draw(cx - total / 2, y, THEME_INK_3, THEME_ALPHA_OPAQUE);
            draw_text(cx - total / 2 + THEME_LOCK_W + THEME_TEXT_GAP, y, THEME_TEXT_BODY,
                      THEME_INK_2, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, str(STR_BACKUP_LOCKED));
        }
    }
    stereo_layer_end();

    if (s->dialog.open) {
        u8 a = (u8)(THEME_ALPHA_SCRIM * tween_value(&s->dialog.scrim));
        draw_rect(0, 0, THEME_TOP_W, THEME_SCREEN_H, THEME_SCRIM, a);
    }
}

/* --- Bottom screen ---------------------------------------------------------- */

static void register_target(BackupsScene *s, int id, GeomRect box)
{
    if (s->rect_count >= T_COUNT)
        return;
    s->rects[s->rect_count] = (HitRect){box.x, box.y, box.w, box.h};
    s->ids[s->rect_count] = id;
    s->rect_count++;
}

static void draw_row(const Row *r, GeomRect box, bool focus)
{
    CardView cv = {focus, true, THEME_ALPHA_OPAQUE};
    card_draw(box, &cv);

    int x = box.x + THEME_BACKUP_PAD;
    int right = box.x + box.w - THEME_BACKUP_PAD;
    int reason_h = draw_text_height(THEME_TEXT_BUTTON);
    int date_h = draw_text_height(THEME_TEXT_BODY);
    int y = box.y + (box.h - reason_h - THEME_BACKUP_LINE_GAP - date_h) / 2;

    /* Right: the coins of that state. */
    int text_right = right;
    if (r->has_coins) {
        char num[12];
        snprintf(num, sizeof(num), "%u", r->coins);
        int coin = coin_size_px(COIN_MEDIUM);
        int nw = draw_text_width(THEME_TEXT_BUTTON, num);
        draw_text_vcenter(right, box.y, box.h, THEME_TEXT_BUTTON, THEME_INK, THEME_ALPHA_OPAQUE,
                          DRAW_ALIGN_RIGHT, num);
        text_right = right - nw - THEME_TEXT_GAP - coin;
        coin_draw(COIN_MEDIUM, text_right, box.y + (box.h - coin) / 2, 0);
        text_right -= THEME_BACKUP_PAD;
    }

    int tx = x;
    if (r->locked) {
        lock_draw(x, y + (reason_h - THEME_LOCK_H) / 2, THEME_INK_3, THEME_ALPHA_OPAQUE);
        tx += THEME_LOCK_W + THEME_TEXT_GAP;
    }
    draw_text_fit(tx, y, text_right - tx, THEME_TEXT_BUTTON, THEME_INK, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_LEFT, r->reason);
    if (r->has_date)
        draw_text_fit(x, y + reason_h + THEME_BACKUP_LINE_GAP, text_right - x, THEME_TEXT_BODY,
                      THEME_INK_2, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, r->date);
}

static void draw_scrollbar(const BackupsScene *s, float scroll)
{
    if (s->count <= THEME_BACKUP_ROWS)
        return;
    int track_h =
        THEME_BACKUP_ROWS * THEME_BACKUP_ROW_H + (THEME_BACKUP_ROWS - 1) * THEME_BACKUP_ROW_GAP;
    int x = THEME_BOTTOM_W - THEME_SCROLL_MARGIN - THEME_SCROLL_W;
    int thumb_h = track_h * THEME_BACKUP_ROWS / (int)s->count;
    int range = (int)s->count - THEME_BACKUP_ROWS;
    int thumb_y = (int)((float)(track_h - thumb_h) * scroll / (float)range);
    draw_slice_tinted(DRAW_SLICE_ROUND_3, (GeomRect){x, THEME_BACKUP_Y, THEME_SCROLL_W, track_h},
                      THEME_LINE, THEME_LINE, THEME_ALPHA_OPAQUE);
    draw_slice_tinted(DRAW_SLICE_ROUND_3,
                      (GeomRect){x, THEME_BACKUP_Y + thumb_y, THEME_SCROLL_W, thumb_h}, THEME_INK_3,
                      THEME_INK_3, THEME_ALPHA_OPAQUE);
}

static int dialog_height(void)
{
    return THEME_DIALOG_HEAD_H + THEME_DIALOG_PAD + THEME_CONFIRM_LINE_H + THEME_DIALOG_GAP +
           2 * THEME_RESTORE_LINE_H + THEME_DIALOG_GAP + THEME_RESTORE_NOTE_H + THEME_DIALOG_GAP +
           THEME_BTN_H + THEME_DIALOG_PAD;
}

static void draw_dialog(BackupsScene *s)
{
    if (!s->dialog.open)
        return;
    dialog_draw_scrim(&s->dialog);
    GeomRect inner = dialog_draw_frame(&s->dialog, str(STR_BTN_RESTORE), dialog_height());

    draw_opacity_push(dialog_content_alpha(&s->dialog));
    const Row *r = &s->rows[s->nav.index];
    int cx = inner.x + inner.w / 2;
    int w = inner.w - 2 * THEME_DIALOG_PAD;
    int y = inner.y + THEME_DIALOG_PAD;

    draw_text_fit(cx, y, w, THEME_TEXT_HEADER, THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
                  str(STR_RESTORE_QUESTION));
    y += THEME_CONFIRM_LINE_H + THEME_DIALOG_GAP;
    draw_text_fit(cx, y, w, THEME_TEXT_BUTTON, THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
                  r->reason);
    if (r->has_date)
        draw_text(cx, y + THEME_RESTORE_LINE_H, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_CENTER, r->date);
    y += 2 * THEME_RESTORE_LINE_H + THEME_DIALOG_GAP;
    draw_text_wrapped(cx, y, w, THEME_RESTORE_NOTE_LINE_H, 2, THEME_TEXT_BODY, THEME_INK_2,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, str(STR_RESTORE_SAFETY));
    y += THEME_RESTORE_NOTE_H + THEME_DIALOG_GAP;

    int left = inner.x + THEME_DIALOG_PAD;
    if (s->flow.phase == PF_RUNNING) {
        GeomRect track = {left, y + THEME_BTN_H / 2 - THEME_PROGRESS_H - 2, w, THEME_PROGRESS_H};
        /* Out of 101, so the bar never reaches "complete" and turns gold. */
        unsigned fill = (unsigned)(purchase_flow_progress(&s->flow) * 100.0f);
        progress_draw(track, fill, 101, PROGRESS_ON_CARD, THEME_ALPHA_OPAQUE);
        draw_text(cx, track.y + track.h + 4, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_CENTER, str(STR_DO_NOT_TURN_OFF));
        s->dialog_rects[0] = s->dialog_rects[1] = (HitRect){0, 0, 0, 0};
    } else {
        ButtonSpec cancel = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_CANCEL)};
        ButtonView cvw = {false, s->focus.visible && s->dialog_focus == D_CANCEL,
                          press_offset(&s->dialog_press[D_CANCEL])};
        GeomRect b = button_draw(&cancel, left, y, &cvw);
        s->dialog_rects[D_CANCEL] = (HitRect){b.x, b.y, b.w, b.h};

        ButtonSpec restore = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_RESTORE)};
        ButtonView rvw = {false, s->focus.visible && s->dialog_focus == D_RESTORE,
                          press_offset(&s->dialog_press[D_RESTORE])};
        b = button_draw(&restore, left + w - button_width(&restore), y, &rvw);
        s->dialog_rects[D_RESTORE] = (HitRect){b.x, b.y, b.w, b.h};
    }
    draw_opacity_pop();
}

static void backups_draw_bottom(Scene *self)
{
    BackupsScene *s = (BackupsScene *)self;
    s->rect_count = 0;

    float scroll = tween_value(&s->scroll);
    bool settled = tween_done(&s->scroll) && !s->dialog.open;
    GeomRect area = chrome_content();
    draw_clip_push(area);
    int step = THEME_BACKUP_ROW_H + THEME_BACKUP_ROW_GAP;
    unsigned top = (unsigned)scroll;
    for (unsigned i = top; i < s->count && i <= top + THEME_BACKUP_ROWS; i++) {
        int y = THEME_BACKUP_Y + (int)(((float)i - scroll) * (float)step);
        GeomRect box = {THEME_BACKUP_X, y,
                        THEME_BOTTOM_W - 2 * THEME_BACKUP_X - THEME_SCROLL_W - THEME_SCROLL_MARGIN,
                        THEME_BACKUP_ROW_H};
        int slot = (int)(i - s->nav.first);
        bool tappable = settled && slot >= 0 && slot < THEME_BACKUP_ROWS;
        GeomRect drawn = box;
        if (tappable)
            drawn.y += press_offset(&s->press[T_ROW0 + slot]);
        draw_row(&s->rows[i], drawn, tappable && s->focus.visible && i == s->nav.index);
        if (tappable)
            register_target(s, T_ROW0 + slot, box);
    }
    if (s->count == 0)
        draw_text_vcenter(THEME_BOTTOM_W / 2, area.y, area.h, THEME_TEXT_BODY, THEME_INK_2,
                          THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, str(STR_BACKUPS_EMPTY));
    draw_scrollbar(s, scroll);
    draw_clip_pop();

    char note[16];
    snprintf(note, sizeof(note), "%u/%u", s->count ? s->nav.index + 1 : 0, s->count);
    chrome_header_draw(str(STR_TITLE_BACKUPS), note);

    chrome_bar_draw();
    ButtonSpec back = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_BACK)};
    ButtonView bv = {false, false, press_offset(&s->press[T_BACK])};
    register_target(s, T_BACK, chrome_bar_button(&back, false, &bv));

    ButtonSpec restore = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_RESTORE)};
    ButtonView rv = {s->count == 0, false, press_offset(&s->press[T_RESTORE])};
    register_target(s, T_RESTORE, chrome_bar_button(&restore, true, &rv));

    draw_dialog(s);
    toast_widget_draw(&s->toast);
}

static bool backups_blocks_start(const Scene *self)
{
    return ((const BackupsScene *)self)->dialog.open;
}

static BackupsScene s_backups = {.base = {"S7 Sicherungen", backups_on_enter, NULL, backups_update,
                                          backups_draw_top, backups_draw_bottom,
                                          backups_blocks_start}};

Scene *scene_backups(void)
{
    return &s_backups.base;
}
