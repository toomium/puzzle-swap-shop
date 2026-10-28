#include "ui/widgets/chip.h"

#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"

int chip_width(const ChipSpec *spec)
{
    int w = THEME_CHIP_PAD_L + THEME_CHIP_PAD_R + draw_text_width(THEME_TEXT_BODY, spec->label);
    if (spec->glyph)
        w += THEME_GLYPH_PX + THEME_CHIP_GAP;
    return w;
}

GeomRect chip_box(const ChipSpec *spec, int x, int y)
{
    GeomRect box = {x, y, chip_width(spec), THEME_CHIP_H};
    return box;
}

GeomRect chip_draw(const ChipSpec *spec, int x, int y, const ChipView *view)
{
    GeomRect box = chip_box(spec, x, y);
    if (view->focus)
        draw_slice(DRAW_SLICE_FOCUS, box, (u8)anim_focus_alpha());
    draw_slice(view->pressed ? DRAW_SLICE_CHIP_DOWN : DRAW_SLICE_CHIP, box, THEME_ALPHA_OPAQUE);

    int tx = box.x + THEME_CHIP_PAD_L;
    if (spec->glyph) {
        draw_text_vcenter(tx, box.y, THEME_CHIP_H, THEME_GLYPH_PX, THEME_INK, THEME_ALPHA_OPAQUE,
                          DRAW_ALIGN_LEFT, spec->glyph);
        tx += THEME_GLYPH_PX + THEME_CHIP_GAP;
    }
    draw_text_vcenter(tx, box.y, THEME_CHIP_H, THEME_TEXT_BODY, THEME_INK_2, THEME_ALPHA_OPAQUE,
                      DRAW_ALIGN_LEFT, spec->label);
    return box;
}
