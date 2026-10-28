/*
 * Smoke test scenes, see smoke.h.
 *
 * Scene A ("Komponenten") pages through the theme, the type scale, buttons,
 * piece and card states, the atlas, a dialog and the coin counter with new
 * pieces. L/R or the bar buttons switch pages, X (or the chip in the title
 * bar) pushes scene B ("3D-Test"), which shows every depth layer and goes
 * back with B.
 *
 * Captions that only exist in this test are written here; everything a user
 * will see in the app comes from core/strings.c.
 */
#include "ui/smoke/smoke.h"

#include <stdio.h>

#include <3ds.h>

#include "core/coin_counter.h"
#include "core/press.h"
#include "core/strings.h"
#include "core/toast.h"
#include "core/version.h"
#include "core/widget_math.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/board.h"
#include "ui/widgets/button.h"
#include "ui/widgets/card.h"
#include "ui/widgets/chip.h"
#include "ui/widgets/chrome.h"
#include "ui/widgets/coin.h"
#include "ui/widgets/dialog.h"
#include "ui/widgets/pill.h"
#include "ui/widgets/progress.h"
#include "ui/widgets/tile.h"
#include "ui/widgets/toast_view.h"

#define OPAQUE THEME_ALPHA_OPAQUE

/* --- Shared helpers ------------------------------------------------------- */

static void caption(int cx, int y, const char *text)
{
    draw_text(cx, y, THEME_TEXT_BODY, THEME_INK_3, OPAQUE, DRAW_ALIGN_CENTER, text);
}

/* --- Scene B: depth layers ------------------------------------------------- */

typedef struct {
    Scene base;
    HitTap tap;
    HitRect back; /* where the button was drawn last frame */
    Press press;
} DepthScene;

static void depth_on_enter(Scene *self)
{
    DepthScene *d = (DepthScene *)self;
    hit_tap_reset(&d->tap);
    press_init(&d->press);
}

static void depth_update(Scene *self, const InputFrame *in)
{
    DepthScene *d = (DepthScene *)self;
    int tapped = input_tap(&d->tap, in, &d->back, 1);
    press_update(&d->press, hit_tap_pressed(&d->tap) == 0, ANIM_BUTTON_RELEASE);
    if ((in->down & KEY_B) || tapped == 0) {
        audio_play(SFX_BACK);
        scene_pop();
    }
}

typedef struct {
    StereoLayer layer;
    const char *label;
    int x;
    int w;
} LayerRow;

static const LayerRow s_layer_rows[] = {
    {STEREO_LAYER_BACKGROUND, "Himmel, Wiese  -6", 24, 150},
    {STEREO_LAYER_CLOUDS, "Wolken  -4", 24, 110},
    {STEREO_LAYER_BOARD, "Brett  0", 24, 90},
    {STEREO_LAYER_FRONT, "Pillen, Markise  +2", 24, 140},
    {STEREO_LAYER_POP, "Band, Funken  +3", 24, 130},
};

static void depth_draw_top(Scene *self, const StereoView *view)
{
    (void)self;
    int y = 50;
    for (size_t i = 0; i < sizeof(s_layer_rows) / sizeof(s_layer_rows[0]); i++) {
        const LayerRow *row = &s_layer_rows[i];
        stereo_layer_begin(view, row->layer);
        GeomRect box = {row->x, y, row->w, 26};
        draw_slice(DRAW_SLICE_PILL, box, OPAQUE);
        draw_text_vcenter(box.x + THEME_PILL_PAD, box.y, box.h, THEME_TEXT_BODY, THEME_INK, OPAQUE,
                          DRAW_ALIGN_LEFT, row->label);
        /* A marker that moves with the layer, to see the offset against the straight edge. */
        draw_rect(box.x + box.w + 8, box.y, 4, box.h, THEME_PLAZA, OPAQUE);
        stereo_layer_end();
        y += 32;
    }

    stereo_layer_begin(view, STEREO_LAYER_POP);
    draw_sprite(DRAW_SPARK, 300, 80, OPAQUE);
    draw_sprite(DRAW_SPARK, 340, 130, OPAQUE);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    char line[64];
    snprintf(line, sizeof(line), "3D-Regler %d %%   Auge: %s", (int)(view->slider * 100.0f + 0.5f),
             view->eye == STEREO_EYE_LEFT ? "links" : "rechts");
    draw_text(THEME_TOP_W / 2, 216, THEME_TEXT_BODY, THEME_INK, OPAQUE, DRAW_ALIGN_CENTER, line);
    stereo_layer_end();
}

static void depth_draw_bottom(Scene *self)
{
    DepthScene *d = (DepthScene *)self;
    chrome_header_draw("3D-Test", NULL);

    GeomRect card = {THEME_CONTENT_PAD, THEME_CONTENT_Y + 8, THEME_BOTTOM_W - 2 * THEME_CONTENT_PAD,
                     THEME_CONTENT_H - 16};
    draw_slice(DRAW_SLICE_CARD, card, OPAQUE);
    static const char *const lines[] = {
        "Oben liegen fünf Ebenen hintereinander.", "Himmel und Wolken sollen hinter dem Bildschirm",
        "liegen, Pillen und Markise davor.",       "Bei Regler 0 steht alles auf einer Ebene.",
        "Der Übergang zurück läuft von links.",
    };
    int y = card.y + 10;
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        draw_text(card.x + 10, y, THEME_TEXT_BODY, THEME_INK, OPAQUE, DRAW_ALIGN_LEFT, lines[i]);
        y += draw_text_height(THEME_TEXT_BODY) + 4;
    }

    chrome_bar_draw();
    ButtonSpec back = {BUTTON_PRIMARY, DRAW_GLYPH_B, str(STR_BTN_BACK)};
    ButtonView view = {false, false, press_offset(&d->press)};
    GeomRect box = button_draw(&back, THEME_BAR_MARGIN, chrome_bar_button_y(), &view);
    d->back = (HitRect){box.x, box.y, box.w, box.h};
}

