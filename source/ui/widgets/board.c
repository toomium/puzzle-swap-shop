#include "ui/widgets/board.h"

#include <stdio.h>

#include "core/widget_math.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"
#include "ui/widgets/art.h"

GeomRect board_art_rect(int pieces)
{
    const ThemeBoard *b = theme_board(pieces);
    if (!b) {
        GeomRect none = {0, 0, 0, 0};
        return none;
    }
    int w = b->cols * b->cell, h = b->rows * b->cell;
    GeomRect art = {(THEME_TOP_W - w) / 2, THEME_BOARD_Y + THEME_BOARD_FRAME, w, h};
    return art;
}

static void draw_cell(GeomRect c, int number, bool selected, bool rare)
{
    char num[8];
    snprintf(num, sizeof(num), "%d", number);
    draw_rect(c.x, c.y, c.w, c.h, selected ? THEME_SELECTED : THEME_PAPER, THEME_ALPHA_OPAQUE);
    if (selected) {
        int e = THEME_SELECT_EDGE;
        draw_rect(c.x, c.y, c.w, e, THEME_PLAZA, THEME_ALPHA_OPAQUE);
        draw_rect(c.x, c.y + c.h - e, c.w, e, THEME_PLAZA, THEME_ALPHA_OPAQUE);
        draw_rect(c.x, c.y + e, e, c.h - 2 * e, THEME_PLAZA, THEME_ALPHA_OPAQUE);
        draw_rect(c.x + c.w - e, c.y + e, e, c.h - 2 * e, THEME_PLAZA, THEME_ALPHA_OPAQUE);
    }
    draw_text_vcenter(c.x + c.w / 2, c.y, c.h, THEME_TEXT_BODY,
                      selected ? THEME_PLAZA_DARK : THEME_INK_3, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_CENTER, num);
    if (rare)
        draw_ellipse(c.x + c.w - THEME_RARE_DOT_INSET + 2, c.y + 2, THEME_RARE_DOT / 2 + 1,
                     THEME_RARE_DOT / 2 + 1, THEME_RARE, THEME_ALPHA_OPAQUE);
}

/* Rank of piece i among the new pieces, so they pop in one after another. */
static unsigned fresh_rank(uint64_t fresh, int i)
{
    unsigned rank = 0;
    for (int k = 0; k < i; k++)
        rank += (unsigned)widget_mask_has(fresh, k);
    return rank;
}

GeomRect board_draw(const BoardSpec *spec)
{
    const ThemeBoard *b = theme_board(spec->pieces);
    GeomRect art = board_art_rect(spec->pieces);
    if (!b)
        return art;

    draw_slice(DRAW_SLICE_BOARD, geom_rect_inset(art, THEME_BOARD_FRAME), THEME_ALPHA_OPAQUE);
    art_draw(art, THEME_ALPHA_OPAQUE);

    for (int i = 0; i < b->pieces; i++) {
        bool fresh = widget_mask_has(spec->fresh, i);
        bool owned = widget_mask_has(spec->owned, i);
        if (owned &&
            !(fresh && anim_new_piece_scale(spec->fresh_frame, fresh_rank(spec->fresh, i)) <= 0.0f))
            continue;
        GeomRect c = widget_board_cell(art, b->cols, b->cell, i);
        draw_cell(c, i + 1, widget_mask_has(spec->selected, i), widget_mask_has(spec->rare, i));
    }

    for (int col = 1; col < b->cols; col++)
        draw_rect(art.x + col * b->cell - THEME_BOARD_LINE, art.y, THEME_BOARD_LINE, art.h,
                  THEME_WHITE, THEME_ALPHA_GRID);
    for (int row = 1; row < b->rows; row++)
        draw_rect(art.x, art.y + row * b->cell - THEME_BOARD_LINE, art.w, THEME_BOARD_LINE,
                  THEME_WHITE, THEME_ALPHA_GRID);

    /* New pieces on top, so a piece that is still large overlaps its neighbors. */
    for (int i = 0; i < b->pieces; i++) {
        if (!widget_mask_has(spec->fresh, i))
            continue;
        float s = anim_new_piece_scale(spec->fresh_frame, fresh_rank(spec->fresh, i));
        if (s <= 0.0f)
            continue;
        GeomRect c = widget_board_cell(art, b->cols, b->cell, i);
        GeomRect big = widget_scale_rect(c, s);
        /* The whole picture scaled around the same point, so the piece shows its own part. */
        GeomRect pic = {big.x - (c.x - art.x) * big.w / c.w, big.y - (c.y - art.y) * big.h / c.h,
                        art.w * big.w / c.w, art.h * big.h / c.h};
        draw_clip_push(big);
        art_draw(pic, THEME_ALPHA_OPAQUE);
        draw_clip_pop();
    }
    return art;
}
