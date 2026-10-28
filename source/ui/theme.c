#include "ui/theme.h"

#include <math.h>

#include <citro2d.h>

/* The only place with color values. */
/* clang-format off */
static const u32 s_rgb[THEME_COLOR_COUNT] = {
    [THEME_PLAZA] = 0x5BB83C,
    [THEME_PLAZA_HI] = 0x7CD154,
    [THEME_PLAZA_DARK] = 0x3C8A25,
    [THEME_PLAZA_DEEP] = 0x2E6E1C,
    [THEME_PLAZA_LIGHT] = 0xE4F4D8,
    [THEME_PLAZA_MIST] = 0xF2FAEC,
    [THEME_SKY_LIGHT] = 0xE3F6FD,
    [THEME_PAPER] = 0xFFF7E6,
    [THEME_PAPER_LINE] = 0xEADCBC,
    [THEME_INK] = 0x33363A,
    [THEME_INK_2] = 0x6B7177,
    [THEME_INK_3] = 0xA3AAB0,
    [THEME_LINE] = 0xD3DBCD,
    [THEME_COIN] = 0xF7BE16,
    [THEME_COIN_DARK] = 0xC88A00,
    [THEME_COIN_LIGHT] = 0xFFE48A,
    [THEME_FOCUS] = 0x2DB3F0,
    [THEME_RARE] = 0xEE6AA7,
    [THEME_DANGER] = 0xE5483C,
    [THEME_SCRIM] = 0x1C2818,
    [THEME_WHITE] = 0xFFFFFF,
    [THEME_BLACK] = 0x000000,
    [THEME_STRIPE_A] = 0xF0F8E9,
    [THEME_STRIPE_B] = 0xE8F4DF,
    [THEME_MEADOW] = 0xF4FBF2,
    [THEME_HILL_A] = 0xCDEBB6,
    [THEME_HILL_B] = 0xC3E6A8,
    [THEME_BAR_TOP] = 0xF8F9F7,
    [THEME_BAR_BOTTOM] = 0xE9ECE6,
    [THEME_BAR_LINE] = 0xCDD4C8,
    [THEME_TRACK] = 0xE5EAE1,
    [THEME_SELECTED] = 0xE6F6DA,
};
/* clang-format on */

const ThemeSwatch theme_swatches[] = {
    {"Plaza", THEME_PLAZA},
    {"Plaza hi", THEME_PLAZA_HI},
    {"Plaza dark", THEME_PLAZA_DARK},
    {"Plaza deep", THEME_PLAZA_DEEP},
    {"Plaza light", THEME_PLAZA_LIGHT},
    {"Plaza mist", THEME_PLAZA_MIST},
    {"Sky light", THEME_SKY_LIGHT},
    {"Paper", THEME_PAPER},
    {"Paper line", THEME_PAPER_LINE},
    {"Ink", THEME_INK},
    {"Ink 2", THEME_INK_2},
    {"Ink 3", THEME_INK_3},
    {"Line", THEME_LINE},
    {"Coin", THEME_COIN},
    {"Coin dark", THEME_COIN_DARK},
    {"Coin light", THEME_COIN_LIGHT},
    {"Focus", THEME_FOCUS},
    {"Rare", THEME_RARE},
    {"Danger", THEME_DANGER},
    {"Scrim", THEME_SCRIM},
};

const int theme_swatch_count = sizeof(theme_swatches) / sizeof(theme_swatches[0]);

static const ThemeBoard s_boards[] = {
    {15, 5, 3, 40, 52, 48},
    {24, 6, 4, 34, 40, 36},
    {40, 8, 5, 25, 33, 29},
};

const ThemeBoard *theme_board(int pieces)
{
    for (size_t i = 0; i < sizeof(s_boards) / sizeof(s_boards[0]); i++) {
        if (s_boards[i].pieces == pieces)
            return &s_boards[i];
    }
    return NULL;
}

u32 theme_rgb(ThemeColor color)
{
    if ((unsigned)color >= THEME_COLOR_COUNT)
        return s_rgb[THEME_DANGER];
    return s_rgb[color];
}

u32 theme_color(ThemeColor color, u8 alpha)
{
    u32 rgb = theme_rgb(color);
    return C2D_Color32((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, alpha);
}

static float s_font_px = 30.0f;
static float s_line_feed = 30.0f;

void theme_init(void)
{
    /* Scale 1.0 renders the system font at its nominal glyph height.
     * px / height gave the sizes checked on hardware. */
    FINF_s *info = C2D_FontGetInfo(NULL);
    if (info && info->height > 0) {
        s_font_px = (float)info->height;
        s_line_feed = info->lineFeed > 0 ? (float)info->lineFeed : s_font_px;
    }
}

float theme_text_scale(int px)
{
    if (px < THEME_TEXT_MIN)
        px = THEME_TEXT_MIN;
    return (float)px / s_font_px;
}

int theme_text_line_height(int px)
{
    return (int)ceilf(s_line_feed * theme_text_scale(px));
}
