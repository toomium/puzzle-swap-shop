/*
 * Error screens: the layout of
 * S1 with one sentence and one or two actions. E-01, E-02, E-03 and E-06
 * can appear at start; E-04 and E-05 after a purchase, with the gallery
 * below them on the scene stack.
 */
#include "core/press.h"
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

enum { T_SECOND, T_MAIN, T_COUNT }; /* left button, right (primary) button */

typedef struct {
    StringId body;
    StringId main_label;
    const char *main_glyph;
    StringId second_label; /* STR_COUNT: no second button */
    const char *second_glyph;
} NoticeLayout;

/* Developer preview (error_preview.c): -1 follows the model, 0/1 forces "journal readable". */
static int s_preview_journal = -1;

static bool journal_readable(void)
{
    if (s_preview_journal >= 0)
        return s_preview_journal != 0;
    return scenes_model()->recovery == RECOVERY_INTERRUPTED;
}

static const NoticeLayout *layout_for(Route route)
{
    static const NoticeLayout quit_only = {STR_E01, STR_BTN_QUIT, DRAW_GLYPH_A, STR_COUNT, NULL};
    static const NoticeLayout not_installed = {STR_E02, STR_BTN_QUIT, DRAW_GLYPH_A, STR_COUNT,
                                               NULL};
    static const NoticeLayout to_backups = {STR_E05, STR_BTN_TO_BACKUPS, DRAW_GLYPH_A, STR_COUNT,
                                            NULL};
    static const NoticeLayout read_only = {STR_E03, STR_BTN_VIEW_ONLY, DRAW_GLYPH_A, STR_BTN_QUIT,
                                           DRAW_GLYPH_X};
    static const NoticeLayout interrupted = {STR_E06, STR_BTN_RESTORE, DRAW_GLYPH_A, STR_BTN_KEEP,
                                             DRAW_GLYPH_X};
    static const NoticeLayout failed = {STR_E04, STR_BTN_OK, DRAW_GLYPH_A, STR_COUNT, NULL};
    static const NoticeLayout undo_failed = {STR_E05, STR_BTN_RESTORE, DRAW_GLYPH_A, STR_COUNT,
                                             NULL};
    switch (route) {
    case ROUTE_E02:
        return &not_installed;
    case ROUTE_E04:
        return &failed;
    case ROUTE_E05:
        /* Without a readable journal the backup is not known: the user picks one in S7. */
        return journal_readable() ? &undo_failed : &to_backups;
    case ROUTE_E03:
        return &read_only;
    case ROUTE_E06:
        return &interrupted;
    default:
        return &quit_only;
    }
}

typedef struct {
    Scene base;
    Route route;
    bool preview; /* shown from the developer list: buttons change nothing */
    FocusState focus;
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT];
    Toast toast;
} NoticeScene;

static void notice_on_enter(Scene *self)
{
    NoticeScene *s = (NoticeScene *)self;
    focus_init(&s->focus, T_MAIN, true);
    hit_tap_reset(&s->tap);
    for (int i = 0; i < T_COUNT; i++) {
        press_init(&s->press[i]);
        s->rects[i] = (HitRect){0, 0, 0, 0};
    }
    toast_widget_init(&s->toast);
}

static void notice_main(NoticeScene *s)
{
    Model *m = scenes_model();
    switch (s->route) {
    case ROUTE_E03:
        audio_play(SFX_CONFIRM);
        scenes_go(ROUTE_GALLERY);
        break;
    case ROUTE_E04:
        audio_play(SFX_CONFIRM);
        scene_pop(); /* the gallery */
        break;
    case ROUTE_E05:
        if (m->recovery != RECOVERY_INTERRUPTED) {
            audio_play(SFX_CONFIRM);
            scene_replace(scene_backups());
            break;
        }
        audio_play(SFX_CONFIRM);
        if (model_resolve_interrupted(m, true) == TXN_OK) {
            scene_pop();
        } else {
            audio_play(SFX_ERROR);
            toast_show(&s->toast, str(STR_RESTORE_FAILED));
        }
        break;
    case ROUTE_E06:
        audio_play(SFX_CONFIRM);
        if (model_resolve_interrupted(m, true) == TXN_OK) {
            scenes_go(route_after_load(m->status));
        } else {
            audio_play(SFX_ERROR);
            toast_show(&s->toast, str(STR_RESTORE_FAILED));
        }
        break;
    default:
        audio_play(SFX_BACK);
        scenes_request_quit();
        break;
    }
}

