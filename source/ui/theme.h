#ifndef PSS_UI_THEME_H
#define PSS_UI_THEME_H

#include <3ds/types.h>

/*
 * Design tokens.
 *
 * Colors are named here and defined in theme.c only; no other file may
 * contain a color value (checked by tools/check_colors.sh in CI). Sizes and
 * positions are in 3DS pixels on the 4 px grid where the design allows it.
 */

typedef enum {
    /* Section 4 */
    THEME_PLAZA,
    THEME_PLAZA_HI,
    THEME_PLAZA_DARK,
    THEME_PLAZA_DEEP,
    THEME_PLAZA_LIGHT,
    THEME_PLAZA_MIST,
    THEME_SKY_LIGHT,
    THEME_PAPER,
    THEME_PAPER_LINE,
    THEME_INK,
    THEME_INK_2,
    THEME_INK_3,
    THEME_LINE,
    THEME_COIN,
    THEME_COIN_DARK,
    THEME_COIN_LIGHT,
    THEME_FOCUS,
    THEME_RARE,
    THEME_DANGER,
    THEME_SCRIM,
    /* Supporting tones */
    THEME_WHITE,
    THEME_BLACK,
    THEME_STRIPE_A,
    THEME_STRIPE_B,
    THEME_MEADOW,
    THEME_HILL_A,
    THEME_HILL_B,
    THEME_BAR_TOP,
    THEME_BAR_BOTTOM,
    THEME_BAR_LINE,
    THEME_TRACK,
    THEME_SELECTED,
    THEME_COLOR_COUNT,
} ThemeColor;

/* Opacity levels, 0..255. */
#define THEME_ALPHA_OPAQUE      255
#define THEME_ALPHA_SCRIM       115 /* 45 %, behind dialogs */
#define THEME_ALPHA_DISABLED    115 /* 45 %, locked buttons */
#define THEME_ALPHA_UNAVAILABLE 102 /* 40 %, panels not on the console */
#define THEME_ALPHA_CLOUD       217 /* 85 % */
#define THEME_ALPHA_SHADOW      20  /* 8 %, progress track on the top screen */
#define THEME_ALPHA_OWNED_HAZE  102 /* 40 % white over owned pieces */
#define THEME_ALPHA_GRID        115 /* 45 % white board grid lines */
#define THEME_ALPHA_TOAST       235 /* 92 % */

/* Screens */
#define THEME_TOP_W    400
#define THEME_BOTTOM_W 320
#define THEME_SCREEN_H 240
#define THEME_GRID     4

/* Top screen (section 3) */
#define THEME_AWNING_H          30
#define THEME_AWNING_ROD_H      3
#define THEME_AWNING_CLOTH_H    19
#define THEME_AWNING_SCALLOP_H  8
#define THEME_AWNING_STRIPE_W   20
#define THEME_PILL_Y            36
#define THEME_PILL_H            22
#define THEME_PILL_MARGIN       8
#define THEME_PILL_PAD          10
#define THEME_PILL_COIN_PAD     3
#define THEME_BOARD_Y           64
#define THEME_BOARD_FRAME       5
#define THEME_BOARD_RADIUS      8
#define THEME_PROGRESS_Y        214
#define THEME_PROGRESS_W        150
#define THEME_PROGRESS_H        6
#define THEME_PROGRESS_SMALL_H  5
#define THEME_PROGRESS_TEXT_GAP 8
#define THEME_MEADOW_STOP       62 /* % of the height where sky turns to meadow */

/* Bottom screen (section 3) */
#define THEME_HEADER_H       28
#define THEME_HEADER_TITLE_X 10
#define THEME_CONTENT_Y      28
#define THEME_CONTENT_H      172
#define THEME_CONTENT_PAD    8
#define THEME_BAR_Y          200
#define THEME_BAR_H          40
#define THEME_BAR_MARGIN     6
#define THEME_TOUCH_MIN      28
#define THEME_TILE_GAP       3

