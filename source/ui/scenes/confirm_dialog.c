#include "ui/scenes/confirm_dialog.h"

#include <stdio.h>

#include "core/panel_bits.h"
#include "core/strings.h"
#include "core/widget_math.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/button.h"
#include "ui/widgets/coin.h"
#include "ui/widgets/progress.h"

enum { C_CANCEL, C_BUY };

void confirm_init(ConfirmDialog *c)
{
    dialog_init(&c->dialog);
    purchase_flow_init(&c->flow);
    c->focus = C_BUY;
    hit_tap_reset(&c->tap);
    for (int i = 0; i < 2; i++) {
        press_init(&c->press[i]);
        c->rects[i] = (HitRect){0, 0, 0, 0};
    }
}

void confirm_open(ConfirmDialog *c, const PurchasePlan *plan)
{
    confirm_init(c);
    c->plan = *plan;
    dialog_open(&c->dialog);
}

bool confirm_active(const ConfirmDialog *c)
{
    return c->dialog.open;
}

bool confirm_buying(const ConfirmDialog *c)
{
    return c->flow.phase == PF_RUNNING;
}

void confirm_ran(ConfirmDialog *c)
{
    purchase_flow_ran(&c->flow);
}

bool confirm_finished(const ConfirmDialog *c)
{
    return purchase_flow_done(&c->flow);
}

ConfirmEvent confirm_update(ConfirmDialog *c, const InputFrame *in, u32 keys)
{
    ConfirmEvent ev = CONFIRM_NONE;

    if (confirm_buying(c)) {
        if (purchase_flow_work_due(&c->flow))
            ev = CONFIRM_RUN;
        purchase_flow_step(&c->flow);
        return ev;
    }

    dialog_step(&c->dialog);
    int hit = input_tap(&c->tap, in, c->rects, 2);
    if (dialog_ready(&c->dialog)) {
        int dir = input_direction(keys);
        if (dir == FOCUS_LEFT && c->focus != C_CANCEL) {
            c->focus = C_CANCEL;
            audio_play(SFX_MOVE);
        } else if (dir == FOCUS_RIGHT && c->focus != C_BUY) {
            c->focus = C_BUY;
            audio_play(SFX_MOVE);
        }

        int act = hit;
        if (keys & KEY_A)
            act = c->focus;
        if (keys & KEY_B)
            act = C_CANCEL;

        if (act == C_CANCEL) {
            audio_play(SFX_BACK);
            dialog_close(&c->dialog);
            ev = CONFIRM_CANCELLED;
        } else if (act == C_BUY && purchase_flow_start(&c->flow)) {
            audio_play(SFX_CONFIRM);
        }
    }
    int pressed = hit_tap_pressed(&c->tap);
    for (int i = 0; i < 2; i++)
        press_update(&c->press[i], pressed == i, ANIM_BUTTON_RELEASE);
    return ev;
}

void confirm_draw_top(const ConfirmDialog *c)
{
    if (!c->dialog.open)
        return;
    u8 a = (u8)(THEME_ALPHA_SCRIM * tween_value(&c->dialog.scrim));
    draw_rect(0, 0, THEME_TOP_W, THEME_SCREEN_H, THEME_SCRIM, a);
}

/* Piece numbers, 1-based, of the plan in ascending order. */
static unsigned plan_numbers(const PurchasePlan *plan, unsigned *out, unsigned max)
{
    unsigned n = 0;
    for (unsigned i = 0; i < PANEL_MAX_PIECES && n < max; i++) {
        if (plan->pieces & ((uint64_t)1 << i))
            out[n++] = i + 1;
    }
    return n;
}

static int chips_per_row(int inner_w)
{
    return (inner_w - 2 * THEME_DIALOG_PAD + THEME_CONFIRM_CHIP_GAP) /
           (THEME_CONFIRM_CHIP_W + THEME_CONFIRM_CHIP_GAP);
}

