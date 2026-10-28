/*
 * S2 Puzzle-Galerie. Bottom: 3 x 2 puzzle
 * cards per page, L and R turn the page. Top: the focused puzzle as a big
 * board with its progress. A or a tap opens the piece picker.
 */
#include <stdio.h>

#include "core/gallery_nav.h"
#include "core/panel_bits.h"
#include "core/press.h"
#include "core/strings.h"
#include "core/toast.h"
#include "core/tween.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/quit_dialog.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/art.h"
#include "ui/widgets/board.h"
#include "ui/widgets/button.h"
#include "ui/widgets/card.h"
#include "ui/widgets/chip.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/pill.h"
#include "ui/widgets/progress.h"
#include "ui/widgets/toast_view.h"

#define HINT_DELAY_FRAMES 30

enum {
    T_QUIT,
    T_OPEN,
    T_BACKUPS,
    T_PAGE_PREV,
    T_PAGE_NEXT,
    T_CARD0,
    T_COUNT = T_CARD0 + GALLERY_PER_PAGE,
};

typedef struct {
    Scene base;
    bool ready;
    unsigned pos;      /* focused entry, in gallery order */
    unsigned panel_id; /* the panel there, to find it again after the order changed */
    FocusState focus;
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT];
    int ids[T_COUNT];
    int rect_count;
    Tween slide; /* 0 -> 1 while the cards move to another page */
    int slide_dir;
    unsigned from_page;
    Toast toast;
    unsigned hint_wait; /* frames until the missing-pictures toast; 0 = none pending */
    QuitDialog quit;
} GalleryScene;

static const PanelState *panel_at(unsigned pos)
{
    return model_panel(scenes_model(), scenes_model()->gallery[pos]);
}

static void gallery_on_enter(Scene *self)
{
    GalleryScene *s = (GalleryScene *)self;
    Model *m = scenes_model();
    if (!s->ready) {
        s->ready = true;
        s->pos = 0;
        focus_init(&s->focus, 0, true);
        tween_start(&s->slide, 0.0f, 1.0f, 0, 0, EASE_OUT_CUBIC);
        for (int i = 0; i < T_COUNT; i++)
            press_init(&s->press[i]);
        toast_widget_init(&s->toast);
        quit_dialog_init(&s->quit);
        /* Every start: the pictures of the built-in puzzles come from the SD card. The toast
         * waits until the scene has settled, so it is not used up while the screen fades in. */
        s->hint_wait = model_builtin_pictures_missing(m) > 0 ? HINT_DELAY_FRAMES : 0;
    } else {
        /* Back from the picker: a purchase may have moved the puzzle in the list. */
        for (unsigned i = 0; i < PANEL_COUNT; i++) {
            if (m->gallery[i] == s->panel_id)
                s->pos = i;
        }
    }
    s->panel_id = m->gallery[s->pos];
    s->rect_count = 0;
    hit_tap_reset(&s->tap);
}

static void turn_page(GalleryScene *s, int delta)
{
    unsigned from = gallery_page_of(s->pos);
    if (!gallery_page_step(&s->pos, delta, PANEL_COUNT)) {
        audio_play(SFX_ERROR);
        return;
    }
    s->from_page = from;
    s->slide_dir = delta;
    anim_page_slide(&s->slide);
    s->panel_id = scenes_model()->gallery[s->pos];
    audio_play(SFX_PAGE);
}

static void open_focused(GalleryScene *s, bool by_key)
{
    if (!panel_at(s->pos)->available) {
        audio_play(SFX_ERROR);
        toast_show(&s->toast, str(STR_PANEL_UNAVAILABLE));
        return;
    }
    audio_play(SFX_CONFIRM);
    scene_push(scene_picker(scenes_model()->gallery[s->pos], by_key && s->focus.visible));
}

