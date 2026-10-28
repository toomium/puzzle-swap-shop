#include "ui/widgets/tile.h"

#include <stdio.h>

#include "core/widget_math.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"
#include "ui/widgets/art.h"

void tile_draw(GeomRect box, const TileSpec *spec, const TileView *view)
{
    char num[8];
    snprintf(num, sizeof(num), "%d", spec->number);
    u8 alpha =
        spec->locked && spec->state == TILE_MISSING ? THEME_ALPHA_DISABLED : THEME_ALPHA_OPAQUE;

    if (view->bump != 1.0f)
        box = widget_scale_rect(box, view->bump);
    if (view->focus)
        draw_slice(DRAW_SLICE_FOCUS_SMALL, box, (u8)anim_focus_alpha());

    switch (spec->state) {
    case TILE_MISSING:
        draw_slice(spec->rare ? DRAW_SLICE_TILE_RARE : DRAW_SLICE_TILE, box, alpha);
        draw_text_vcenter(box.x + box.w / 2, box.y, box.h, THEME_TEXT_BODY, THEME_INK_2, alpha,
                          DRAW_ALIGN_CENTER, num);
        break;
    case TILE_SELECTED:
        box.y -= THEME_TILE_LIFT;
        draw_slice(DRAW_SLICE_TILE_SELECTED, box, alpha);
        draw_text_vcenter(box.x + box.w / 2, box.y, box.h, THEME_TEXT_BODY, THEME_WHITE, alpha,
                          DRAW_ALIGN_CENTER, num);
        draw_sprite(DRAW_CHECK, box.x + box.w - THEME_TILE_CHECK_INSET, box.y + 1, alpha);
        break;
    case TILE_OWNED:
        draw_clip_push(box);
        art_draw(spec->art, THEME_ALPHA_OPAQUE);
        draw_clip_pop();
        draw_rect(box.x, box.y, box.w, box.h, THEME_WHITE, THEME_ALPHA_OWNED_HAZE);
        draw_sprite(DRAW_CHECK_BADGE, box.x + box.w - THEME_BADGE_INSET,
                    box.y + box.h - THEME_BADGE_INSET, THEME_ALPHA_OPAQUE);
        break;
    }
    if (spec->rare && spec->state != TILE_OWNED)
        draw_ellipse(box.x + box.w - THEME_RARE_DOT_INSET, box.y + 2, THEME_RARE_DOT,
                     THEME_RARE_DOT, THEME_RARE, alpha);
}
