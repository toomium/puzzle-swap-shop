#include "ui/widgets/chrome.h"

#include "ui/draw.h"
#include "ui/theme.h"

void chrome_header_draw(const char *title, const char *right_note)
{
    draw_rect(0, 0, THEME_BOTTOM_W, THEME_HEADER_H, THEME_WHITE, THEME_ALPHA_OPAQUE);
    draw_rect(0, THEME_HEADER_H - 1, THEME_BOTTOM_W, 1, THEME_LINE, THEME_ALPHA_OPAQUE);
    draw_text_vcenter(THEME_HEADER_TITLE_X, 0, THEME_HEADER_H, THEME_TEXT_HEADER, THEME_INK,
                      THEME_ALPHA_OPAQUE, DRAW_ALIGN_LEFT, title);
    if (right_note)
        draw_text_vcenter(THEME_BOTTOM_W - THEME_CONTENT_PAD, 0, THEME_HEADER_H, THEME_TEXT_BODY,
                          THEME_INK_3, THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT, right_note);
}

void chrome_bar_draw(void)
{
    draw_gradient_v(0, THEME_BAR_Y, THEME_BOTTOM_W, THEME_BAR_H, THEME_BAR_TOP, THEME_BAR_BOTTOM,
                    THEME_ALPHA_OPAQUE);
    draw_rect(0, THEME_BAR_Y, THEME_BOTTOM_W, 1, THEME_BAR_LINE, THEME_ALPHA_OPAQUE);
}

int chrome_bar_button_y(void)
{
    return THEME_BAR_Y + (THEME_BAR_H - THEME_BTN_H) / 2 - 1;
}

GeomRect chrome_bar_button(const ButtonSpec *spec, bool right, const ButtonView *view)
{
    int x = right ? THEME_BOTTOM_W - THEME_BAR_MARGIN - button_width(spec) : THEME_BAR_MARGIN;
    return button_draw(spec, x, chrome_bar_button_y(), view);
}

GeomRect chrome_content(void)
{
    GeomRect r = {0, THEME_CONTENT_Y, THEME_BOTTOM_W, THEME_CONTENT_H};
    return r;
}