static DepthScene s_depth = {
    .base = {"3D-Test", depth_on_enter, NULL, depth_update, depth_draw_top, depth_draw_bottom}};

/* --- Scene A: components --------------------------------------------------- */

typedef enum {
    PAGE_COLORS,
    PAGE_TYPE,
    PAGE_BUTTONS,
    PAGE_STATES,
    PAGE_GRID,
    PAGE_ATLAS,
    PAGE_DIALOG,
    PAGE_COINS,
    PAGE_COUNT,
} Page;

static const char *const s_page_titles[PAGE_COUNT] = {
    "Farben", "Schrift", "Knöpfe", "Zustände", "Teile-Raster", "Grafiken", "Dialog", "Münzen",
};

typedef enum {
    T_PREV,
    T_NEXT,
    T_DEPTH,
    T_ERRORS,
    T_BTN_HINT,
    T_DLG_OPEN,
    T_DLG_CANCEL,
    T_DLG_BUY,
    T_COINS_PLUS,
    T_COINS_MINUS,
    T_FRESH,
    T_TILE0,
    T_COUNT = T_TILE0 + 40,
} Target;

#define BOARD_PIECES 40
#define OWNED_AT(i)  (((i) * 7) % 10 < 7)
#define START_COINS  120
#define FRESH_PIECES 3

typedef struct {
    Scene base;
    Page page;
    FocusState focus;
    HitTap tap;
    Press press[T_COUNT];
    HitRect rects[T_COUNT];
    int ids[T_COUNT];
    int rect_count;
    Toast toast;
    Dialog dialog;
    CoinCounter coins;
    unsigned coin_target;
    uint64_t owned, rare, selected, fresh;
    unsigned bump_start[BOARD_PIECES];
    bool bumping[BOARD_PIECES];
    unsigned fresh_frame;
    bool fresh_running;
    int sfx_last;
    AudioResult audio;
} CompScene;

static CompScene s_comp;

static void register_target(CompScene *s, Target id, GeomRect box)
{
    if (s->rect_count >= T_COUNT)
        return;
    s->rects[s->rect_count] = (HitRect){box.x, box.y, box.w, box.h};
    s->ids[s->rect_count] = id;
    s->rect_count++;
}

static bool target_pressed(const CompScene *s, Target id)
{
    int p = hit_tap_pressed(&s->tap);
    return p >= 0 && p < s->rect_count && s->ids[p] == (int)id;
}

static int count_bits(uint64_t mask)
{
    int n = 0;
    for (int i = 0; i < BOARD_PIECES; i++)
        n += widget_mask_has(mask, i);
    return n;
}

static void comp_on_enter(Scene *self)
{
    CompScene *s = (CompScene *)self;
    hit_tap_reset(&s->tap);
}

static void toggle_tile(CompScene *s, int i)
{
    if (widget_mask_has(s->owned, i))
        return;
    uint64_t bit = 1ull << i;
    bool selecting = !(s->selected & bit);
    s->selected ^= bit;
    s->bumping[i] = true;
    s->bump_start[i] = anim_frame();
    audio_play(selecting ? SFX_SELECT : SFX_DESELECT);
}

static void change_page(CompScene *s, int delta)
{
    int p = (int)s->page + delta;
    if (p < 0 || p >= PAGE_COUNT) {
        audio_play(SFX_ERROR);
        return;
    }
    s->page = (Page)p;
    audio_play(SFX_PAGE);
}

static void activate(CompScene *s, Target id)
{
    if (id >= T_TILE0) {
        s->focus.index = id - T_TILE0;
        toggle_tile(s, id - T_TILE0);
        return;
    }
    switch (id) {
    case T_PREV:
        change_page(s, -1);
        break;
    case T_NEXT:
        change_page(s, 1);
        break;
    case T_DEPTH:
        audio_play(SFX_CONFIRM);
        scene_push(&s_depth.base);
        break;
    case T_ERRORS: /* all error screens */
        audio_play(SFX_CONFIRM);
        scene_push(scene_error_preview());
        break;
    case T_BTN_HINT:
        audio_play(SFX_ERROR);
        toast_show(&s->toast, str(STR_NOT_ENOUGH_COINS));
        break;
    case T_DLG_OPEN:
        audio_play(SFX_CONFIRM);
        dialog_open(&s->dialog);
        break;
    case T_DLG_CANCEL:
        audio_play(SFX_BACK);
        dialog_close(&s->dialog);
        break;
    case T_DLG_BUY:
        audio_play(SFX_BUY);
        dialog_close(&s->dialog);
        break;
    case T_COINS_PLUS:
        s->coin_target = s->coin_target + 10 > 300 ? 300 : s->coin_target + 10;
        coin_counter_set(&s->coins, s->coin_target);
        audio_play(SFX_CONFIRM);
        break;
    case T_COINS_MINUS:
        s->coin_target = s->coin_target < 10 ? 0 : s->coin_target - 10;
        coin_counter_set(&s->coins, s->coin_target);
        audio_play(SFX_CONFIRM);
        break;
    case T_FRESH:
        s->fresh = 0;
        for (int i = 0, n = 0; i < BOARD_PIECES && n < FRESH_PIECES; i++) {
            if (!widget_mask_has(s->owned, i) && !widget_mask_has(s->selected, i)) {
                s->fresh |= 1ull << i;
                n++;
            }
        }
        s->fresh_frame = 0;
        s->fresh_running = true;
        audio_play(SFX_BUY);
        break;
    default:
        break;
    }
}

