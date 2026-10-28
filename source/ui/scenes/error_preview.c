/*
 * Developer view of the error screens: one button per screen, so that
 * each can be looked at without breaking anything on the console. Opened with
 * the chip "Fehler" in the smoke test. The captions only exist here (as in the smoke test).
 */
#include "core/press.h"
#include "core/strings.h"
#include "core/version.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/button.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/sign.h"

typedef struct {
    const char *label;
    Route route;
    bool journal; /* E-05: the journal can be read */
} Entry;

static const Entry k_entries[] = {
    {"E-01 Archiv", ROUTE_E01, false},       {"E-02 Mii Plaza", ROUTE_E02, false},
    {"E-03 Unbekannt", ROUTE_E03, false},    {"E-04 Kauf", ROUTE_E04, false},
    {"E-05 mit Journal", ROUTE_E05, true},   {"E-05 ohne Journal", ROUTE_E05, false},
    {"E-06 Unterbrochen", ROUTE_E06, false},
};
#define ENTRY_COUNT ((int)(sizeof(k_entries) / sizeof(k_entries[0])))
#define T_BACK      ENTRY_COUNT /* the bar button comes after the entries */

typedef struct {
    Scene base;
    FocusState focus;
    HitTap tap;
    Press press[ENTRY_COUNT + 1];
    HitRect rects[ENTRY_COUNT + 1];
} PreviewScene;

static void preview_on_enter(Scene *self)
{
    PreviewScene *s = (PreviewScene *)self;
    focus_init(&s->focus, 0, true);
    hit_tap_reset(&s->tap);
    for (int i = 0; i <= ENTRY_COUNT; i++) {
        press_init(&s->press[i]);
        s->rects[i] = (HitRect){0, 0, 0, 0};
    }
}

static void preview_open(int i)
{
    audio_play(SFX_CONFIRM);
    scene_push(scene_notice_preview(k_entries[i].route, k_entries[i].journal));
}

static void preview_update(Scene *self, const InputFrame *in)
{
    PreviewScene *s = (PreviewScene *)self;
    u32 keys = input_focus(&s->focus, in);
    int tapped = input_tap(&s->tap, in, s->rects, ENTRY_COUNT + 1);

    int dir = input_direction(keys);
    if (dir >= 0 && s->focus.index < ENTRY_COUNT) {
        if (focus_grid_move(&s->focus.index, THEME_PREVIEW_COLS, ENTRY_COUNT, (FocusDir)dir, 0) ==
            FOCUS_MOVED)
            audio_play(SFX_MOVE);
    }

    if ((keys & KEY_B) || tapped == T_BACK) {
        audio_play(SFX_BACK);
        scene_pop();
        return;
    }
    if (tapped >= 0)
        preview_open(tapped);
    else if ((keys & KEY_A) && s->focus.index < ENTRY_COUNT)
        preview_open(s->focus.index);

    for (int i = 0; i <= ENTRY_COUNT; i++)
        press_update(&s->press[i], hit_tap_pressed(&s->tap) == i, ANIM_BUTTON_RELEASE);
}

static void preview_draw_top(Scene *self, const StereoView *view)
{
    (void)self;
    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    sign_draw(str(STR_APP_NAME), "Fehlerbildschirme", THEME_ALPHA_OPAQUE);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    sign_version_draw(version_string());
    stereo_layer_end();
}

static void preview_draw_bottom(Scene *self)
{
    PreviewScene *s = (PreviewScene *)self;
    chrome_header_draw("Fehler ansehen", NULL);

    int col_w = (THEME_BOTTOM_W - 2 * THEME_PREVIEW_X) / THEME_PREVIEW_COLS;
    for (int i = 0; i < ENTRY_COUNT; i++) {
        ButtonSpec spec = {BUTTON_SECONDARY, NULL, k_entries[i].label};
        ButtonView v = {false, s->focus.visible && s->focus.index == i, press_offset(&s->press[i])};
        int x = THEME_PREVIEW_X + (i % THEME_PREVIEW_COLS) * col_w;
        int y = THEME_NOTICE_Y + (i / THEME_PREVIEW_COLS) * THEME_PREVIEW_ROW_H;
        GeomRect b = button_draw(&spec, x, y, &v);
        s->rects[i] = (HitRect){b.x, b.y, b.w, b.h};
    }

    chrome_bar_draw();
    ButtonSpec back = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_BACK)};
    ButtonView bv = {false, false, press_offset(&s->press[T_BACK])};
    GeomRect b = chrome_bar_button(&back, false, &bv);
    s->rects[T_BACK] = (HitRect){b.x, b.y, b.w, b.h};
}

static PreviewScene s_preview = {.base = {"Fehler ansehen", preview_on_enter, NULL, preview_update,
                                          preview_draw_top, preview_draw_bottom}};

Scene *scene_error_preview(void)
{
    return &s_preview.base;
}