static void gallery_update(Scene *self, const InputFrame *in)
{
    GalleryScene *s = (GalleryScene *)self;
    u32 keys = input_focus(&s->focus, in);
    if (s->hint_wait > 0 && --s->hint_wait == 0)
        toast_show(&s->toast, str(STR_PICTURES_MISSING));
    if (quit_dialog_active(&s->quit)) {
        quit_dialog_update(&s->quit, in, keys);
        toast_step(&s->toast);
        return;
    }
    int hit = input_tap(&s->tap, in, s->rects, s->rect_count);
    int tapped = hit >= 0 ? s->ids[hit] : -1;

    if (tween_done(&s->slide)) {
        int dir = input_direction(keys);
        if (dir >= 0) {
            unsigned before = s->pos;
            if (gallery_move(&s->pos, (FocusDir)dir, PANEL_COUNT) == FOCUS_MOVED &&
                s->pos != before) {
                s->panel_id = scenes_model()->gallery[s->pos];
                audio_play(SFX_MOVE);
            }
        }
        if (keys & KEY_L)
            turn_page(s, -1);
        if (keys & KEY_R)
            turn_page(s, 1);
        if (tapped == T_PAGE_PREV)
            turn_page(s, -1);
        if (tapped == T_PAGE_NEXT)
            turn_page(s, 1);

        if ((keys & KEY_B) || tapped == T_QUIT) {
            audio_play(SFX_CONFIRM);
            quit_dialog_open(&s->quit); /* B never leaves the app by itself */
        } else if ((keys & KEY_X) || tapped == T_BACKUPS) {
            audio_play(SFX_CONFIRM);
            scene_push(scene_backups());
        } else if (keys & KEY_A) {
            open_focused(s, true);
        } else if (tapped == T_OPEN) {
            open_focused(s, false);
        } else if (tapped >= T_CARD0) {
            s->pos = gallery_page_of(s->pos) * GALLERY_PER_PAGE + (unsigned)(tapped - T_CARD0);
            s->panel_id = scenes_model()->gallery[s->pos];
            open_focused(s, false);
        }
        if (in->down & KEY_SELECT)
            scenes_push_smoke(); /* developer view of all widgets */
    }

    tween_step(&s->slide);
    toast_step(&s->toast);
    int p = hit_tap_pressed(&s->tap);
    int pressed = p >= 0 && p < s->rect_count ? s->ids[p] : -1;
    for (int i = 0; i < T_COUNT; i++)
        press_update(&s->press[i], pressed == i, ANIM_BUTTON_RELEASE);
}

/* --- Top screen -------------------------------------------------------------- */

static void gallery_draw_top(Scene *self, const StereoView *view)
{
    GalleryScene *s = (GalleryScene *)self;
    const Model *m = scenes_model();
    const PanelState *p = panel_at(s->pos);
    unsigned owned = p->available ? panel_bits_count(p->owned) : 0;
    unsigned total = p->piece_count;

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    char pieces[24];
    strings_pieces(pieces, sizeof(pieces), total);
    pill_name_draw(THEME_PILL_MARGIN, THEME_PILL_Y, p->name, pieces);
    pill_coin_draw(THEME_TOP_W - THEME_PILL_MARGIN, THEME_PILL_Y, true, m->coins, 0);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    BoardSpec board = {(int)total, p->available ? p->owned : 0, 0, p->rare, 0, 0};
    art_use(p->available ? p->id : ART_PLACEHOLDER);
    board_draw(&board);
    art_use(ART_PLACEHOLDER);

    GeomRect track = {(THEME_TOP_W - THEME_PROGRESS_W) / 2, THEME_PROGRESS_Y, THEME_PROGRESS_W,
                      THEME_PROGRESS_H};
    int ty = track.y + (track.h - draw_text_height(THEME_TEXT_BODY)) / 2;
    if (p->available) {
        progress_draw(track, owned, total, PROGRESS_ON_SKY, THEME_ALPHA_OPAQUE);
        char left[32], right[32];
        snprintf(left, sizeof(left), str(STR_PROGRESS_OF), owned, total);
        draw_text(track.x - THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT, left);
        if (owned < total) {
            snprintf(right, sizeof(right), str(STR_PROGRESS_MISSING), total - owned);
            draw_text(track.x + track.w + THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, right);
        }
    } else {
        draw_text(THEME_TOP_W / 2, ty, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_CENTER, str(STR_PANEL_UNAVAILABLE));
    }
    stereo_layer_end();
    quit_dialog_draw_top(&s->quit);
}

/* --- Bottom screen ------------------------------------------------------------ */

static void register_target(GalleryScene *s, int id, GeomRect box)
{
    if (s->rect_count >= T_COUNT)
        return;
    s->rects[s->rect_count] = (HitRect){box.x, box.y, box.w, box.h};
    s->ids[s->rect_count] = id;
    s->rect_count++;
}