static void comp_update(Scene *self, const InputFrame *in)
{
    CompScene *s = (CompScene *)self;

    if ((in->down & KEY_Y) && s->page != PAGE_GRID) { /* sound test; Y selects all on the grid */
        s->sfx_last = (s->sfx_last + 1) % SFX_COUNT;
        audio_play((Sfx)s->sfx_last);
    }
    if (in->down & KEY_SELECT)
        audio_set_enabled(!audio_is_enabled());

    u32 keys = input_focus(&s->focus, in);
    int hit = input_tap(&s->tap, in, s->rects, s->rect_count);
    int tapped = hit >= 0 ? s->ids[hit] : -1;

    if (s->dialog.open) {
        /* Modal: only the dialog's own buttons and B/A react, once it has finished opening. */
        if (dialog_ready(&s->dialog)) {
            if (keys & KEY_B)
                activate(s, T_DLG_CANCEL);
            else if ((keys & KEY_A) || tapped == T_DLG_BUY)
                activate(s, T_DLG_BUY);
            else if (tapped == T_DLG_CANCEL)
                activate(s, T_DLG_CANCEL);
        }
    } else {
        if (keys & KEY_L)
            change_page(s, -1);
        if (keys & KEY_R)
            change_page(s, 1);
        if (keys & KEY_X)
            activate(s, T_DEPTH);
        if (tapped >= 0)
            activate(s, (Target)tapped);

        if (s->page == PAGE_GRID) {
            int dir = input_direction(keys);
            if (dir >= 0) {
                if (focus_grid_move(&s->focus.index, 8, BOARD_PIECES, (FocusDir)dir, 0) ==
                    FOCUS_MOVED)
                    audio_play(SFX_MOVE);
            }
            if (keys & KEY_A)
                toggle_tile(s, s->focus.index);
            if (keys & KEY_Y) /* all missing pieces, like the real picker */
                s->selected = (~s->owned) & ((1ull << BOARD_PIECES) - 1);
        } else if (keys & KEY_A) {
            if (s->page == PAGE_BUTTONS)
                activate(s, T_BTN_HINT);
            else if (s->page == PAGE_DIALOG)
                activate(s, T_DLG_OPEN);
            else if (s->page == PAGE_COINS)
                activate(s, T_FRESH);
        }
    }

    for (int id = 0; id < T_COUNT; id++)
        press_update(&s->press[id], target_pressed(s, (Target)id), ANIM_BUTTON_RELEASE);

    toast_step(&s->toast);
    dialog_step(&s->dialog);
    if (coin_counter_step(&s->coins))
        audio_play(SFX_COIN);
    if (s->fresh_running) {
        s->fresh_frame++;
        if (s->fresh_frame > ANIM_NEW_PIECE + ANIM_NEW_PIECE_OFFSET * FRESH_PIECES) {
            s->fresh_running = false;
            s->owned |= s->fresh;
            s->fresh = 0;
        }
    }
}

/* --- Scene A: top screen ---------------------------------------------------- */

static void comp_draw_top(Scene *self, const StereoView *view)
{
    CompScene *s = (CompScene *)self;

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    char count[16];
    unsigned owned_now = (unsigned)count_bits(s->owned);
    snprintf(count, sizeof(count), "%u/%d", owned_now, BOARD_PIECES);
    pill_name_draw(THEME_PILL_MARGIN, THEME_PILL_Y, "Sommerwiese", count);
    pill_coin_draw(THEME_TOP_W - THEME_PILL_MARGIN, THEME_PILL_Y, true,
                   coin_counter_value(&s->coins),
                   coin_counter_shining(&s->coins) ? (u8)anim_focus_alpha() : 0);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    /* Selected pieces are not owned yet; fresh ones are owned and pop in. */
    BoardSpec board = {BOARD_PIECES, s->owned | s->fresh, s->selected, s->rare,
                       s->fresh,     s->fresh_frame};
    GeomRect art = board_draw(&board);

    unsigned after = owned_now + (unsigned)count_bits(s->selected);
    GeomRect track = {(THEME_TOP_W - THEME_PROGRESS_W) / 2, THEME_PROGRESS_Y, THEME_PROGRESS_W,
                      THEME_PROGRESS_H};
    progress_draw(track, after, BOARD_PIECES, PROGRESS_ON_SKY, OPAQUE);
    char left[24], right[24];
    snprintf(left, sizeof(left), str(STR_PROGRESS_OF), owned_now, (unsigned)BOARD_PIECES);
    snprintf(right, sizeof(right), str(STR_AFTER_PURCHASE), after, (unsigned)BOARD_PIECES);
    int ty = track.y + (track.h - draw_text_height(THEME_TEXT_BODY)) / 2;
    draw_text(track.x - THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2, OPAQUE,
              DRAW_ALIGN_RIGHT, left);
    draw_text(track.x + track.w + THEME_PROGRESS_TEXT_GAP, ty, THEME_TEXT_BODY, THEME_INK_2, OPAQUE,
              DRAW_ALIGN_LEFT, right);

    char snd[80];
    snprintf(snd, sizeof(snd), " Klang: %s   SELECT: %s%s",
             s->sfx_last >= 0 ? sfx_file_name((Sfx)s->sfx_last) : "-",
             audio_is_enabled() ? "an" : "aus",
             s->audio == AUDIO_ERR_DSP ? "  (kein dspfirm.cdc)"
             : s->audio != AUDIO_OK    ? "  (Datei fehlt)"
                                       : "");
    draw_text(THEME_TOP_W / 2, THEME_PROGRESS_Y + 14, THEME_TEXT_BODY, THEME_INK, OPAQUE,
              DRAW_ALIGN_CENTER, snd);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_POP);
    draw_sprite(DRAW_SPARK, art.x + art.w - THEME_BOARD_SPARK_X, art.y - THEME_BOARD_SPARK_Y,
                OPAQUE);
    stereo_layer_end();
}