/* Components (section 6) */
#define THEME_BTN_H         30
#define THEME_BTN_RADIUS    8
#define THEME_BTN_PAD_L     6
#define THEME_BTN_PAD_R     12
#define THEME_BTN_GAP       6
#define THEME_GLYPH_PX      16
#define THEME_CHIP_H        20
#define THEME_CHIP_RADIUS   10
#define THEME_CHIP_PAD_L    3
#define THEME_CHIP_PAD_R    7
#define THEME_CHIP_GAP      4
#define THEME_PILL_RADIUS   11
#define THEME_CARD_RADIUS   8
#define THEME_CARD_SHADOW   2
#define THEME_PUZZLE_CARD_W 96
#define THEME_PUZZLE_CARD_H 78
#define THEME_PREVIEW_W     86
#define THEME_PREVIEW_H     34
#define THEME_CARD_GAP      8
#define THEME_DIALOG_W      280
#define THEME_DIALOG_RADIUS 12
#define THEME_DIALOG_HEAD_H 26
#define THEME_TILE_RADIUS   5
#define THEME_COIN_SMALL    12
#define THEME_COIN_MEDIUM   16
#define THEME_COIN_LARGE    20
#define THEME_FOCUS_RING    4 /* 2 px white plus 2 px FOCUS outside the element */

/* Widget details (section 6) */
#define THEME_PRESS_PX         1  /* pressed buttons sit this much lower */
#define THEME_TEXT_GAP         5  /* name to count in a pill, coin to number */
#define THEME_TILE_LIFT        1  /* selected pieces are raised */
#define THEME_TILE_CHECK_INSET 10 /* check on a selected piece, from the right edge */
#define THEME_BADGE_INSET      13 /* check badge on an owned piece, from right and bottom edge */
#define THEME_RARE_DOT         6
#define THEME_RARE_DOT_INSET   8
#define THEME_STAR_INSET       12 /* star of a completed card, from the right edge of the picture */
#define THEME_STAR_DROP        -6 /* ... and from its top edge (negative: pokes out above) */
#define THEME_CARD_PAD         5  /* puzzle card: preview inset */
#define THEME_CARD_TEXT_PAD    6
#define THEME_CARD_NAME_Y      41
#define THEME_CARD_PROGRESS_Y  58
#define THEME_CARD_TRACK_Y     5 /* track below the progress text's top */
#define THEME_CARD_TRACK_GAP   4 /* between track and count */
#define THEME_BOARD_SPARK_X    4 /* spark at the board's top right corner */
#define THEME_BOARD_SPARK_Y    14
#define THEME_BOARD_LINE       1
#define THEME_SELECT_EDGE      2 /* green frame of a selected piece on the board */
#define THEME_DIALOG_PAD       12
#define THEME_DIALOG_MIN_H     96
#define THEME_TOAST_H          26
#define THEME_TOAST_PAD        12
#define THEME_TOAST_GAP        8 /* above the button bar */
#define THEME_TOAST_RISE       4
#define THEME_PRICE_GAP        4 /* coin to number of a price tag */

/* S0/S1 and the error screens  */
#define THEME_SIGN_W          236
#define THEME_SIGN_Y          52
#define THEME_SIGN_PAD_TOP    16
#define THEME_SIGN_PAD_BOTTOM 14
#define THEME_SIGN_GAP        8 /* logo to title */
#define THEME_SIGN_ROD_W      3
#define THEME_SIGN_ROD_INSET  40 /* rod from the sign's side edge */
#define THEME_VERSION_MARGIN  8  /* version line from the right and bottom edge */
#define THEME_NOTICE_MARGIN   16 /* card of S1 and errors, from the screen edges */
#define THEME_NOTICE_Y        36
#define THEME_NOTICE_H        132
#define THEME_NOTICE_PAD      12
#define THEME_NOTICE_LINE_H   16
#define THEME_FILE_ROW_H      24
#define THEME_FILE_ROW_GAP    8
#define THEME_FILE_ROW_RADIUS 5
#define THEME_FILE_DOT        8
#define THEME_FILE_DOT_RING   2 /* light ring around the dot */
#define THEME_FILE_DOT_PAD    8
#define THEME_PREVIEW_COLS    2 /* developer list of the error screens */
#define THEME_PREVIEW_ROW_H   36
#define THEME_PREVIEW_X       12

/* Gallery and piece picker  */
#define THEME_GALLERY_X       8
#define THEME_GALLERY_Y       32
#define THEME_PAGER_W         92
#define THEME_PAGER_Y         4
#define THEME_PAGER_PAD       6
#define THEME_GRID_Y          (THEME_CONTENT_Y + THEME_CONTENT_PAD)
#define THEME_SUMMARY_Y       3 /* pieces line in the button bar, from its top */
#define THEME_SUMMARY_PRICE_Y 15

