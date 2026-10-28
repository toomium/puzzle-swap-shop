#include "ui/backdrop.h"

#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/theme.h"

/* Sky and clouds are wider than the screen, so a shifted layer leaves no gap at the edge. */
#define OVERSCAN STEREO_MAX_SHIFT

/* Positions of the two clouds, in px from the left and from the top. */
#define CLOUD_X       30
#define CLOUD_Y       70
#define CLOUD_SMALL_X 300
#define CLOUD_SMALL_Y 96

/* Hills as ellipses: x, y, w, h. */
#define HILL_A_X -100
#define HILL_A_Y 200
#define HILL_A_W 320
#define HILL_A_H 100
#define HILL_B_X 110
#define HILL_B_Y 200
#define HILL_B_W 400
#define HILL_B_H 112

static void draw_cloud(DrawSprite sprite, int x, int y)
{
    /* Clouds drift to the right and re-enter from the left. */
    int w = draw_sprite_width(sprite);
    int span = THEME_TOP_W + w;
    int drift = anim_clouds_px((unsigned)span);
    draw_sprite(sprite, (x + drift) % span - w / 2, y, THEME_ALPHA_CLOUD);
}

void backdrop_top_draw(const StereoView *view)
{
    int meadow_y = THEME_SCREEN_H * THEME_MEADOW_STOP / 100;

    stereo_layer_begin(view, STEREO_LAYER_BACKGROUND);
    draw_gradient_v(-OVERSCAN, 0, THEME_TOP_W + 2 * OVERSCAN, meadow_y, THEME_SKY_LIGHT,
                    THEME_MEADOW, THEME_ALPHA_OPAQUE);
    draw_gradient_v(-OVERSCAN, meadow_y, THEME_TOP_W + 2 * OVERSCAN, THEME_SCREEN_H - meadow_y,
                    THEME_MEADOW, THEME_PLAZA_LIGHT, THEME_ALPHA_OPAQUE);
    draw_ellipse(HILL_A_X, HILL_A_Y, HILL_A_W, HILL_A_H, THEME_HILL_A, THEME_ALPHA_OPAQUE);
    draw_ellipse(HILL_B_X, HILL_B_Y, HILL_B_W, HILL_B_H, THEME_HILL_B, THEME_ALPHA_OPAQUE);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_CLOUDS);
    draw_cloud(DRAW_CLOUD, CLOUD_X, CLOUD_Y);
    draw_cloud(DRAW_CLOUD_SMALL, CLOUD_SMALL_X, CLOUD_SMALL_Y);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    GeomRect awning = {-OVERSCAN, 0, THEME_TOP_W + 2 * OVERSCAN, draw_sprite_height(DRAW_AWNING)};
    draw_sprite_tiled(DRAW_AWNING, awning, OVERSCAN, 0); /* keeps the stripes at the same place */
    stereo_layer_end();
}

void backdrop_bottom_draw(void)
{
    int w = draw_sprite_width(DRAW_STRIPES);
    int off = anim_stripes_px((unsigned)w);
    draw_rect(0, 0, THEME_BOTTOM_W, THEME_SCREEN_H, THEME_STRIPE_A, THEME_ALPHA_OPAQUE);
    draw_sprite_tiled(DRAW_STRIPES, (GeomRect){0, 0, THEME_BOTTOM_W, THEME_SCREEN_H}, off, off);
}
