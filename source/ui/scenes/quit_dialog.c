#include "ui/scenes/quit_dialog.h"

#include "core/strings.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/scenes/scenes.h"
#include "ui/theme.h"
#include "ui/widgets/button.h"

enum { Q_CANCEL, Q_QUIT };

void quit_dialog_init(QuitDialog *q)
{
    dialog_init(&q->dialog);
    q->focus = Q_CANCEL; /* the safe choice first */
    hit_tap_reset(&q->tap);
    for (int i = 0; i < 2; i++) {
        press_init(&q->press[i]);
        q->rects[i] = (HitRect){0, 0, 0, 0};
    }
}

void quit_dialog_open(QuitDialog *q)
{
    quit_dialog_init(q);
    dialog_open(&q->dialog);
}

bool quit_dialog_active(const QuitDialog *q)
{
    return q->dialog.open;
}

void quit_dialog_update(QuitDialog *q, const InputFrame *in, unsigned keys)
{
    dialog_step(&q->dialog);
    int hit = input_tap(&q->tap, in, q->rects, 2);
    if (dialog_ready(&q->dialog)) {
        int dir = input_direction(keys);
        if (dir == FOCUS_LEFT && q->focus != Q_CANCEL) {
            q->focus = Q_CANCEL;
            audio_play(SFX_MOVE);
        } else if (dir == FOCUS_RIGHT && q->focus != Q_QUIT) {
            q->focus = Q_QUIT;
            audio_play(SFX_MOVE);
        }
        int act = hit;
        if (keys & KEY_A)
            act = q->focus;
        if (keys & KEY_B)
            act = Q_CANCEL;
        if (act == Q_CANCEL) {
            audio_play(SFX_BACK);
            dialog_close(&q->dialog);
        } else if (act == Q_QUIT) {
            audio_play(SFX_BACK);
            scenes_request_quit();
        }
    }
    int pressed = hit_tap_pressed(&q->tap);
    for (int i = 0; i < 2; i++)
        press_update(&q->press[i], pressed == i, ANIM_BUTTON_RELEASE);
}

void quit_dialog_draw_top(const QuitDialog *q)
{
    if (!q->dialog.open)
        return;
    u8 a = (u8)(THEME_ALPHA_SCRIM * tween_value(&q->dialog.scrim));
    draw_rect(0, 0, THEME_TOP_W, THEME_SCREEN_H, THEME_SCRIM, a);
}

void quit_dialog_draw(QuitDialog *q, bool focus_visible)
{
    if (!q->dialog.open)
        return;
    int height = THEME_DIALOG_HEAD_H + THEME_DIALOG_PAD + THEME_CONFIRM_LINE_H + THEME_DIALOG_GAP +
                 THEME_BTN_H + THEME_DIALOG_PAD;
    dialog_draw_scrim(&q->dialog);
    GeomRect inner = dialog_draw_frame(&q->dialog, str(STR_BTN_QUIT), height);

    draw_opacity_push(dialog_content_alpha(&q->dialog));
    int w = inner.w - 2 * THEME_DIALOG_PAD;
    int y = inner.y + THEME_DIALOG_PAD;
    draw_text_fit(inner.x + inner.w / 2, y, w, THEME_TEXT_HEADER, THEME_INK, THEME_ALPHA_OPAQUE,
                  DRAW_ALIGN_CENTER, str(STR_QUIT_QUESTION));
    y += THEME_CONFIRM_LINE_H + THEME_DIALOG_GAP;

    int left = inner.x + THEME_DIALOG_PAD;
    ButtonSpec cancel = {BUTTON_SECONDARY, DRAW_GLYPH_B, str(STR_BTN_CANCEL)};
    ButtonView cv = {false, focus_visible && q->focus == Q_CANCEL,
                     press_offset(&q->press[Q_CANCEL])};
    GeomRect b = button_draw(&cancel, left, y, &cv);
    q->rects[Q_CANCEL] = (HitRect){b.x, b.y, b.w, b.h};

    ButtonSpec quit = {BUTTON_PRIMARY, DRAW_GLYPH_A, str(STR_BTN_QUIT)};
    ButtonView qv = {false, focus_visible && q->focus == Q_QUIT, press_offset(&q->press[Q_QUIT])};
    b = button_draw(&quit, left + w - button_width(&quit), y, &qv);
    q->rects[Q_QUIT] = (HitRect){b.x, b.y, b.w, b.h};
    draw_opacity_pop();
}