/* --- Scene A: bottom screen pages -------------------------------------------- */

static void page_colors(CompScene *s)
{
    (void)s;
    const int cols = 4, cell_w = 73, gap = 4, swatch_h = 16, row_h = 33;
    for (int i = 0; i < theme_swatch_count; i++) {
        int x = THEME_CONTENT_PAD + (i % cols) * (cell_w + gap);
        int y = THEME_CONTENT_Y + 6 + (i / cols) * row_h;
        GeomRect sw = {x, y, cell_w, swatch_h};
        draw_slice_tinted(DRAW_SLICE_ROUND_5, sw, THEME_LINE, THEME_LINE, OPAQUE);
        u8 alpha = theme_swatches[i].color == THEME_SCRIM ? THEME_ALPHA_SCRIM : OPAQUE;
        draw_slice_tinted(DRAW_SLICE_ROUND_5, geom_rect_inset(sw, -1), theme_swatches[i].color,
                          theme_swatches[i].color, alpha);
        draw_text_fit(x, y + swatch_h + 1, cell_w, THEME_TEXT_BODY, THEME_INK_2, OPAQUE,
                      DRAW_ALIGN_LEFT, theme_swatches[i].name);
    }
}

static void page_type(CompScene *s)
{
    (void)s;
    char headline[48], pieces[24], line[64];
    strings_new_pieces(pieces, sizeof(pieces), 3);
    snprintf(headline, sizeof(headline), str(STR_BOUGHT_HEADLINE), pieces);
    snprintf(line, sizeof(line), str(STR_BOUGHT_PROGRESS), "Sommerwiese", 31u, 40u);

    static const struct {
        const char *label;
        int px;
        ThemeColor color;
    } rows[] = {
        {"Titel 15", THEME_TEXT_TITLE, THEME_INK},  {"Kopf 13", THEME_TEXT_HEADER, THEME_INK},
        {"Knopf 12", THEME_TEXT_BUTTON, THEME_INK}, {"Text 11", THEME_TEXT_BODY, THEME_INK},
        {"Neben 11", THEME_TEXT_BODY, THEME_INK_2},
    };
    const char *samples[] = {headline, str(STR_TITLE_GALLERY), str(STR_BTN_BUY), line,
                             "28/40 · kleinste Größe, nichts darunter"};

    int y = THEME_CONTENT_Y + 6;
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        int h = draw_text_height(rows[i].px) + 2;
        draw_text_vcenter(THEME_CONTENT_PAD, y, h, THEME_TEXT_BODY, THEME_INK_3, OPAQUE,
                          DRAW_ALIGN_LEFT, rows[i].label);
        draw_text_vcenter(64, y, h, rows[i].px, rows[i].color, OPAQUE, DRAW_ALIGN_LEFT, samples[i]);
        y += h;
    }

    y += 4;
    draw_text(THEME_CONTENT_PAD, y + 2, THEME_TEXT_BODY, THEME_INK_3, OPAQUE, DRAW_ALIGN_LEFT,
              "Glyphen");
    draw_text(64, y, THEME_GLYPH_PX, THEME_INK, OPAQUE, DRAW_ALIGN_LEFT,
              DRAW_GLYPH_A " " DRAW_GLYPH_B " " DRAW_GLYPH_X " " DRAW_GLYPH_Y " " DRAW_GLYPH_L
                           " " DRAW_GLYPH_R " " DRAW_GLYPH_DPAD);
    y += 24;

    GeomRect card = {THEME_CONTENT_PAD, y, 148, 40};
    draw_slice(DRAW_SLICE_CARD, card, OPAQUE);
    caption(card.x + card.w / 2, card.y + 4, "Ellipse bei 128 px");
    draw_text_fit(card.x + 10, card.y + 20, 128, THEME_TEXT_BODY, THEME_INK, OPAQUE,
                  DRAW_ALIGN_LEFT, "Sommerwiese am Leuchtturm von Mii-Hafen");

    GeomRect clip_card = {THEME_BOTTOM_W - THEME_CONTENT_PAD - 148, y, 148, 40};
    draw_slice(DRAW_SLICE_CARD, clip_card, OPAQUE);
    caption(clip_card.x + clip_card.w / 2, clip_card.y + 4, "Clipping");
    GeomRect window = {clip_card.x + 10, clip_card.y + 20, 128, 14};
    draw_rect(window.x, window.y, window.w, window.h, THEME_PLAZA_MIST, OPAQUE);
    draw_clip_push(window);
    int run = (int)((anim_frame() / 2) % (unsigned)(window.w + 200));
    draw_text(window.x + window.w - run, window.y + 1, THEME_TEXT_BODY, THEME_PLAZA_DARK, OPAQUE,
              DRAW_ALIGN_LEFT, str(STR_TITLE_PICKER));
    draw_clip_pop();
}

