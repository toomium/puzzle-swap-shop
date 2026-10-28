#include "ui/anim.h"

#define PIECE_SELECT_PEAK 1.08f
#define DIALOG_POP_FROM   0.92f
#define NEW_PIECE_FROM    1.3f

static unsigned s_frame;

void anim_tick(void)
{
    s_frame++;
}

unsigned anim_frame(void)
{
    return s_frame;
}

unsigned anim_focus_alpha(void)
{
    return easing_pulse_alpha(s_frame, ANIM_FOCUS_PERIOD);
}

int anim_stripes_px(unsigned period)
{
    return tween_scroll_px(s_frame, ANIM_STRIPES_NUM, ANIM_STRIPES_DEN, period);
}

int anim_clouds_px(unsigned period)
{
    return tween_scroll_px(s_frame, ANIM_CLOUDS_NUM, ANIM_CLOUDS_DEN, period);
}

void anim_screen_in(Tween *t)
{
    tween_start(t, 0.0f, 1.0f, ANIM_SCREEN, 0, EASE_OUT_CUBIC);
}

void anim_page_slide(Tween *t)
{
    tween_start(t, 0.0f, 1.0f, ANIM_PAGE, 0, EASE_OUT_CUBIC);
}

void anim_button_release(Tween *t)
{
    tween_start(t, 1.0f, 0.0f, ANIM_BUTTON_RELEASE, 0, EASE_OUT_BACK);
}

void anim_dialog_scrim(Tween *t)
{
    tween_start(t, 0.0f, 1.0f, ANIM_DIALOG_SCRIM, 0, EASE_LINEAR);
}

void anim_dialog_pop(Tween *t)
{
    tween_start(t, DIALOG_POP_FROM, 1.0f, ANIM_DIALOG_POP, 0, EASE_OUT_BACK);
}

float anim_piece_select_scale(unsigned frame)
{
    return tween_bump(frame, ANIM_PIECE_SELECT, 1.0f, PIECE_SELECT_PEAK);
}

float anim_new_piece_scale(unsigned frame, unsigned index)
{
    unsigned start = index * ANIM_NEW_PIECE_OFFSET;
    if (frame <= start)
        return 0.0f;
    float p = tween_stagger(frame, index, ANIM_NEW_PIECE, ANIM_NEW_PIECE_OFFSET);
    return NEW_PIECE_FROM + (1.0f - NEW_PIECE_FROM) * easing_out_back(p);
}
