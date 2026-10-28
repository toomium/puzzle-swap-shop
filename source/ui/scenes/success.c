/*
 * S6 Kauf erfolgreich. Top: the new pieces
 * pop into the board one after another, the "Neue Teile!" ribbon drops in,
 * the coin display counts down. A puzzle that is complete now gets more:
 * confetti and a star. Bottom: a card that confirms the purchase and names
 * the backup that was made before it.
 */
#include <stdio.h>
#include <string.h>

#include "core/backup_names.h"
#include "core/coin_counter.h"
#include "core/confetti.h"
#include "core/easing.h"
#include "core/panel_bits.h"
#include "core/press.h"
#include "core/strings.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/art.h"
#include "ui/widgets/board.h"
#include "ui/widgets/button.h"
#include "ui/widgets/card.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/pill.h"
#include "ui/widgets/progress.h"

#define BOARD_POP_DELAY   6  /* frames before the first new piece pops in */
#define RIBBON_DROP       14 /* frames the ribbon takes to fall in */
#define CELEBRATION_START 20
#define STAR_HOLD         100 /* frames the star stays after the celebration began */
#define STAR_FADE         30

typedef struct {
    Scene base;
    unsigned panel_id;
    uint64_t pieces;
    unsigned cost;
    unsigned coins_before;
    char backup[BACKUP_NAME_MAX];
    bool complete;
    unsigned frame;
    CoinCounter coins;
    FocusState focus;
    HitTap tap;
    Press press;
    HitRect rect;
} SuccessScene;

static const PanelState *panel_of(const SuccessScene *s)
{
    return model_panel(scenes_model(), s->panel_id);
}

static void success_on_enter(Scene *self)
{
    SuccessScene *s = (SuccessScene *)self;
    const PanelState *p = panel_of(s);
    s->frame = 0;
    s->complete = p->owned == panel_bits_all(p->piece_count);
    coin_counter_init(&s->coins, s->coins_before, ANIM_COIN_STEP, ANIM_COIN_MAX,
                      ANIM_COIN_SOUND_EVERY);
    coin_counter_set(&s->coins, s->coins_before - s->cost);
    focus_init(&s->focus, 0, true);
    hit_tap_reset(&s->tap);
    press_init(&s->press);
    s->rect = (HitRect){0, 0, 0, 0};
    audio_play(s->complete ? SFX_COMPLETE : SFX_BUY);
}

static void success_update(Scene *self, const InputFrame *in)
{
    SuccessScene *s = (SuccessScene *)self;
    u32 keys = input_focus(&s->focus, in);
    int tapped = input_tap(&s->tap, in, &s->rect, 1);

    s->frame++;
    if (coin_counter_step(&s->coins))
        audio_play(SFX_COIN);
    press_update(&s->press, hit_tap_pressed(&s->tap) == 0, ANIM_BUTTON_RELEASE);

    if ((keys & KEY_A) || tapped == 0) {
        audio_play(SFX_CONFIRM);
        if (s->complete)
            scene_pop(); /* back to the gallery */
        else
            scene_replace(scene_picker(s->panel_id, s->focus.visible));
    }
}

/* --- Top screen -------------------------------------------------------------- */

static ThemeColor confetti_color(int index)
{
    static const ThemeColor colors[CONFETTI_COLORS] = {THEME_PLAZA, THEME_COIN, THEME_RARE,
                                                       THEME_FOCUS};
    return colors[index];
}

static void draw_ribbon(const SuccessScene *s, GeomRect art)
{
    const char *text = s->complete ? str(STR_PUZZLE_COMPLETE) : str(STR_NEW_PIECES_BANNER);
    int text_w = draw_text_width(THEME_TEXT_TITLE, text);
    int w = text_w + 2 * THEME_RIBBON_PAD;
    int final_y = art.y - THEME_BOARD_FRAME - THEME_RIBBON_RISE;
    float t = (float)s->frame / (float)RIBBON_DROP;
    float e = easing_apply(EASE_OUT_BACK, t);
    /* From above the screen to its place, with the overshoot of ease-out-back. */
    int y = -THEME_RIBBON_H + (int)((float)(final_y + THEME_RIBBON_H) * e);
    GeomRect box = {(THEME_TOP_W - w) / 2, y, w, THEME_RIBBON_H};

    /* Folded ends behind the ribbon, then the ribbon itself. */
    draw_rect(box.x - THEME_RIBBON_TAIL, box.y + 6, THEME_RIBBON_TAIL + 6, THEME_RIBBON_H - 2,
              THEME_PLAZA_DARK, THEME_ALPHA_OPAQUE);
    draw_rect(box.x + box.w - 6, box.y + 6, THEME_RIBBON_TAIL + 6, THEME_RIBBON_H - 2,
              THEME_PLAZA_DARK, THEME_ALPHA_OPAQUE);
    draw_slice_tinted(DRAW_SLICE_ROUND_5, box, THEME_PLAZA_HI, THEME_PLAZA, THEME_ALPHA_OPAQUE);
    draw_text_vcenter(box.x + box.w / 2, box.y, box.h, THEME_TEXT_TITLE, THEME_WHITE,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, text);
}

