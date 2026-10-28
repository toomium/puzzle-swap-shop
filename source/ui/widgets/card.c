#include "ui/widgets/card.h"

#include <stdio.h>

#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"
#include "ui/widgets/art.h"
#include "ui/widgets/progress.h"

void card_draw(GeomRect box, const CardView *view)
{
    u8 alpha = view->available ? view->alpha : (u8)(view->alpha * THEME_ALPHA_UNAVAILABLE / 255);
    if (view->focus)
        draw_slice(DRAW_SLICE_FOCUS, box, (u8)anim_focus_alpha());
    draw_slice(DRAW_SLICE_CARD, box, alpha);
}

void puzzle_card_draw(GeomRect box, const PuzzleCardSpec *spec, bool focus, bool available)
{
    bool done = available && spec->total > 0 && spec->owned >= spec->total;
    u8 a = available ? THEME_ALPHA_OPAQUE : THEME_ALPHA_UNAVAILABLE;
    CardView cv = {focus, available, THEME_ALPHA_OPAQUE};
    card_draw(box, &cv);

    GeomRect thumb = {box.x + THEME_CARD_PAD, box.y + THEME_CARD_PAD, THEME_PREVIEW_W,
                      THEME_PREVIEW_H};
    draw_slice_tinted(DRAW_SLICE_ROUND_5, thumb, THEME_SKY_LIGHT, THEME_SKY_LIGHT, a);
    art_draw(thumb, a);
    if (done)
        draw_sprite(DRAW_STAR, thumb.x + thumb.w - THEME_STAR_INSET, thumb.y + THEME_STAR_DROP, a);

    draw_text_fit(box.x + THEME_CARD_TEXT_PAD, box.y + THEME_CARD_NAME_Y,
                  box.w - 2 * THEME_CARD_TEXT_PAD, THEME_TEXT_BODY, THEME_INK, a, DRAW_ALIGN_LEFT,
                  spec->name);

    char num[16];
    snprintf(num, sizeof(num), "%u/%u", spec->owned, spec->total);
    int num_w = draw_text_width(THEME_TEXT_BODY, num);
    int py = box.y + THEME_CARD_PROGRESS_Y;
    GeomRect track = {box.x + THEME_CARD_TEXT_PAD, py + THEME_CARD_TRACK_Y,
                      box.w - 2 * THEME_CARD_TEXT_PAD - num_w - THEME_CARD_TRACK_GAP,
                      THEME_PROGRESS_SMALL_H};
    progress_draw(track, available ? spec->owned : 0, spec->total, PROGRESS_ON_CARD, a);
    draw_text(box.x + box.w - THEME_CARD_TEXT_PAD, py, THEME_TEXT_BODY, THEME_INK_2, a,
              DRAW_ALIGN_RIGHT, num);
}
