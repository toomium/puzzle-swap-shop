#ifndef PSS_UI_SCENES_QUIT_DIALOG_H
#define PSS_UI_SCENES_QUIT_DIALOG_H

#include <stdbool.h>

#include "core/hit.h"
#include "core/press.h"
#include "ui/input.h"
#include "ui/widgets/dialog.h"

/*
 * "App beenden?": the only way to leave the
 * app from a normal screen. Ⓑ never leaves by itself; it opens this dialog
 * where Ⓑ cancels. The owner scene calls quit_dialog_update first while the
 * dialog is active and skips its own input then.
 */

typedef struct {
    Dialog dialog;
    int focus; /* 0 cancel, 1 quit */
    HitTap tap;
    Press press[2];
    HitRect rects[2];
} QuitDialog;

void quit_dialog_init(QuitDialog *q);
void quit_dialog_open(QuitDialog *q);
bool quit_dialog_active(const QuitDialog *q);

/* `keys` are the keys after the focus model. Calls scenes_request_quit on confirmation. */
void quit_dialog_update(QuitDialog *q, const InputFrame *in, unsigned keys);

/* Scrim and dialog over the bottom screen's content. */
void quit_dialog_draw(QuitDialog *q, bool focus_visible);

/* Dims the top screen while the dialog is open. */
void quit_dialog_draw_top(const QuitDialog *q);

#endif