static void success_draw_top(Scene *self, const StereoView *view)
{
    SuccessScene *s = (SuccessScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_of(s);
    unsigned total = p->piece_count;
    unsigned owned = panel_bits_count(p->owned);

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    char count[16];
    snprintf(count, sizeof(count), "%u/%u", owned, total);
    pill_name_draw(THEME_PILL_MARGIN, THEME_PILL_Y, p->name, count);
    GeomRect coins = pill_coin_draw(THEME_TOP_W - THEME_PILL_MARGIN, THEME_PILL_Y, true,
                                    coin_counter_value(&s->coins),
                                    coin_counter_shining(&s->coins) ? (u8)anim_focus_alpha() : 0);

    /* The price as a small badge under the coin display. */
    char delta[12];
    snprintf(delta, sizeof(delta), "−%u", s->cost);
    int dw = draw_text_width(THEME_TEXT_BUTTON, delta) + 2 * THEME_DELTA_PAD;
    GeomRect badge = {coins.x + coins.w - dw, coins.y + coins.h + 6, dw, THEME_DELTA_H};
    draw_slice_tinted(DRAW_SLICE_ROUND_8, badge, THEME_COIN, THEME_COIN_DARK, THEME_ALPHA_OPAQUE);
    draw_text_vcenter(badge.x + badge.w / 2, badge.y, badge.h, THEME_TEXT_BUTTON, THEME_WHITE,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, delta);
    (void)m;
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    BoardSpec board = {(int)total, p->owned, 0, p->rare, s->pieces, 0};
    board.fresh_frame = s->frame > BOARD_POP_DELAY ? s->frame - BOARD_POP_DELAY : 0;
    art_use(p->id);
    GeomRect art = board_draw(&board);
    art_use(ART_PLACEHOLDER);

    GeomRect track = {(THEME_TOP_W - THEME_PROGRESS_W) / 2, THEME_PROGRESS_Y, THEME_PROGRESS_W,
                      THEME_PROGRESS_H};
    progress_draw(track, owned, total, PROGRESS_ON_SKY, THEME_ALPHA_OPAQUE);
    char left[32], right[32];
    snprintf(left, sizeof(left), str(STR_PROGRESS_OF), owned, total);
    int ty = track.y + (track.h - draw_text_height(THEME_TEXT_BODY)) / 2;
    draw_text(track.x - THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2,
              THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT, left);
    if (owned < total) {
        snprintf(right, sizeof(right), str(STR_PROGRESS_MISSING), total - owned);
        draw_text(track.x + track.w + THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, right);
    }
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_POP);
    draw_ribbon(s, art);

    /* Sparks on the new pieces while they arrive. */
    const ThemeBoard *tb = theme_board((int)total);
    if (tb && s->frame > BOARD_POP_DELAY && s->frame < BOARD_POP_DELAY + 50) {
        int shown = 0;
        for (int i = 0; i < tb->pieces && shown < 3; i++) {
            if (!(s->pieces & ((uint64_t)1 << i)))
                continue;
            GeomRect c = {art.x + (i % tb->cols) * tb->cell, art.y + (i / tb->cols) * tb->cell,
                          tb->cell, tb->cell};
            draw_sprite(DRAW_SPARK, c.x + c.w - 8, c.y + 2, (u8)anim_focus_alpha());
            shown++;
        }
    }

    if (s->complete && s->frame >= CELEBRATION_START) {
        unsigned f = s->frame - CELEBRATION_START;
        for (unsigned i = 0; i < CONFETTI_COUNT; i++) {
            ConfettiBit b;
            if (confetti_at(f, i, THEME_TOP_W, THEME_SCREEN_H, &b))
                draw_rect(b.x, b.y, b.size, b.size, confetti_color(b.color), THEME_ALPHA_OPAQUE);
        }
        if (f < STAR_HOLD + STAR_FADE) {
            float t = easing_apply(EASE_OUT_BACK, (float)f / (float)ANIM_NEW_PIECE);
            u8 a = f < STAR_HOLD
                       ? THEME_ALPHA_OPAQUE
                       : (u8)(THEME_ALPHA_OPAQUE * (STAR_HOLD + STAR_FADE - f) / STAR_FADE);
            draw_sprite_scaled(DRAW_STAR, art.x + art.w / 2, art.y + art.h / 2,
                               t * (float)THEME_STAR_BIG, a);
        }
    }
    stereo_layer_end();
}

/* --- Bottom screen ------------------------------------------------------------ */

