#ifndef PSS_UI_ANIM_H
#define PSS_UI_ANIM_H

#include "core/tween.h"

/*
 * Motion tokens in frames at 60 fps.
 * Scenes use these constants and helpers instead of numbers of their own,
 * so the feel can be tuned in one place.
 */

#define ANIM_BOOT_FADE        12 /* S0 sign fades in */
#define ANIM_SCREEN           12 /* screen change: slide and fade, ease-out-cubic */
#define ANIM_SCREEN_SHIFT_PX  16 /* bottom screen slide distance */
#define ANIM_PAGE             10 /* gallery page slide */
#define ANIM_BUTTON_RELEASE   6  /* button springs back, ease-out-back */
#define ANIM_PIECE_SELECT     8  /* piece bump 1.00 -> 1.08 -> 1.00 */
#define ANIM_DIALOG_SCRIM     8  /* scrim fades in, linear */
#define ANIM_DIALOG_POP       10 /* dialog scales 0.92 -> 1.00, ease-out-back */
#define ANIM_FOCUS_PERIOD     60 /* focus ring opacity cycle */
#define ANIM_NEW_PIECE        14 /* each new piece pops in, ease-out-back */
#define ANIM_NEW_PIECE_OFFSET 4  /* delay between new pieces */
#define ANIM_COIN_STEP        2  /* one coin every 2 frames */
#define ANIM_COIN_MAX         45 /* whole count never longer */
#define ANIM_COIN_SOUND_EVERY 5  /* coin sound every 5 coins */

/* Endless background motion as num/den px per frame. */
#define ANIM_STRIPES_NUM 1 /* 0.25 px per frame */
#define ANIM_STRIPES_DEN 4
#define ANIM_CLOUDS_NUM  1 /* 0.1 px per frame */
#define ANIM_CLOUDS_DEN  10

/* Advance the global frame counter; call once per frame. */
void anim_tick(void);
unsigned anim_frame(void);

/* Focus ring opacity 0..255 for the current frame. */
unsigned anim_focus_alpha(void);

/* Whole-pixel offsets of the background layers, wrapped to `period` px. */
int anim_stripes_px(unsigned period);
int anim_clouds_px(unsigned period);

/* Ready-made tweens for the events in section 8. */
void anim_screen_in(Tween *t);      /* 0 -> 1: fade and slide progress */
void anim_page_slide(Tween *t);     /* 0 -> 1 */
void anim_button_release(Tween *t); /* 1 -> 0: pixels still pressed down */
void anim_dialog_scrim(Tween *t);   /* 0 -> 1: scrim opacity factor */
void anim_dialog_pop(Tween *t);     /* 0.92 -> 1.00 scale */

/* Scale of a selected piece `frame` frames after the tap. */
float anim_piece_select_scale(unsigned frame);

/* Scale of new piece `index` in the success animation, 1.3 -> 1.0; 0 = not visible yet. */
float anim_new_piece_scale(unsigned frame, unsigned index);

#endif
