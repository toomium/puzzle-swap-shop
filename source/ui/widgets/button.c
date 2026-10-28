#include "ui/widgets/button.h"

#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"

int button_width(const ButtonSpec *spec)
{
    int w = THEME_BTN_PAD_L + THEME_BTN_PAD_R + draw_text_width(THEME_TEXT_BUTTON, spec->label);
    if (spec->glyph)
        w += THEME_GLYPH_PX + THEME_BTN_GAP;
    return w;
}

GeomRect button_box(const ButtonSpec *spec, int x, int y)
{
    GeomRect box = {x, y, button_width(spec), THEME_BTN_H};
    return box;
}

GeomRect button_draw(const ButtonSpec *spec, int x, int y, const ButtonView *view)
{
    GeomRect box = button_box(spec, x, y);
    bool down = view->press_px > 0 && !view->disabled;
    bool primary = spec->kind == BUTTON_PRIMARY && !view->disabled;
    u8 alpha = view->disabled ? THEME_ALPHA_DISABLED : THEME_ALPHA_OPAQUE;

    DrawSlice slice = primary ? (down ? DRAW_SLICE_BUTTON_PRIMARY_DOWN : DRAW_SLICE_BUTTON_PRIMARY)
                              : (down ? DRAW_SLICE_BUTTON_DOWN : DRAW_SLICE_BUTTON);
    ThemeColor ink = primary ? THEME_WHITE : THEME_INK;

    if (view->focus && !view->disabled)
        draw_slice(DRAW_SLICE_FOCUS, box, (u8)anim_focus_alpha());
    draw_slice(slice, box, alpha);

    int dy = down ? THEME_PRESS_PX : 0;
    int tx = box.x + THEME_BTN_PAD_L;
    if (spec->glyph) {
        draw_text_vcenter(tx, box.y + dy, THEME_BTN_H, THEME_GLYPH_PX, ink, alpha, DRAW_ALIGN_LEFT,
                          spec->glyph);
        tx += THEME_GLYPH_PX + THEME_BTN_GAP;
    }
    draw_text_vcenter(tx, box.y + dy, THEME_BTN_H, THEME_TEXT_BUTTON, ink, alpha, DRAW_ALIGN_LEFT,
                      spec->label);
    return box;
}