static void page_buttons(CompScene *s)
{
    int y = THEME_CONTENT_Y + 8;
    GeomRect b;
    ButtonSpec spec;
    ButtonView view;

    spec = (ButtonSpec){BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_BACK)};
    view = (ButtonView){false, false, 0};
    b = button_draw(&spec, THEME_CONTENT_PAD, y, &view);
    caption(b.x + b.w / 2, y + 36, "Sekundär");
    spec = (ButtonSpec){BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_BUY)};
    b = button_draw(&spec, b.x + b.w + 14, y, &view);
    caption(b.x + b.w / 2, y + 36, "Primär");
    view = (ButtonView){false, false, 1};
    b = button_draw(&spec, b.x + b.w + 14, y, &view);
    caption(b.x + b.w / 2, y + 36, "Gedrückt");

    y += 56;
    spec = (ButtonSpec){BUTTON_SECONDARY, DRAW_GLYPH_A, str(STR_BTN_BUY)};
    view = (ButtonView){true, false, 0};
    b = button_draw(&spec, THEME_CONTENT_PAD, y, &view);
    caption(b.x + b.w / 2, y + 36, "Gesperrt");
    spec = (ButtonSpec){BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_BUY)};
    view = (ButtonView){false, true, 0};
    b = button_draw(&spec, b.x + b.w + 18, y, &view);
    caption(b.x + b.w / 2, y + 36, "Fokus");

    /* A live button: touch it to see it pressed and the hint appear; A does the same. */
    spec = (ButtonSpec){BUTTON_SECONDARY, DRAW_GLYPH_A, "Hinweis"};
    view = (ButtonView){false, false, press_offset(&s->press[T_BTN_HINT])};
    b = button_draw(&spec, b.x + b.w + 18, y, &view);
    register_target(s, T_BTN_HINT, b);
    caption(b.x + b.w / 2, y + 36, "Antippen");

    y += 56;
    ChipSpec chip = {DRAW_GLYPH_Y, str(STR_BTN_SELECT_ALL)};
    ChipView cv = {false, false};
    GeomRect c = chip_draw(&chip, THEME_CONTENT_PAD, y + 5, &cv);
    caption(c.x + c.w / 2, y + 30, "Chip");
    GeomRect pill = pill_coin_draw(c.x + c.w + 20, y + 4, false, 120, 0);
    caption(pill.x + pill.w / 2, y + 30, "Münzanzeige");
    coin_price_draw(pill.x + pill.w + 20, y + 2, 24, 6, THEME_INK);
    caption(pill.x + pill.w + 20 + 18, y + 30, "Preis");
}

static void page_states(CompScene *s)
{
    (void)s;
    const ThemeBoard *b = theme_board(40);
    int y = THEME_CONTENT_Y + 12;
    static const struct {
        TileState state;
        bool rare, locked, focus;
        const char *label;
    } tiles[] = {
        {TILE_MISSING, false, false, false, "Fehlt"},
        {TILE_SELECTED, false, false, false, "Gewählt"},
        {TILE_OWNED, false, false, false, "Da"},
        {TILE_MISSING, true, false, false, "Selten"},
        {TILE_MISSING, false, true, false, "Gesperrt"},
        {TILE_MISSING, false, false, true, "Fokus"},
    };
    int step = (THEME_BOTTOM_W - 2 * THEME_CONTENT_PAD) / 6;
    for (int i = 0; i < 6; i++) {
        int cx = THEME_CONTENT_PAD + step * i + step / 2;
        GeomRect box = {cx - b->tile_w / 2, y, b->tile_w, b->tile_h};
        /* The owned tile shows its part of a whole 8 x 5 picture. */
        GeomRect art = {box.x - 3 * b->tile_w, box.y - 2 * b->tile_h, 8 * b->tile_w, 5 * b->tile_h};
        TileSpec spec = {tiles[i].state, 23, tiles[i].rare, tiles[i].locked, art};
        TileView view = {tiles[i].focus, 1.0f};
        tile_draw(box, &spec, &view);
        caption(cx, y + b->tile_h + 6, tiles[i].label);
    }

    y += 56;
    static const PuzzleCardSpec cards[] = {
        {"Sommerwiese am Leuchtturm", 28, 40},
        {"Herbstwald", 24, 24},
        {"Polarlicht", 0, 15},
    };
    for (int i = 0; i < 3; i++) {
        GeomRect card = {THEME_CONTENT_PAD + 4 + i * (THEME_PUZZLE_CARD_W + THEME_CARD_GAP), y,
                         THEME_PUZZLE_CARD_W, THEME_PUZZLE_CARD_H};
        puzzle_card_draw(card, &cards[i], i == 0, i != 2);
    }
}