/* The cards of one page, shifted by dx. Only the settled page takes touches. */
static void draw_page(GalleryScene *s, unsigned page, int dx, bool settled)
{
    unsigned items = gallery_page_items(PANEL_COUNT, page);
    draw_translate_push(dx, 0);
    for (unsigned i = 0; i < items; i++) {
        unsigned pos = page * GALLERY_PER_PAGE + i;
        const PanelState *p = panel_at(pos);
        GeomRect box = {
            THEME_GALLERY_X + (int)(i % GALLERY_COLS) * (THEME_PUZZLE_CARD_W + THEME_CARD_GAP),
            THEME_GALLERY_Y + (int)(i / GALLERY_COLS) * (THEME_PUZZLE_CARD_H + THEME_CARD_GAP),
            THEME_PUZZLE_CARD_W, THEME_PUZZLE_CARD_H};
        int lift = settled ? press_offset(&s->press[T_CARD0 + i]) : 0;
        GeomRect drawn = box;
        drawn.y += lift;
        PuzzleCardSpec spec = {p->name, p->available ? panel_bits_count(p->owned) : 0,
                               p->piece_count};
        art_use(p->available ? p->id : ART_PLACEHOLDER);
        puzzle_card_draw(drawn, &spec, settled && s->focus.visible && pos == s->pos, p->available);
        art_use(ART_PLACEHOLDER);
        if (settled) /* a closed card is tappable too: it explains why it stays closed */
            register_target(s, T_CARD0 + (int)i, box);
    }
    draw_translate_pop();
}

static void draw_pager(GalleryScene *s)
{
    GeomRect box = {THEME_BOTTOM_W - THEME_CONTENT_PAD - THEME_PAGER_W, THEME_PAGER_Y,
                    THEME_PAGER_W, THEME_CHIP_H};
    draw_slice(DRAW_SLICE_PILL, box, THEME_ALPHA_OPAQUE);
    draw_text_vcenter(box.x + THEME_PAGER_PAD, box.y, box.h, THEME_GLYPH_PX, THEME_INK,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, DRAW_GLYPH_L);
    draw_text_vcenter(box.x + box.w - THEME_PAGER_PAD, box.y, box.h, THEME_GLYPH_PX, THEME_INK,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT, DRAW_GLYPH_R);
    char num[16];
    snprintf(num, sizeof(num), "%u/%u", gallery_page_of(s->pos) + 1, gallery_pages(PANEL_COUNT));
    draw_text_vcenter(box.x + box.w / 2, box.y, box.h, THEME_TEXT_BODY, THEME_INK_2,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, num);

    GeomRect half = {box.x, box.y, box.w / 2, box.h};
    register_target(s, T_PAGE_PREV, half);
    half.x += half.w;
    register_target(s, T_PAGE_NEXT, half);
}

static void gallery_draw_bottom(Scene *self)
{
    GalleryScene *s = (GalleryScene *)self;
    s->rect_count = 0;

    bool sliding = !tween_done(&s->slide);
    unsigned page = gallery_page_of(s->pos);
    GeomRect area = chrome_content();
    draw_clip_push(area);
    if (sliding) {
        float t = tween_value(&s->slide);
        int span = THEME_BOTTOM_W;
        draw_page(s, s->from_page, (int)(-s->slide_dir * t * (float)span), false);
        draw_page(s, page, (int)(s->slide_dir * (1.0f - t) * (float)span), false);
    } else {
        draw_page(s, page, 0, true);
    }
    draw_clip_pop();

    chrome_header_draw(str(STR_TITLE_GALLERY), NULL);
    draw_pager(s);

    chrome_bar_draw();
    ButtonSpec quit = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_QUIT)};
    ButtonView qv = {false, false, press_offset(&s->press[T_QUIT])};
    GeomRect b = chrome_bar_button(&quit, false, &qv);
    register_target(s, T_QUIT, b);

    ChipSpec backups = {DRAW_GLYPH_X, str(STR_BTN_BACKUPS)};
    ChipView kv = {press_offset(&s->press[T_BACKUPS]) != 0, false};
    GeomRect cb = chip_box(&backups, (THEME_BOTTOM_W - chip_width(&backups)) / 2,
                           THEME_BAR_Y + (THEME_BAR_H - THEME_CHIP_H) / 2);
    register_target(s, T_BACKUPS, chip_draw(&backups, cb.x, cb.y, &kv));

    ButtonSpec open = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_OPEN)};
    ButtonView ov = {!panel_at(s->pos)->available, false, press_offset(&s->press[T_OPEN])};
    b = chrome_bar_button(&open, true, &ov);
    register_target(s, T_OPEN, b);

    quit_dialog_draw(&s->quit, s->focus.visible);
    toast_widget_draw(&s->toast);
}

static bool gallery_blocks_start(const Scene *self)
{
    return quit_dialog_active(&((const GalleryScene *)self)->quit);
}

static GalleryScene s_gallery = {.base = {"S2 Galerie", gallery_on_enter, NULL, gallery_update,
                                          gallery_draw_top, gallery_draw_bottom,
                                          gallery_blocks_start}};

Scene *scene_gallery(void)
{
    return &s_gallery.base;
}
