/*
 * S1 Erste Sicherung. Without an original
 * backup nothing may be bought, so this comes before the gallery. A creates
 * the backup; B quits.
 */
#include <stdio.h>

#include "core/gamecoin.h"
#include "core/meet_save.h"
#include "core/onboarding.h"
#include "core/press.h"
#include "core/size_text.h"
#include "core/strings.h"
#include "core/toast.h"
#include "core/version.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/quit_dialog.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/button.h"
#include "ui/widgets/card.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/sign.h"
#include "ui/widgets/toast_view.h"

enum { T_QUIT, T_CREATE, T_COUNT };

typedef struct {
    Scene base;
    Onboarding flow;
    FocusState focus;
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT]; /* where the buttons were drawn last frame */
    Toast toast;
    QuitDialog quit;
} OnbScene;

static void onb_on_enter(Scene *self)
{
    OnbScene *s = (OnbScene *)self;
    onboarding_init(&s->flow);
    focus_init(&s->focus, T_CREATE, true);
    hit_tap_reset(&s->tap);
    for (int i = 0; i < T_COUNT; i++) {
        press_init(&s->press[i]);
        s->rects[i] = (HitRect){0, 0, 0, 0};
    }
    toast_widget_init(&s->toast);
    quit_dialog_init(&s->quit);
}

static void onb_update(Scene *self, const InputFrame *in)
{
    OnbScene *s = (OnbScene *)self;
    u32 keys = input_focus(&s->focus, in);
    if (quit_dialog_active(&s->quit)) {
        quit_dialog_update(&s->quit, in, keys);
        return;
    }
    int tapped = input_tap(&s->tap, in, s->rects, T_COUNT);

    if (!onboarding_busy(&s->flow)) {
        int dir = input_direction(keys);
        int target = dir == FOCUS_LEFT ? T_QUIT : dir == FOCUS_RIGHT ? T_CREATE : s->focus.index;
        if (target != s->focus.index) {
            s->focus.index = target;
            audio_play(SFX_MOVE);
        }

        int act = tapped;
        if (keys & KEY_A)
            act = s->focus.index;
        if (keys & KEY_B)
            act = T_QUIT;
        if (act == T_QUIT) {
            audio_play(SFX_CONFIRM);
            quit_dialog_open(&s->quit); /* B never leaves the app by itself */
        } else if (act == T_CREATE && onboarding_start(&s->flow)) {
            audio_play(SFX_CONFIRM);
        }
    }

    if (onboarding_work_due(&s->flow)) {
        bool ok = model_create_original(scenes_model()) == BACKUP_OK;
        onboarding_finish(&s->flow, ok);
        if (ok) {
            audio_play(SFX_BUY);
        } else {
            audio_play(SFX_ERROR);
            toast_show(&s->toast, str(STR_BACKUP_FAILED));
        }
    }
    onboarding_step(&s->flow);
    toast_step(&s->toast);

    for (int i = 0; i < T_COUNT; i++)
        press_update(&s->press[i], hit_tap_pressed(&s->tap) == i, ANIM_BUTTON_RELEASE);

    if (s->flow.phase == ONB_DONE)
        scenes_go(route_after_load(scenes_model()->status));
}

static void onb_draw_top(Scene *self, const StereoView *view)
{
    OnbScene *s = (OnbScene *)self;
    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    sign_draw(str(STR_APP_NAME), str(STR_APP_TAGLINE), THEME_ALPHA_OPAQUE);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    sign_version_draw(version_string());
    stereo_layer_end();
    quit_dialog_draw_top(&s->quit);
}