static void page_grid(CompScene *s)
{
    const ThemeBoard *b = theme_board(BOARD_PIECES);
    int gx = (THEME_BOTTOM_W - (b->cols * b->tile_w + (b->cols - 1) * THEME_TILE_GAP)) / 2;
    int gy = THEME_CONTENT_Y + THEME_CONTENT_PAD;
    GeomRect art = {gx, gy, b->cols * b->tile_w + (b->cols - 1) * THEME_TILE_GAP,
                    b->rows * b->tile_h + (b->rows - 1) * THEME_TILE_GAP};

    for (int i = 0; i < BOARD_PIECES; i++) {
        GeomRect box = {gx + (i % b->cols) * (b->tile_w + THEME_TILE_GAP),
                        gy + (i / b->cols) * (b->tile_h + THEME_TILE_GAP), b->tile_w, b->tile_h};
        TileState state = widget_mask_has(s->owned, i)      ? TILE_OWNED
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
        TileSpec spec = {state, i + 1, widget_mask_has(s->rare, i), false, art};
        TileView view = {s->focus.visible && s->focus.index == i, bump};
        tile_draw(box, &spec, &view);
        if (state != TILE_OWNED)
            register_target(s, (Target)(T_TILE0 + i), box);
    }
}

static void page_atlas(CompScene *s)
{
    (void)s;
    static const struct {
        DrawSprite sprite;
        const char *label;
    } items[] = {
        {DRAW_COIN_12, "Münze 12"}, {DRAW_COIN_16, "16"},        {DRAW_COIN_20, "20"},
        {DRAW_STAR, "Stern"},       {DRAW_CHECK_BADGE, "Haken"}, {DRAW_SPARK, "Funke"},
    };
    int y = THEME_CONTENT_Y + 8;
    int step = (THEME_BOTTOM_W - 2 * THEME_CONTENT_PAD) / 6;
    for (int i = 0; i < 6; i++) {
        int cx = THEME_CONTENT_PAD + step * i + step / 2;
        int w = draw_sprite_width(items[i].sprite), h = draw_sprite_height(items[i].sprite);
        draw_sprite(items[i].sprite, cx - w / 2, y + (22 - h) / 2, OPAQUE);
        caption(cx, y + 24, items[i].label);
    }
    y += 44;
    draw_slice_tinted(DRAW_SLICE_ROUND_3, (GeomRect){THEME_CONTENT_PAD + 6, y, 16, 16},
                      THEME_PLAZA_HI, THEME_PLAZA, OPAQUE);
    draw_sprite(DRAW_CHECK, THEME_CONTENT_PAD + 10, y + 4, OPAQUE);

    draw_sprite(DRAW_LOGO, 40, y - 2, OPAQUE);
    draw_text(100, y + 8, THEME_TEXT_TITLE, THEME_INK, OPAQUE, DRAW_ALIGN_LEFT, str(STR_APP_NAME));
    draw_text(100, y + 28, THEME_TEXT_BODY, THEME_INK_2, OPAQUE, DRAW_ALIGN_LEFT,
              str(STR_APP_TAGLINE));

    y += 60;
    GeomRect sky = {THEME_CONTENT_PAD, y, 150, 58};
    draw_slice(DRAW_SLICE_CARD, sky, OPAQUE);
    draw_clip_push(geom_rect_inset(sky, -1));
    draw_gradient_v(sky.x, sky.y, sky.w, sky.h, THEME_FOCUS, THEME_SKY_LIGHT, OPAQUE);
    draw_sprite(DRAW_CLOUD, sky.x + 6, sky.y + 6, THEME_ALPHA_CLOUD);
    draw_sprite(DRAW_CLOUD_SMALL, sky.x + 90, sky.y + 30, THEME_ALPHA_CLOUD);
    draw_clip_pop();
    caption(sky.x + sky.w / 2, sky.y + sky.h + 4, "Wolken");

    GeomRect aw = {THEME_BOTTOM_W - THEME_CONTENT_PAD - 150, y, 150, 58};
    draw_slice(DRAW_SLICE_CARD, aw, OPAQUE);
    draw_clip_push(geom_rect_inset(aw, -1));
    draw_sprite_tiled(DRAW_STRIPES, aw, (int)(anim_frame() / 4), (int)(anim_frame() / 4));
    draw_sprite_tiled(DRAW_AWNING, (GeomRect){aw.x, aw.y, aw.w, 32}, 0, 0);
    draw_clip_pop();
    caption(aw.x + aw.w / 2, aw.y + aw.h + 4, "Markise, Streifen");
}

static void page_dialog_background(CompScene *s)
{
    const ThemeBoard *b = theme_board(40);
    for (int i = 0; i < 8; i++) {
        GeomRect box = {16 + i * (b->tile_w + THEME_TILE_GAP) + 10, THEME_CONTENT_Y + 10, b->tile_w,
                        b->tile_h};
        TileSpec spec = {i == 2 || i == 5 ? TILE_SELECTED : TILE_MISSING, 21 + i, false, false,
                         box};
        TileView view = {false, 1.0f};
        tile_draw(box, &spec, &view);
    }
    ButtonSpec open = {BUTTON_PRIMARY, DRAW_GLYPH_A, "Dialog öffnen"};
    ButtonView view = {false, false, press_offset(&s->press[T_DLG_OPEN])};
    int x = (THEME_BOTTOM_W - button_width(&open)) / 2;
    GeomRect bx = button_draw(&open, x, THEME_CONTENT_Y + 80, &view);
    if (!s->dialog.open)
        register_target(s, T_DLG_OPEN, bx);
}

