/*
 * S8 Einstellungen und Info. Bottom: sound switch and
 * language, applied and saved at once. Top: version, license, thanks and the
 * notice that this is unofficial homebrew.
 */
#include <stdio.h>

#include "core/press.h"
#include "core/settings_menu.h"
#include "core/strings.h"
#include "core/toast.h"
#include "core/version.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/button.h"
#include "ui/widgets/card.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/sign.h"
#include "ui/widgets/toast_view.h"

enum {
    T_BACK,
    T_CHANGE,
    T_ROW0,
    T_COUNT = T_ROW0 + SETTINGS_ROW_COUNT,
};

typedef struct {
    Scene base;
    FocusState focus; /* index: the row */
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT];
    Toast toast;
} SettingsScene;

static void settings_on_enter(Scene *self)
{
    SettingsScene *s = (SettingsScene *)self;
    focus_init(&s->focus, SETTINGS_ROW_SOUND, true);
    hit_tap_reset(&s->tap);
    for (int i = 0; i < T_COUNT; i++) {
        press_init(&s->press[i]);
        s->rects[i] = (HitRect){0, 0, 0, 0};
    }
    toast_widget_init(&s->toast);
}

/* Applies a changed setting and writes settings.ini; the screen keeps the new value either way. */
static void commit(SettingsScene *s, const Settings *next)
{
    Model *m = scenes_model();
    if (next->sound != m->settings.sound)
        audio_set_enabled(next->sound);
    if (next->language != m->settings.language)
        scenes_apply_language(next->language);
    if (!model_save_settings(m, next)) {
        /* Not saved: the value holds until the app closes. */
        m->settings = *next;
        audio_play(SFX_ERROR);
        toast_show(&s->toast, str(STR_SETTINGS_SAVE_FAILED));
    }
}

static void change(SettingsScene *s, int row, int delta)
{
    Settings next = scenes_model()->settings;
    if (row == SETTINGS_ROW_SOUND)
        next.sound = !next.sound;
    else
        next.language = settings_menu_cycle_language(next.language, delta);
    commit(s, &next);
    /* A sound switched on is heard at once; the click confirms it. */
    audio_play(SFX_CONFIRM);
}

static void settings_update(Scene *self, const InputFrame *in)
{
    SettingsScene *s = (SettingsScene *)self;
    u32 keys = input_focus(&s->focus, in);
    int tapped = input_tap(&s->tap, in, s->rects, T_COUNT);

    int dir = input_direction(keys);
    if (dir == FOCUS_UP || dir == FOCUS_DOWN) {
        if (settings_menu_move(&s->focus.index, dir == FOCUS_UP ? -1 : 1))
            audio_play(SFX_MOVE);
        else
            audio_play(SFX_ERROR);
    }

    int row = s->focus.index;
    if (keys & KEY_B) {
        audio_play(SFX_BACK);
        scene_pop();
    } else if (tapped == T_BACK) {
        audio_play(SFX_BACK);
        scene_pop();
    } else if (tapped >= T_ROW0) {
        s->focus.index = tapped - T_ROW0;
        change(s, tapped - T_ROW0, 1);
    } else if ((keys & KEY_A) || tapped == T_CHANGE) {
        change(s, row, 1);
    } else if (row == SETTINGS_ROW_LANGUAGE && dir == FOCUS_LEFT) {
        change(s, row, -1);
    } else if (row == SETTINGS_ROW_LANGUAGE && dir == FOCUS_RIGHT) {
        change(s, row, 1);
    }

    toast_step(&s->toast);
    for (int i = 0; i < T_COUNT; i++)
        press_update(&s->press[i], hit_tap_pressed(&s->tap) == i, ANIM_BUTTON_RELEASE);
}

/* --- Top screen ------------------------------------------------------------- */

static void settings_draw_top(Scene *self, const StereoView *view)
{
    (void)self;
    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    sign_draw(str(STR_APP_NAME), str(STR_APP_TAGLINE), THEME_ALPHA_OPAQUE);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    char version[48];
    snprintf(version, sizeof(version), str(STR_VERSION_LINE), version_string());
    int cx = THEME_TOP_W / 2;
    int max_w = THEME_TOP_W - 2 * THEME_INFO_PAD;
    int y = THEME_INFO_CARD_Y + 100;
    draw_text(cx, y, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, version);
    y += THEME_INFO_LINE_H;
    draw_text(cx, y, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
              str(STR_INFO_LICENSE));
    y += THEME_INFO_LINE_H + THEME_INFO_GAP;
    y += THEME_INFO_LINE_H * draw_text_wrapped(cx, y, max_w, THEME_INFO_LINE_H, 2, THEME_TEXT_BODY,
                                               THEME_INK_3, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
                                               str(STR_INFO_THANKS));
    y += THEME_INFO_GAP;
    draw_text_fit(cx, y, max_w, THEME_TEXT_BODY, THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER,
                  str(STR_UNOFFICIAL));
    stereo_layer_end();
}

