#include "ui/widgets/dialog.h"

#include "core/widget_math.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"

void dialog_init(Dialog *d)
{
    d->open = false;
    anim_dialog_scrim(&d->scrim);
    anim_dialog_pop(&d->pop);
}

void dialog_open(Dialog *d)
{
    d->open = true;
    anim_dialog_scrim(&d->scrim);
    anim_dialog_pop(&d->pop);
}

void dialog_close(Dialog *d)
{
    d->open = false;
}

void dialog_step(Dialog *d)
{
    if (!d->open)
        return;
    tween_step(&d->scrim);
    tween_step(&d->pop);
}

bool dialog_ready(const Dialog *d)
{
    return d->open && tween_done(&d->pop) && tween_done(&d->scrim);
}

GeomRect dialog_box(int height)
{
    if (height < THEME_DIALOG_MIN_H)
        height = THEME_DIALOG_MIN_H;
    /* On the 4 px grid, a little above the middle . */
    int y = ((THEME_SCREEN_H - height) / 2 / THEME_GRID) * THEME_GRID;
    GeomRect box = {(THEME_BOTTOM_W - THEME_DIALOG_W) / 2, y, THEME_DIALOG_W, height};
    return box;
}

void dialog_draw_scrim(const Dialog *d)
{
    if (!d->open)
        return;
    u8 a = (u8)(THEME_ALPHA_SCRIM * tween_value(&d->scrim));
    draw_rect(0, 0, THEME_BOTTOM_W, THEME_SCREEN_H, THEME_SCRIM, a);
}

unsigned char dialog_content_alpha(const Dialog *d)
{
    if (!d->open)
        return 0;
    float p = tween_progress(&d->pop);
    float a = (p - 0.4f) / 0.6f;
    return a <= 0.0f ? 0 : a >= 1.0f ? THEME_ALPHA_OPAQUE : (u8)(a * THEME_ALPHA_OPAQUE);
}

GeomRect dialog_draw_frame(const Dialog *d, const char *title, int height)
{
    GeomRect final = dialog_box(height);
    GeomRect inner = {final.x, final.y + THEME_DIALOG_HEAD_H, final.w,
                      final.h - THEME_DIALOG_HEAD_H};
    if (!d->open)
        return inner;

    GeomRect box = widget_scale_rect(final, tween_value(&d->pop));
    draw_slice(DRAW_SLICE_DIALOG, box, THEME_ALPHA_OPAQUE);
    draw_slice(DRAW_SLICE_DIALOG_HEAD, (GeomRect){box.x, box.y, box.w, THEME_DIALOG_HEAD_H},
               THEME_ALPHA_OPAQUE);
    draw_text_vcenter(box.x + box.w / 2, box.y, THEME_DIALOG_HEAD_H, THEME_TEXT_BUTTON, THEME_WHITE,
                      dialog_content_alpha(d), DRAW_ALIGN_CENTER, title);
    return inner;
}