static void dialog_draw(CompScene *s)
{
    if (!s->dialog.open)
        return;
    dialog_draw_scrim(&s->dialog);

    const int height = 188;
    GeomRect inner = dialog_draw_frame(&s->dialog, str(STR_TITLE_CONFIRM), height);
    GeomRect dlg = dialog_box(height);

    draw_opacity_push(dialog_content_alpha(&s->dialog));
    char pieces[24], coins[32], question[96];
    strings_pieces(pieces, sizeof(pieces), 3);
    strings_coins(coins, sizeof(coins), 6);
    snprintf(question, sizeof(question), str(STR_CONFIRM_QUESTION), pieces, coins);
    int qh = draw_text_height(THEME_TEXT_HEADER) + 4;
    draw_text_wrapped(inner.x + inner.w / 2, inner.y + 10, inner.w - 28, qh, 2, THEME_TEXT_HEADER,
                      THEME_INK, OPAQUE, DRAW_ALIGN_CENTER, question);

    static const int nums[] = {23, 26, 34};
    for (int i = 0; i < 3; i++) {
        GeomRect m = {dlg.x + dlg.w / 2 - 44 + i * 31, dlg.y + 76, 26, 22};
        draw_slice_tinted(DRAW_SLICE_ROUND_5, m, THEME_PLAZA_DARK, THEME_PLAZA_DARK, OPAQUE);
        draw_slice_tinted(DRAW_SLICE_ROUND_5, geom_rect_inset(m, -1), THEME_PLAZA_HI, THEME_PLAZA,
                          OPAQUE);
        char n[4];
        snprintf(n, sizeof(n), "%d", nums[i]);
        draw_text_vcenter(m.x + m.w / 2, m.y, m.h, THEME_TEXT_BODY, THEME_WHITE, OPAQUE,
                          DRAW_ALIGN_CENTER, n);
    }

    GeomRect math = {dlg.x + 14, dlg.y + 108, dlg.w - 28, 30};
    draw_slice_tinted(DRAW_SLICE_ROUND_8, math, THEME_LINE, THEME_LINE, OPAQUE);
    draw_slice_tinted(DRAW_SLICE_ROUND_8, geom_rect_inset(math, -1), THEME_PLAZA_MIST,
                      THEME_PLAZA_MIST, OPAQUE);
    int mx = math.x + 30;
    mx += draw_text_vcenter(mx, math.y, math.h, THEME_TEXT_BODY, THEME_INK_2, OPAQUE,
                            DRAW_ALIGN_LEFT, str(STR_BALANCE)) +
          8;
    coin_draw(COIN_MEDIUM, mx, math.y + 7, 0);
    mx += 20;
    mx += draw_text_vcenter(mx, math.y, math.h, THEME_TEXT_HEADER, THEME_INK, OPAQUE,
                            DRAW_ALIGN_LEFT, "120") +
          10;
    mx += draw_text_vcenter(mx, math.y, math.h, THEME_TEXT_HEADER, THEME_INK_3, OPAQUE,
                            DRAW_ALIGN_LEFT, "→") +
          10;
    draw_text_vcenter(mx, math.y, math.h, THEME_TEXT_HEADER, THEME_PLAZA_DARK, OPAQUE,
                      DRAW_ALIGN_LEFT, "114");

    int by = dlg.y + dlg.h - THEME_BTN_H - THEME_DIALOG_PAD;
    ButtonSpec cancel = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_CANCEL)};
    ButtonSpec buy = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_BUY)};
    ButtonView cv = {false, false, press_offset(&s->press[T_DLG_CANCEL])};
    ButtonView bv = {false, s->focus.visible, press_offset(&s->press[T_DLG_BUY])};
    GeomRect cb = button_draw(&cancel, dlg.x + THEME_DIALOG_PAD, by, &cv);
    GeomRect bb = button_draw(&buy, dlg.x + dlg.w - THEME_DIALOG_PAD - button_width(&buy), by, &bv);
    draw_opacity_pop();
    if (dialog_ready(&s->dialog)) {
        register_target(s, T_DLG_CANCEL, cb);
        register_target(s, T_DLG_BUY, bb);
    }
}

static void page_coins(CompScene *s)
{
    int y = THEME_CONTENT_Y + 10;
    char line[48];
    snprintf(line, sizeof(line), "Anzeige oben: %u", coin_counter_value(&s->coins));
    draw_text(THEME_CONTENT_PAD + 4, y, THEME_TEXT_BODY, THEME_INK, OPAQUE, DRAW_ALIGN_LEFT, line);
    draw_text(THEME_CONTENT_PAD + 4, y + 16, THEME_TEXT_BODY, THEME_INK_2, OPAQUE, DRAW_ALIGN_LEFT,
              "Eine Münze alle 2 Frames, höchstens 45 Frames.");

    ButtonSpec minus = {BUTTON_SECONDARY, NULL, "-10"};
    ButtonSpec plus = {BUTTON_SECONDARY, NULL, "+10"};
    ButtonView mv = {false, false, press_offset(&s->press[T_COINS_MINUS])};
    ButtonView pv = {false, false, press_offset(&s->press[T_COINS_PLUS])};
    GeomRect mb = button_draw(&minus, THEME_CONTENT_PAD + 4, y + 44, &mv);
    GeomRect pb = button_draw(&plus, mb.x + mb.w + 10, y + 44, &pv);
    register_target(s, T_COINS_MINUS, mb);
    register_target(s, T_COINS_PLUS, pb);

    ButtonSpec fresh = {BUTTON_PRIMARY, DRAW_GLYPH_A, "Neue Teile"};
    ButtonView fv = {false, false, press_offset(&s->press[T_FRESH])};
    GeomRect fb = button_draw(&fresh, THEME_CONTENT_PAD + 4, y + 90, &fv);
    register_target(s, T_FRESH, fb);
    draw_text(fb.x + fb.w + 10, y + 90 + 8, THEME_TEXT_BODY, THEME_INK_2, OPAQUE, DRAW_ALIGN_LEFT,
              "springen oben ins Brett");

    coin_price_draw(THEME_CONTENT_PAD + 4, y + 130, 24, 2, THEME_INK);
    coin_price_draw(THEME_CONTENT_PAD + 64, y + 130, 24, 10, THEME_INK);
    ChipSpec chip = {DRAW_GLYPH_Y, str(STR_BTN_SELECT_ALL)};
    ChipView cv = {true, false};
    chip_draw(&chip, THEME_CONTENT_PAD + 130, y + 132, &cv);
}