static void draw_file_row(GeomRect row, StringId label, const char *file, uint32_t bytes,
                          bool green, bool working)
{
    draw_slice_tinted(DRAW_SLICE_ROUND_5, row, THEME_LINE, THEME_LINE, THEME_ALPHA_OPAQUE);
    draw_slice_tinted(DRAW_SLICE_ROUND_5, geom_rect_inset(row, 1), THEME_PLAZA_MIST,
                      THEME_PLAZA_MIST, THEME_ALPHA_OPAQUE);

    int ring = THEME_FILE_DOT + 2 * THEME_FILE_DOT_RING;
    int dx = row.x + THEME_FILE_DOT_PAD;
    int dy = row.y + (row.h - THEME_FILE_DOT) / 2;
    u8 alpha = working ? (u8)anim_focus_alpha() : THEME_ALPHA_OPAQUE;
    if (green || working) {
        draw_ellipse(dx - THEME_FILE_DOT_RING, dy - THEME_FILE_DOT_RING, ring, ring,
                     THEME_PLAZA_LIGHT, alpha);
        draw_ellipse(dx, dy, THEME_FILE_DOT, THEME_FILE_DOT, THEME_PLAZA, alpha);
    } else {
        draw_ellipse(dx, dy, THEME_FILE_DOT, THEME_FILE_DOT, THEME_INK_3, THEME_ALPHA_OPAQUE);
    }

    char size[16], right[40];
    size_text_format(size, sizeof(size), bytes);
    snprintf(right, sizeof(right), "%s · %s", file, size);
    int pad = THEME_FILE_DOT_PAD + THEME_FILE_DOT + THEME_FILE_DOT_PAD;
    int right_w = draw_text_width(THEME_TEXT_BODY, right);
    int right_x = row.x + row.w - THEME_FILE_DOT_PAD;
    draw_text_vcenter(right_x, row.y, row.h, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_RIGHT, right);
    draw_text_fit(row.x + pad, row.y + (row.h - draw_text_height(THEME_TEXT_BUTTON)) / 2,
                  row.w - pad - right_w - 2 * THEME_FILE_DOT_PAD, THEME_TEXT_BUTTON, THEME_INK,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, str(label));
}

static void onb_draw_bottom(Scene *self)
{
    OnbScene *s = (OnbScene *)self;
    chrome_header_draw(str(STR_TITLE_ONBOARDING), NULL);

    GeomRect card = {THEME_NOTICE_MARGIN, THEME_NOTICE_Y, THEME_BOTTOM_W - 2 * THEME_NOTICE_MARGIN,
                     THEME_NOTICE_H};
    CardView cv = {false, true, THEME_ALPHA_OPAQUE};
    card_draw(card, &cv);
    draw_text_wrapped(card.x + THEME_NOTICE_PAD, card.y + THEME_NOTICE_PAD,
                      card.w - 2 * THEME_NOTICE_PAD, THEME_NOTICE_LINE_H, 3, THEME_TEXT_BODY,
                      THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, str(STR_ONBOARDING_BODY));

    bool working = s->flow.phase == ONB_WORKING;
    int rows_h = 2 * THEME_FILE_ROW_H + THEME_FILE_ROW_GAP;
    GeomRect row = {card.x + THEME_NOTICE_PAD, card.y + card.h - THEME_NOTICE_PAD - rows_h,
                    card.w - 2 * THEME_NOTICE_PAD, THEME_FILE_ROW_H};
    draw_file_row(row, STR_FILE_MEET, "meet.dat", MEET_SAVE_SIZE, onboarding_dot_green(&s->flow, 0),
                  working);
    row.y += THEME_FILE_ROW_H + THEME_FILE_ROW_GAP;
    draw_file_row(row, STR_FILE_COINS, "gamecoin.dat", GAMECOIN_SIZE,
                  onboarding_dot_green(&s->flow, 1), working);

    chrome_bar_draw();
    bool busy = onboarding_busy(&s->flow);
    bool show_focus = s->focus.visible && !busy;

    ButtonSpec quit = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_QUIT)};
    ButtonView qv = {busy, show_focus && s->focus.index == T_QUIT, press_offset(&s->press[T_QUIT])};
    GeomRect b = chrome_bar_button(&quit, false, &qv);
    s->rects[T_QUIT] = (HitRect){b.x, b.y, b.w, b.h};

    ButtonSpec create = {BUTTON_PRIMARY, DRAW_GLYPH_A,
                         working ? str(STR_BACKUP_WORKING) : str(STR_BTN_CREATE_BACKUP)};
    ButtonView cr = {busy, show_focus && s->focus.index == T_CREATE,
                     press_offset(&s->press[T_CREATE])};
    b = chrome_bar_button(&create, true, &cr);
    s->rects[T_CREATE] = busy ? (HitRect){0, 0, 0, 0} : (HitRect){b.x, b.y, b.w, b.h};
    if (busy)
        s->rects[T_QUIT] = (HitRect){0, 0, 0, 0};

    quit_dialog_draw(&s->quit, s->focus.visible);
    toast_widget_draw(&s->toast);
}

static bool onb_blocks_start(const Scene *self)
{
    const OnbScene *s = (const OnbScene *)self;
    return onboarding_busy(&s->flow) || quit_dialog_active(&s->quit);
}

static OnbScene s_onb = {.base = {"S1 Erste Sicherung", onb_on_enter, NULL, onb_update,
                                  onb_draw_top, onb_draw_bottom, onb_blocks_start}};

Scene *scene_onboarding(void)
{
    return &s_onb.base;
}
