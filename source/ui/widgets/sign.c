#include "ui/widgets/sign.h"

#include <stdio.h>

#include "core/strings.h"
#include "ui/draw.h"
#include "ui/theme.h"

GeomRect sign_draw(const char *title, const char *subtitle, unsigned char alpha)
{
    int logo_h = draw_sprite_height(DRAW_LOGO);
    int title_h = draw_text_height(THEME_TEXT_TITLE);
    int sub_h = draw_text_height(THEME_TEXT_BODY);
    GeomRect box = {(THEME_TOP_W - THEME_SIGN_W) / 2, THEME_SIGN_Y, THEME_SIGN_W,
                    THEME_SIGN_PAD_TOP + logo_h + THEME_SIGN_GAP + title_h + sub_h +
                        THEME_SIGN_PAD_BOTTOM};

    draw_opacity_push(alpha);
    int rod_top = THEME_AWNING_H - THEME_AWNING_SCALLOP_H;
    int rod_h = box.y - rod_top;
    draw_rect(box.x + THEME_SIGN_ROD_INSET, rod_top, THEME_SIGN_ROD_W, rod_h, THEME_BAR_LINE,
              THEME_ALPHA_OPAQUE);
    draw_rect(box.x + box.w - THEME_SIGN_ROD_INSET - THEME_SIGN_ROD_W, rod_top, THEME_SIGN_ROD_W,
              rod_h, THEME_BAR_LINE, THEME_ALPHA_OPAQUE);
    draw_slice(DRAW_SLICE_DIALOG, box, THEME_ALPHA_OPAQUE);

    int y = box.y + THEME_SIGN_PAD_TOP;
    draw_sprite(DRAW_LOGO, box.x + (box.w - draw_sprite_width(DRAW_LOGO)) / 2, y,
                THEME_ALPHA_OPAQUE);
    y += logo_h + THEME_SIGN_GAP;
    int cx = box.x + box.w / 2;
    draw_text_fit(cx, y, box.w - 2 * THEME_SIGN_PAD_TOP, THEME_TEXT_TITLE, THEME_INK,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, title);
    draw_text_fit(cx, y + title_h, box.w - 2 * THEME_SIGN_PAD_TOP, THEME_TEXT_BODY, THEME_INK_2,
                  THEME_ALPHA_OPAQUE, DRAW_ALIGN_CENTER, subtitle);
    draw_opacity_pop();
    return box;
}

void sign_version_draw(const char *version)
{
    char line[48];
    snprintf(line, sizeof(line), str(STR_VERSION_LINE), version);
    int y = THEME_SCREEN_H - THEME_VERSION_MARGIN - draw_text_height(THEME_TEXT_BODY);
    draw_text(THEME_TOP_W - THEME_VERSION_MARGIN, y, THEME_TEXT_BODY, THEME_INK_3,
              THEME_ALPHA_OPAQUE, DRAW_ALIGN_RIGHT, line);
}