/* --- Bottom screen ---------------------------------------------------------- */

static void draw_switch(int right, int cy, bool on)
{
    GeomRect track = {right - THEME_SWITCH_W, cy - THEME_SWITCH_H / 2, THEME_SWITCH_W,
                      THEME_SWITCH_H};
    ThemeColor color = on ? THEME_PLAZA : THEME_TRACK;
    draw_slice_tinted(DRAW_SLICE_ROUND_8, track, color, color, THEME_ALPHA_OPAQUE);
    int knob_x = on ? track.x + track.w - THEME_SWITCH_INSET - THEME_SWITCH_KNOB
                    : track.x + THEME_SWITCH_INSET;
    draw_ellipse(knob_x, cy - THEME_SWITCH_KNOB / 2, THEME_SWITCH_KNOB, THEME_SWITCH_KNOB,
                 THEME_WHITE, THEME_ALPHA_OPAQUE);
}

static void draw_row(SettingsScene *s, int row, GeomRect box)
{
    const Settings *set = &scenes_model()->settings;
    CardView cv = {s->focus.visible && s->focus.index == row, true, THEME_ALPHA_OPAQUE};
    GeomRect drawn = box;
    drawn.y += press_offset(&s->press[T_ROW0 + row]);
    card_draw(drawn, &cv);

    int left = drawn.x + THEME_SETTING_PAD;
    int right = drawn.x + drawn.w - THEME_SETTING_PAD;
    StringId label = row == SETTINGS_ROW_SOUND ? STR_SETTINGS_SOUND : STR_SETTINGS_LANGUAGE;
    draw_text_vcenter(left, drawn.y, drawn.h, THEME_TEXT_HEADER, THEME_INK, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, str(label));

    if (row == SETTINGS_ROW_SOUND) {
        draw_switch(right, drawn.y + drawn.h / 2, set->sound);
        draw_text_vcenter(right - THEME_SWITCH_W - THEME_TEXT_GAP, drawn.y, drawn.h,
                          THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT,
                          str(set->sound ? STR_SWITCH_ON : STR_SWITCH_OFF));
    } else {
        draw_text_vcenter(right, drawn.y, drawn.h, THEME_TEXT_BUTTON, THEME_PLAZA_DARK,
                          THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT,
                          str(settings_menu_language_label(set->language)));
    }
    s->rects[T_ROW0 + row] = (HitRect){box.x, box.y, box.w, box.h};
}

static void settings_draw_bottom(Scene *self)
{
    SettingsScene *s = (SettingsScene *)self;
    chrome_header_draw(str(STR_TITLE_SETTINGS), NULL);

    for (int row = 0; row < SETTINGS_ROW_COUNT; row++) {
        GeomRect box = {THEME_CONTENT_PAD,
                        THEME_CONTENT_Y + THEME_CONTENT_PAD +
                            row * (THEME_SETTING_ROW_H + THEME_SETTING_ROW_GAP),
                        THEME_BOTTOM_W - 2 * THEME_CONTENT_PAD, THEME_SETTING_ROW_H};
        draw_row(s, row, box);
    }

    chrome_bar_draw();
    ButtonSpec back = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_BACK)};
    ButtonView bv = {false, false, press_offset(&s->press[T_BACK])};
    GeomRect b = chrome_bar_button(&back, false, &bv);
    s->rects[T_BACK] = (HitRect){b.x, b.y, b.w, b.h};

    ButtonSpec change = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_CHANGE)};
    ButtonView cv = {false, false, press_offset(&s->press[T_CHANGE])};
    b = chrome_bar_button(&change, true, &cv);
    s->rects[T_CHANGE] = (HitRect){b.x, b.y, b.w, b.h};

    toast_widget_draw(&s->toast);
}

static SettingsScene s_settings = {.base = {"S8 Einstellungen", settings_on_enter, NULL,
                                            settings_update, settings_draw_top,
                                            settings_draw_bottom}};

Scene *scene_settings(void)
{
    return &s_settings.base;
}
