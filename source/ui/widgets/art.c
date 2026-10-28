#include "ui/widgets/art.h"

#include "ui/art_source.h"
#include "ui/draw.h"

static int s_panel = ART_PLACEHOLDER;

void art_use(int panel)
{
    s_panel = panel;
}

static void placeholder_draw(GeomRect r, unsigned char alpha)
{
    draw_gradient_v(r.x, r.y, r.w, r.h * 3 / 5, THEME_FOCUS, THEME_SKY_LIGHT, alpha);
    draw_rect(r.x, r.y + r.h * 3 / 5, r.w, r.h - r.h * 3 / 5, THEME_HILL_B, alpha);
    draw_ellipse(r.x - r.w / 4, r.y + r.h / 2, r.w, r.h, THEME_PLAZA, alpha);
    draw_ellipse(r.x + r.w / 3, r.y + r.h * 3 / 5, r.w, r.h, THEME_PLAZA_DARK, alpha);
    draw_ellipse(r.x + r.w * 3 / 4, r.y + r.h / 8, r.h / 4, r.h / 4, THEME_COIN, alpha);
}

void art_draw(GeomRect r, unsigned char alpha)
{
    const C2D_Image *image = s_panel >= 0 ? art_source_get((unsigned)s_panel) : NULL;
    draw_clip_push(r);
    if (image != NULL) {
        /* Fill the rectangle without stretching: crop the picture's longer side, centred. */
        Tex3DS_SubTexture sub = *image->subtex;
        float image_aspect = (float)sub.width / (float)sub.height;
        float rect_aspect = (float)r.w / (float)r.h;
        if (rect_aspect > image_aspect) {
            float cut = (1.0f - image_aspect / rect_aspect) / 2.0f * (sub.bottom - sub.top);
            sub.top += cut;
            sub.bottom -= cut;
        } else if (rect_aspect < image_aspect) {
            float cut = (1.0f - rect_aspect / image_aspect) / 2.0f * (sub.right - sub.left);
            sub.left += cut;
            sub.right -= cut;
        }
        C2D_Image cropped = {image->tex, &sub};
        draw_image(&cropped, r, alpha);
    } else
        placeholder_draw(r, alpha);
    draw_clip_pop();
}