static void comp_draw_bottom(Scene *self)
{
    CompScene *s = (CompScene *)self;
    s->rect_count = 0;

    switch (s->page) {
    case PAGE_COLORS:
        page_colors(s);
        break;
    case PAGE_TYPE:
        page_type(s);
        break;
    case PAGE_BUTTONS:
        page_buttons(s);
        break;
    case PAGE_STATES:
        page_states(s);
        break;
    case PAGE_GRID:
        page_grid(s);
        break;
    case PAGE_ATLAS:
        page_atlas(s);
        break;
    case PAGE_DIALOG:
        page_dialog_background(s);
        break;
    case PAGE_COINS:
        page_coins(s);
        break;
    case PAGE_COUNT:
        break;
    }

    char ver[24];
    snprintf(ver, sizeof(ver), "v%s", version_string());
    chrome_header_draw(s_page_titles[s->page], NULL);
    ChipSpec depth = {DRAW_GLYPH_X, "3D-Test"};
    ChipView dv = {false, false};
    dv.pressed = press_offset(&s->press[T_DEPTH]) > 0;
    int cx = THEME_BOTTOM_W - THEME_CONTENT_PAD - chip_width(&depth);
    GeomRect depth_box = chip_draw(&depth, cx, (THEME_HEADER_H - THEME_CHIP_H) / 2, &dv);
    register_target(s, T_DEPTH, depth_box);
    ChipSpec errors = {NULL, "Fehler"};
    ChipView ev = {false, false};
    ev.pressed = press_offset(&s->press[T_ERRORS]) > 0;
    GeomRect errors_box = chip_draw(&errors, depth_box.x - 6 - chip_width(&errors),
                                    (THEME_HEADER_H - THEME_CHIP_H) / 2, &ev);
    register_target(s, T_ERRORS, errors_box);
    draw_text_vcenter(errors_box.x - 6, 0, THEME_HEADER_H, THEME_TEXT_BODY, THEME_INK_3, OPAQUE,
                      DRAW_ALIGN_RIGHT, ver);

    chrome_bar_draw();
    int by = chrome_bar_button_y();
    ButtonSpec prev = {BUTTON_SECONDARY, DRAW_GLYPH_L, str(STR_BTN_BACK)};
    ButtonView pv = {s->page == 0, false, press_offset(&s->press[T_PREV])};
    GeomRect pb = button_draw(&prev, THEME_BAR_MARGIN, by, &pv);
    ButtonSpec next = {BUTTON_PRIMARY, DRAW_GLYPH_R, str(STR_BTN_CONTINUE)};
    ButtonView nv = {s->page == PAGE_COUNT - 1, false, press_offset(&s->press[T_NEXT])};
    GeomRect nb =
        button_draw(&next, THEME_BOTTOM_W - THEME_BAR_MARGIN - button_width(&next), by, &nv);
    if (s->page > 0)
        register_target(s, T_PREV, pb);
    if (s->page < PAGE_COUNT - 1)
        register_target(s, T_NEXT, nb);
    char pos[8];
    snprintf(pos, sizeof(pos), "%d/%d", (int)s->page + 1, PAGE_COUNT);
    draw_text_vcenter(THEME_BOTTOM_W / 2, THEME_BAR_Y, THEME_BAR_H, THEME_TEXT_BUTTON, THEME_INK_2,
                      OPAQUE, DRAW_ALIGN_CENTER, pos);

    dialog_draw(s);
    toast_widget_draw(&s->toast);
}

Scene *smoke_start(AudioResult audio)
{
    CompScene *s = &s_comp;
    s->base = (Scene){"Komponenten", comp_on_enter,    NULL, comp_update,
                      comp_draw_top, comp_draw_bottom, NULL};
    s->page = PAGE_COLORS;
    focus_init(&s->focus, 0, false);
    hit_tap_reset(&s->tap);
    for (int i = 0; i < T_COUNT; i++)
        press_init(&s->press[i]);
    toast_widget_init(&s->toast);
    dialog_init(&s->dialog);
    s->coin_target = START_COINS;
    coin_counter_init(&s->coins, START_COINS, ANIM_COIN_STEP, ANIM_COIN_MAX, ANIM_COIN_SOUND_EVERY);
    for (int i = 0; i < BOARD_PIECES; i++) {
        if (OWNED_AT(i))
            s->owned |= 1ull << i;
    }
    s->rare = (1ull << 4) | (1ull << 19) | (1ull << 33);
    s->owned &= ~s->rare;
    s->selected = (1ull << 24) | (1ull << 31) | (1ull << 34);
    s->selected &= ~s->owned;
    s->sfx_last = -1;
    s->audio = audio;
    return &s->base;
}