static void draw_backup_chip(int cx, int y, const char *name)
{
    char label[24], text[56];
    if (!backup_name_label(name, strings_language() == LANG_DE, label, sizeof(label)))
        return;
    snprintf(text, sizeof(text), str(STR_BACKUP_FROM), label);
    int badge = draw_sprite_width(DRAW_CHECK_BADGE);
    int w = THEME_PILL_PAD + badge + THEME_TEXT_GAP + draw_text_width(THEME_TEXT_BODY, text) +
            THEME_PILL_PAD;
    GeomRect box = {cx - w / 2, y, w, THEME_BACKUP_CHIP_H};
    draw_slice_tinted(DRAW_SLICE_ROUND_8, box, THEME_PLAZA_LIGHT, THEME_PLAZA_LIGHT,
                      THEME_ALPHA_OPAQUE);
    draw_sprite(DRAW_CHECK_BADGE, box.x + THEME_PILL_PAD,
                box.y + (box.h - draw_sprite_height(DRAW_CHECK_BADGE)) / 2, THEME_ALPHA_OPAQUE);
    draw_text_vcenter(box.x + THEME_PILL_PAD + badge + THEME_TEXT_GAP, box.y, box.h,
                      THEME_TEXT_BODY, THEME_PLAZA_DARK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, text);
}

static void success_draw_bottom(Scene *self)
{
    SuccessScene *s = (SuccessScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_of(s);
    chrome_header_draw(str(STR_TITLE_BOUGHT), NULL);

    GeomRect card = {THEME_NOTICE_MARGIN, THEME_NOTICE_Y, THEME_BOTTOM_W - 2 * THEME_NOTICE_MARGIN,
                     THEME_BAR_Y - THEME_NOTICE_Y - THEME_CARD_SHADOW - 2};
    CardView cv = {false, true, THEME_ALPHA_OPAQUE};
    card_draw(card, &cv);
    int cx = card.x + card.w / 2;

    /* Green badge with a check, popping in with the first new piece. */
    int ring = THEME_SUCCESS_BADGE + 2 * THEME_SUCCESS_RING;
    float pop = easing_apply(EASE_OUT_BACK, (float)s->frame / (float)ANIM_NEW_PIECE);
    int by = card.y + THEME_SUCCESS_GAP + ring / 2;
    draw_ellipse(cx - (int)((float)ring * pop) / 2, by - (int)((float)ring * pop) / 2,
                 (int)((float)ring * pop), (int)((float)ring * pop), THEME_PLAZA_LIGHT,
                 THEME_ALPHA_OPAQUE);
    draw_ellipse(cx - (int)((float)THEME_SUCCESS_BADGE * pop) / 2,
                 by - (int)((float)THEME_SUCCESS_BADGE * pop) / 2,
                 (int)((float)THEME_SUCCESS_BADGE * pop), (int)((float)THEME_SUCCESS_BADGE * pop),
                 THEME_PLAZA, THEME_ALPHA_OPAQUE);
    draw_sprite_scaled(DRAW_CHECK, cx, by, 2.0f * pop, THEME_ALPHA_OPAQUE);

    int y = card.y + THEME_SUCCESS_GAP + ring + THEME_SUCCESS_GAP;
    char pieces[24], headline[48], line[80], coins[24], coins_line[48];
    strings_new_pieces(pieces, sizeof(pieces), panel_bits_count(s->pieces));
    snprintf(headline, sizeof(headline), str(STR_BOUGHT_HEADLINE), pieces);
    draw_text_fit(cx, y, card.w - 2 * THEME_NOTICE_PAD, THEME_TEXT_TITLE, THEME_INK,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, headline);
    y += draw_text_height(THEME_TEXT_TITLE) + THEME_SUCCESS_GAP / 2;

    snprintf(line, sizeof(line), str(STR_BOUGHT_PROGRESS), p->name, panel_bits_count(p->owned),
             (unsigned)p->piece_count);
    draw_text_fit(cx, y, card.w - 2 * THEME_NOTICE_PAD, THEME_TEXT_BODY, THEME_INK_2,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, line);
    y += THEME_NOTICE_LINE_H;
    strings_coins(coins, sizeof(coins), m->coins);
    snprintf(coins_line, sizeof(coins_line), str(STR_COINS_LEFT), coins);
    draw_text_fit(cx, y, card.w - 2 * THEME_NOTICE_PAD, THEME_TEXT_BODY, THEME_INK_2,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, coins_line);
    y += THEME_NOTICE_LINE_H + THEME_SUCCESS_GAP / 2;

    draw_backup_chip(cx, y, s->backup);

    chrome_bar_draw();
    ButtonSpec next = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_CONTINUE)};
    ButtonView nv = {false, s->focus.visible, press_offset(&s->press)};
    GeomRect b = chrome_bar_button(&next, true, &nv);
    s->rect = (HitRect){b.x, b.y, b.w, b.h};
}

static SuccessScene s_success = {.base = {"S6 Kauf erfolgreich", success_on_enter, NULL,
                                          success_update, success_draw_top, success_draw_bottom}};

Scene *scene_success(unsigned panel_id, uint64_t pieces, unsigned cost, unsigned coins_before,
                     const char *backup)
{
    s_success.panel_id = panel_id;
    s_success.pieces = pieces;
    s_success.cost = cost;
    s_success.coins_before = coins_before;
    strncpy(s_success.backup, backup, sizeof(s_success.backup) - 1);
    s_success.backup[sizeof(s_success.backup) - 1] = '\0';
    return &s_success.base;
}