static int chip_rows(const PurchasePlan *plan)
{
    unsigned count = panel_bits_count(plan->pieces);
    int per_row = chips_per_row(THEME_DIALOG_W);
    int rows = ((int)count + per_row - 1) / per_row;
    return rows > THEME_CONFIRM_ROWS ? THEME_CONFIRM_ROWS : rows < 1 ? 1 : rows;
}

static int dialog_height(const PurchasePlan *plan)
{
    int rows = chip_rows(plan);
    return THEME_DIALOG_HEAD_H + THEME_DIALOG_PAD + 2 * THEME_CONFIRM_LINE_H + THEME_DIALOG_GAP +
           rows * THEME_CONFIRM_CHIP_H + (rows - 1) * THEME_CONFIRM_CHIP_GAP + THEME_DIALOG_GAP +
           THEME_BALANCE_H + THEME_DIALOG_GAP + THEME_BTN_H + THEME_DIALOG_PAD;
}

static void draw_number_chip(GeomRect box, const char *text)
{
    draw_slice_tinted(DRAW_SLICE_ROUND_5, box, THEME_PLAZA_DARK, THEME_PLAZA_DARK,
                      THEME_ALPHA_OPAQUE);
    draw_slice_tinted(DRAW_SLICE_ROUND_5, geom_rect_inset(box, 1), THEME_PLAZA_HI, THEME_PLAZA,
                      THEME_ALPHA_OPAQUE);
    draw_text_vcenter(box.x + box.w / 2, box.y, box.h, THEME_TEXT_BODY, THEME_WHITE,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, text);
}

static int draw_number_chips(const PurchasePlan *plan, int cx, int y)
{
    unsigned numbers[PANEL_MAX_PIECES];
    unsigned count = plan_numbers(plan, numbers, PANEL_MAX_PIECES);
    int per_row = chips_per_row(THEME_DIALOG_W);
    int rows = chip_rows(plan);
    int shown = (int)count;
    bool cut = shown > per_row * rows;
    if (cut)
        shown = per_row * rows;

    for (int row = 0; row < rows; row++) {
        int first = row * per_row;
        int in_row = shown - first < per_row ? shown - first : per_row;
        if (in_row <= 0)
            break;
        int w = in_row * THEME_CONFIRM_CHIP_W + (in_row - 1) * THEME_CONFIRM_CHIP_GAP;
        int x = cx - w / 2;
        int ry = y + row * (THEME_CONFIRM_CHIP_H + THEME_CONFIRM_CHIP_GAP);
        for (int i = 0; i < in_row; i++) {
            char text[8];
            int idx = first + i;
            if (cut && idx == shown - 1)
                snprintf(text, sizeof(text), "+%u", count - (unsigned)idx);
            else
                snprintf(text, sizeof(text), "%u", numbers[idx]);
            GeomRect box = {x + i * (THEME_CONFIRM_CHIP_W + THEME_CONFIRM_CHIP_GAP), ry,
                            THEME_CONFIRM_CHIP_W, THEME_CONFIRM_CHIP_H};
            draw_number_chip(box, text);
        }
    }
    return rows * THEME_CONFIRM_CHIP_H + (rows - 1) * THEME_CONFIRM_CHIP_GAP;
}

