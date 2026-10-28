#ifndef PSS_UI_DRAW_H
#define PSS_UI_DRAW_H

#include <stdbool.h>

#include <3ds.h>
#include <citro2d.h>

#include "core/geom.h"
#include "ui/theme.h"

/*
 * Drawing on top of citro2d: atlas sprites, 9-slices, flat areas and
 * gradients in theme colors, system font text in pixel sizes, button glyphs,
 * ellipsis for long texts and rectangular clipping.
 *
 * All coordinates are whole 3DS pixels. Everything is drawn at depth 0 in
 * call order; later calls cover earlier ones.
 */

/* Images of the atlas gfx/ui.t3s that are drawn as they are. */
typedef enum {
    DRAW_COIN_12,
    DRAW_COIN_16,
    DRAW_COIN_20,
    DRAW_LOGO,
    DRAW_AWNING, /* one 40 px repeat, see draw_awning() */
    DRAW_STRIPES,
    DRAW_CLOUD,
    DRAW_CLOUD_SMALL,
    DRAW_SPARK,
    DRAW_CHECK,       /* on a selected tile */
    DRAW_CHECK_BADGE, /* on an owned tile */
    DRAW_STAR,        /* completed puzzle */
    DRAW_SPRITE_COUNT,
} DrawSprite;

/* Stretchable atlas images. Each one is drawn around a box; shadows, rings
 * and glows reach outside of it by fixed amounts. */
typedef enum {
    DRAW_SLICE_BUTTON,
    DRAW_SLICE_BUTTON_DOWN,
    DRAW_SLICE_BUTTON_PRIMARY,
    DRAW_SLICE_BUTTON_PRIMARY_DOWN,
    DRAW_SLICE_CHIP,
    DRAW_SLICE_CHIP_DOWN,
    DRAW_SLICE_PILL,
    DRAW_SLICE_CARD,
    DRAW_SLICE_DIALOG,
    DRAW_SLICE_DIALOG_HEAD,
    DRAW_SLICE_BOARD,
    DRAW_SLICE_TILE,
    DRAW_SLICE_TILE_SELECTED,
    DRAW_SLICE_TILE_RARE,
    DRAW_SLICE_FOCUS,       /* around a box with radius 8 */
    DRAW_SLICE_FOCUS_SMALL, /* around a box with radius 5 */
    /* White masks, only useful tinted (draw_slice_tinted) */
    DRAW_SLICE_ROUND_3,
    DRAW_SLICE_ROUND_5,
    DRAW_SLICE_ROUND_8,
    DRAW_SLICE_ROUND_12,
    DRAW_SLICE_COUNT,
} DrawSlice;

typedef enum {
    DRAW_ALIGN_LEFT,
    DRAW_ALIGN_CENTER,
    DRAW_ALIGN_RIGHT,
} DrawAlign;

/* Button glyphs of the system font (private use area). A, B, X, Y, L, R are
 * confirmed on hardware. The D-pad glyph is a hypothesis until
 * it is checked on a console in the smoke test. */
#define DRAW_GLYPH_A    ""
#define DRAW_GLYPH_B    ""
#define DRAW_GLYPH_X    ""
#define DRAW_GLYPH_Y    ""
#define DRAW_GLYPH_L    ""
#define DRAW_GLYPH_R    ""
#define DRAW_GLYPH_DPAD ""

/* Loads the atlas from romfs:/gfx/ui.t3x. Needs romfsInit(), C2D_Init() and theme_init(). */
Result draw_init(void);
void draw_fini(void);

/* Once per frame, before the first draw call. */
void draw_frame_begin(void);

/* Starts drawing to a screen target; resets the clip to the whole screen. */
void draw_screen_begin(C3D_RenderTarget *target, int screen_w);

/*
 * Offset and opacity for everything drawn until the matching pop, used by
 * stereo layers and scene transitions. Both nest: offsets add up, opacities
 * multiply. Clip rectangles pushed inside are moved as well. At most 7
 * levels deep; screens start with offset 0 and full opacity.
 */
void draw_translate_push(int dx, int dy);
void draw_translate_pop(void);
void draw_opacity_push(u8 opacity);
void draw_opacity_pop(void);

/* --- Flat areas ---------------------------------------------------------- */

void draw_rect(int x, int y, int w, int h, ThemeColor color, u8 alpha);
void draw_gradient_v(int x, int y, int w, int h, ThemeColor top, ThemeColor bottom, u8 alpha);
void draw_gradient_h(int x, int y, int w, int h, ThemeColor left, ThemeColor right, u8 alpha);
void draw_ellipse(int x, int y, int w, int h, ThemeColor color, u8 alpha);

/* --- Atlas images -------------------------------------------------------- */

int draw_sprite_width(DrawSprite sprite);
int draw_sprite_height(DrawSprite sprite);

void draw_sprite(DrawSprite sprite, int x, int y, u8 alpha);

/* Any image (for example a puzzle picture), stretched to fill `dst`. */
void draw_image(const C2D_Image *image, GeomRect dst, u8 alpha);

/* Scaled around the sprite's center at (cx, cy), for pop and shrink animations. */
void draw_sprite_scaled(DrawSprite sprite, int cx, int cy, float scale, u8 alpha);

/* Repeats a sprite over an area, shifted by (off_x, off_y); clipped to the area. */
void draw_sprite_tiled(DrawSprite sprite, GeomRect area, int off_x, int off_y);

/* The awning across the top of a screen, 30 px plus a 2 px shadow. */
void draw_awning(int screen_w);

/* Draws a 9-slice for the given box. */
void draw_slice(DrawSlice slice, GeomRect box, u8 alpha);

/* Same, with the image's colors replaced by a vertical gradient (use top == bottom
 * for a flat color). Meant for the white DRAW_SLICE_ROUND_* masks. */
void draw_slice_tinted(DrawSlice slice, GeomRect box, ThemeColor top, ThemeColor bottom, u8 alpha);

/* --- Text ---------------------------------------------------------------- */

/* Width of a text in pixels at the given size, rounded up. */
int draw_text_width(int px, const char *text);

/* Height of one line at the given size, see theme_text_line_height(). */
int draw_text_height(int px);

/*
 * Draws one line of text. x is the left edge, center or right edge depending
 * on align; y is the top of the line. Returns the drawn width.
 */
int draw_text(int x, int y, int px, ThemeColor color, u8 alpha, DrawAlign align, const char *text);

/*
 * Like draw_text, but never wider than max_w: a text that does not fit ends
 * with "…". Returns the drawn width.
 */
int draw_text_fit(int x, int y, int max_w, int px, ThemeColor color, u8 alpha, DrawAlign align,
                  const char *text);

/*
 * Draws text broken into lines of at most max_w, at spaces; line_h apart.
 * At most max_lines (up to DRAW_WRAP_MAX_LINES) lines; if the text needs
 * more, the last one ends with "…". Returns the number of lines drawn.
 */
#define DRAW_WRAP_MAX_LINES 8
int draw_text_wrapped(int x, int y, int max_w, int line_h, int max_lines, int px, ThemeColor color,
                      u8 alpha, DrawAlign align, const char *text);

/* Draws text vertically centered in a box of height h starting at y. */
int draw_text_vcenter(int x, int y, int h, int px, ThemeColor color, u8 alpha, DrawAlign align,
                      const char *text);

/* --- Clipping ------------------------------------------------------------ */

/* Restricts drawing to r, intersected with the current clip. Nestable. */
void draw_clip_push(GeomRect r);
void draw_clip_pop(void);

#endif