static void notice_second(NoticeScene *s)
{
    Model *m = scenes_model();
    if (s->route == ROUTE_E06) {
        audio_play(SFX_BACK);
        if (model_resolve_interrupted(m, false) == TXN_OK) {
            scenes_go(route_after_load(m->status));
        } else {
            audio_play(SFX_ERROR);
            toast_show(&s->toast, str(STR_RESTORE_FAILED));
        }
    } else {
        audio_play(SFX_BACK);
        scenes_request_quit();
    }
}

static void notice_update(Scene *self, const InputFrame *in)
{
    NoticeScene *s = (NoticeScene *)self;
    const NoticeLayout *lay = layout_for(s->route);
    bool two = lay->second_label != STR_COUNT;

    u32 keys = input_focus(&s->focus, in);
    int tapped = input_tap(&s->tap, in, s->rects, T_COUNT);

    int dir = input_direction(keys);
    int target = s->focus.index;
    if (two && dir == FOCUS_LEFT)
        target = T_SECOND;
    else if (dir == FOCUS_RIGHT)
        target = T_MAIN;
    if (target != s->focus.index) {
        s->focus.index = target;
        audio_play(SFX_MOVE);
    }

    int act = tapped;
    if (s->preview && (keys & KEY_B)) {
        audio_play(SFX_BACK);
        scene_pop();
        return;
    }
    if (keys & KEY_A)
        act = s->focus.index;
    /* B does nothing here: leaving the app, or deciding about the journal, takes a button. */
    if ((keys & KEY_X) && two)
        act = T_SECOND;

    if (s->preview && act >= 0) { /* never touches the model or the game data */
        audio_play(SFX_BACK);
        scene_pop();
        return;
    }
    if (act == T_MAIN)
        notice_main(s);
    else if (act == T_SECOND && two)
        notice_second(s);

    toast_step(&s->toast);
    for (int i = 0; i < T_COUNT; i++)
        press_update(&s->press[i], hit_tap_pressed(&s->tap) == i, ANIM_BUTTON_RELEASE);
}

static void notice_draw_top(Scene *self, const StereoView *view)
{
    (void)self;
    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    sign_draw(str(STR_APP_NAME), str(STR_APP_TAGLINE), THEME_ALPHA_OPAQUE);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    sign_version_draw(version_string());
    stereo_layer_end();
}

static void notice_draw_bottom(Scene *self)
{
    NoticeScene *s = (NoticeScene *)self;
    const NoticeLayout *lay = layout_for(s->route);

    chrome_header_draw(str(STR_APP_NAME), NULL);
    GeomRect card = {THEME_NOTICE_MARGIN, THEME_NOTICE_Y, THEME_BOTTOM_W - 2 * THEME_NOTICE_MARGIN,
                     THEME_NOTICE_H};
    CardView cv = {false, true, THEME_ALPHA_OPAQUE};
    card_draw(card, &cv);
    draw_text_wrapped(card.x + THEME_NOTICE_PAD, card.y + THEME_NOTICE_PAD,
                      card.w - 2 * THEME_NOTICE_PAD, THEME_NOTICE_LINE_H, 6, THEME_TEXT_BODY,
                      THEME_INK, THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, str(lay->body));

    chrome_bar_draw();
    s->rects[T_SECOND] = (HitRect){0, 0, 0, 0};
    if (lay->second_label != STR_COUNT) {
        ButtonSpec second = {BUTTON_SECONDARY, lay->second_glyph, str(lay->second_label)};
        ButtonView sv = {false, s->focus.visible && s->focus.index == T_SECOND,
                         press_offset(&s->press[T_SECOND])};
        GeomRect b = chrome_bar_button(&second, false, &sv);
        s->rects[T_SECOND] = (HitRect){b.x, b.y, b.w, b.h};
    }
    ButtonSpec main_spec = {BUTTON_PRIMARY, lay->main_glyph, str(lay->main_label)};
    ButtonView mv = {false, s->focus.visible && s->focus.index == T_MAIN,
                     press_offset(&s->press[T_MAIN])};
    GeomRect b = chrome_bar_button(&main_spec, true, &mv);
    s->rects[T_MAIN] = (HitRect){b.x, b.y, b.w, b.h};

    toast_widget_draw(&s->toast);
}

static NoticeScene s_notice_scene = {
    .base = {"Fehler", notice_on_enter, NULL, notice_update, notice_draw_top, notice_draw_bottom},
    .route = ROUTE_E01};

Scene *scene_notice(Route route)
{
    s_notice_scene.route = route;
    s_notice_scene.preview = false;
    s_preview_journal = -1;
    return &s_notice_scene.base;
}

Scene *scene_notice_preview(Route route, bool journal_readable)
{
    s_notice_scene.route = route;
    s_notice_scene.preview = true;
    s_preview_journal = journal_readable ? 1 : 0;
    return &s_notice_scene.base;
}