static void draw_balance(GeomRect row, unsigned before, unsigned after)
{
    draw_slice_tinted(DRAW_SLICE_ROUND_5, row, THEME_LINE, THEME_LINE, THEME_ALPHA_OPAQUE);
    draw_slice_tinted(DRAW_SLICE_ROUND_5, geom_rect_inset(row, 1), THEME_PLAZA_MIST,
                      THEME_PLAZA_MIST, THEME_ALPHA_OPAQUE);

    char a[12], b[12];
    snprintf(a, sizeof(a), "%u", before);
    snprintf(b, sizeof(b), "%u", after);
    int coin = coin_size_px(COIN_MEDIUM);
    int label_w = draw_text_width(THEME_TEXT_BODY, str(STR_BALANCE));
    int a_w = draw_text_width(THEME_TEXT_BUTTON, a);
    int b_w = draw_text_width(THEME_TEXT_BUTTON, b);
    int arrow_w = draw_text_width(THEME_TEXT_BODY, "→");
    int gap = THEME_TEXT_GAP;
    int total = label_w + gap * 2 + coin + gap + a_w + gap + arrow_w + gap + coin + gap + b_w;
    int x = row.x + (row.w - total) / 2;
    int cy = row.y + (row.h - coin) / 2;

    draw_text_vcenter(x, row.y, row.h, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, str(STR_BALANCE));
    x += label_w + gap * 2;
    coin_draw(COIN_MEDIUM, x, cy, 0);
    x += coin + gap;
    draw_text_vcenter(x, row.y, row.h, THEME_TEXT_BUTTON, THEME_INK, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, a);
    x += a_w + gap;
    draw_text_vcenter(x, row.y, row.h, THEME_TEXT_BODY, THEME_INK_3, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, "→");
    x += arrow_w + gap;
    coin_draw(COIN_MEDIUM, x, cy, 0);
    x += coin + gap;
    draw_text_vcenter(x, row.y, row.h, THEME_TEXT_BUTTON, THEME_PLAZA_DARK, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, b);
}

void confirm_draw(ConfirmDialog *c, bool focus_visible)
{
    if (!c->dialog.open)
        return;
    const PurchasePlan *plan = &c->plan;
    dialog_draw_scrim(&c->dialog);
    GeomRect inner = dialog_draw_frame(&c->dialog, str(STR_TITLE_CONFIRM), dialog_height(plan));

    draw_opacity_push(dialog_content_alpha(&c->dialog));
    int cx = inner.x + inner.w / 2;
    int y = inner.y + THEME_DIALOG_PAD;

    char pieces[24], coins[24], question[96];
    strings_pieces(pieces, sizeof(pieces), panel_bits_count(plan->pieces));
    strings_coins(coins, sizeof(coins), plan->cost);
    snprintf(question, sizeof(question), str(STR_CONFIRM_QUESTION), pieces, coins);
    draw_text_wrapped(cx, y, inner.w - 2 * THEME_DIALOG_PAD, THEME_CONFIRM_LINE_H, 2,
                      THEME_TEXT_HEADER, THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
                      question);
    y += 2 * THEME_CONFIRM_LINE_H + THEME_DIALOG_GAP;

    y += draw_number_chips(plan, cx, y) + THEME_DIALOG_GAP;

    GeomRect row = {inner.x + THEME_DIALOG_PAD, y, inner.w - 2 * THEME_DIALOG_PAD, THEME_BALANCE_H};
    draw_balance(row, plan->coins_before, (unsigned)(plan->coins_before - plan->cost));
    y += THEME_BALANCE_H + THEME_DIALOG_GAP;

    if (confirm_buying(c)) {
        GeomRect track = {row.x, y + THEME_BTN_H / 2 - THEME_PROGRESS_H - 2, row.w,
                          THEME_PROGRESS_H};
        /* Out of 101, so the bar never reaches "complete" and turns gold. */
        unsigned fill = (unsigned)(purchase_flow_progress(&c->flow) * 100.0f);
        progress_draw(track, fill, 101, PROGRESS_ON_CARD, THEME_ALPHA_OPAQUE);
        draw_text(cx, track.y + track.h + 4, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_CENTER, str(STR_DO_NOT_TURN_OFF));
        c->rects[0] = c->rects[1] = (HitRect){0, 0, 0, 0};
    } else {
        ButtonSpec cancel = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_CANCEL)};
        ButtonView cv = {false, focus_visible && c->focus == C_CANCEL,
                         press_offset(&c->press[C_CANCEL])};
        GeomRect b = button_draw(&cancel, row.x, y, &cv);
        c->rects[C_CANCEL] = (HitRect){b.x, b.y, b.w, b.h};

        ButtonSpec buy = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_BUY)};
        ButtonView bv = {false, focus_visible && c->focus == C_BUY, press_offset(&c->press[C_BUY])};
        b = button_draw(&buy, row.x + row.w - button_width(&buy), y, &bv);
        c->rects[C_BUY] = (HitRect){b.x, b.y, b.w, b.h};
    }
    draw_opacity_pop();
}