/* Purchase: S4 dialog, S5 bar, S6 success  */
#define THEME_CONFIRM_LINE_H   18
#define THEME_CONFIRM_CHIP_W   24
#define THEME_CONFIRM_CHIP_H   18
#define THEME_CONFIRM_CHIP_GAP 4
#define THEME_CONFIRM_ROWS     2 /* rows of piece numbers; more are cut with "+n" */
#define THEME_BALANCE_H        28
#define THEME_DIALOG_GAP       8
#define THEME_RIBBON_H         24
#define THEME_RIBBON_PAD       22
#define THEME_RIBBON_TAIL      8
#define THEME_RIBBON_RISE      18 /* the ribbon overlaps the board's top edge by this much */
#define THEME_DELTA_H          18
#define THEME_DELTA_PAD        8
#define THEME_SUCCESS_BADGE    52
#define THEME_SUCCESS_RING     4
#define THEME_SUCCESS_GAP      8
#define THEME_BACKUP_CHIP_H    22
#define THEME_STAR_BIG         2 /* scale of the star of a completed puzzle */

/* Backups: S7 list and restore dialog (section 7) */
#define THEME_BACKUP_ROW_H        48
#define THEME_BACKUP_ROW_GAP      6
#define THEME_BACKUP_ROWS         3 /* rows visible in the list */
#define THEME_BACKUP_X            8
#define THEME_BACKUP_Y            (THEME_CONTENT_Y + THEME_CONTENT_PAD)
#define THEME_BACKUP_PAD          10
#define THEME_BACKUP_LINE_GAP     4 /* reason line to date line */
#define THEME_SCROLL_W            4
#define THEME_SCROLL_MARGIN       3 /* scroll bar from the right edge */
#define THEME_LOCK_W              10
#define THEME_LOCK_H              13
#define THEME_RESTORE_LINE_H      16
#define THEME_RESTORE_NOTE_H      28
#define THEME_RESTORE_NOTE_LINE_H 14
#define THEME_BACKUP_CARD_W       280 /* top screen: the focused backup */
#define THEME_BACKUP_CARD_H       96
#define THEME_BACKUP_CARD_Y       76
#define THEME_BACKUP_CARD_PAD     14

/* Settings: S8 (section 7) */
#define THEME_SETTING_ROW_H   44
#define THEME_SETTING_ROW_GAP 8
#define THEME_SETTING_PAD     12
#define THEME_SWITCH_W        38
#define THEME_SWITCH_H        20
#define THEME_SWITCH_KNOB     16
#define THEME_SWITCH_INSET    2
#define THEME_INFO_CARD_W     320
#define THEME_INFO_CARD_Y     64
#define THEME_INFO_PAD        16
#define THEME_INFO_LINE_H     15
#define THEME_INFO_GAP        8

/* Text sizes in pixels (section 5). Nothing is drawn below THEME_TEXT_MIN. */
#define THEME_TEXT_TITLE  15
#define THEME_TEXT_HEADER 13
#define THEME_TEXT_BUTTON 12
#define THEME_TEXT_BODY   11
#define THEME_TEXT_MIN    11

/* Board layouts for the three panel sizes (section 3). */
typedef struct {
    u8 pieces;
    u8 cols;
    u8 rows;
    u8 cell;   /* board cell on the top screen, square */
    u8 tile_w; /* touch tile on the bottom screen */
    u8 tile_h;
} ThemeBoard;

/* Layout for 15, 24 or 40 pieces; NULL for any other count. */
const ThemeBoard *theme_board(int pieces);

typedef struct {
    const char *name;
    ThemeColor color;
} ThemeSwatch;

/* The section 4 palette in document order, for the smoke test. */
extern const ThemeSwatch theme_swatches[];
extern const int theme_swatch_count;

/* 0xRRGGBB of a token. */
u32 theme_rgb(ThemeColor color);

/* citro2d color (ABGR) of a token with the given opacity. */
u32 theme_color(ThemeColor color, u8 alpha);

/* Must be called after C2D_Init so the system font is available. */
void theme_init(void);

/* citro2d scale factor that renders the system font at the given pixel size.
 * Sizes below THEME_TEXT_MIN are raised to it. */
float theme_text_scale(int px);

/* Height of one line of text at the given pixel size, rounded up. */
int theme_text_line_height(int px);

#endif
