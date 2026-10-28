#include "ui/widgets/toast_view.h"

#include "core/geom.h"
#include "ui/draw.h"
#include "ui/theme.h"

void toast_widget_init(Toast *t)
{
    toast_init(t, TOAST_IN_FRAMES, TOAST_HOLD_FRAMES, TOAST_OUT_FRAMES);
}

void toast_widget_draw(const Toast *t)
{
    if (!toast_visible(t))
        return;
    u8 fade = toast_alpha(t);
    if (fade == 0)
        return;
    int max_w = THEME_BOTTOM_W - 2 * THEME_CONTENT_PAD;
    int text_w = draw_text_width(THEME_TEXT_BODY, t->text);
    int w = text_w + 2 * THEME_TOAST_PAD;
    if (w > max_w)
        w = max_w;
    GeomRect box = {(THEME_BOTTOM_W - w) / 2,
                    THEME_BAR_Y - THEME_TOAST_GAP - THEME_TOAST_H +
                        toast_offset(t, THEME_TOAST_RISE),
                    w, THEME_TOAST_H};
    draw_opacity_push(fade);
    draw_slice_tinted(DRAW_SLICE_ROUND_12, box, THEME_INK, THEME_INK, THEME_ALPHA_TOAST);
    draw_text_fit(box.x + THEME_TOAST_PAD, box.y + (box.h - draw_text_height(THEME_TEXT_BODY)) / 2,
                  w - 2 * THEME_TOAST_PAD, THEME_TEXT_BODY, THEME_WHITE, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_LEFT, t->text);
    draw_opacity_pop();
}
