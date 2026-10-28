#ifndef PSS_UI_SCENES_CONFIRM_DIALOG_H
#define PSS_UI_SCENES_CONFIRM_DIALOG_H

#include <stdbool.h>

#include "core/hit.h"
#include "core/press.h"
#include "core/purchase_flow.h"
#include "core/shop.h"
#include "ui/input.h"
#include "ui/widgets/dialog.h"

/*
 * S4 "Kauf bestätigen" and S5 "Kauf läuft":
 * the dialog over the piece picker. It asks first; after A the buttons give
 * way to a progress bar and "Bitte nicht ausschalten" while the scene runs
 * the purchase. The picker owns the purchase call itself.
 */

typedef enum {
    CONFIRM_NONE,
    CONFIRM_CANCELLED, /* the dialog closed, back to S3 */
    CONFIRM_RUN,       /* S5 began: run the purchase now (once) */
} ConfirmEvent;

typedef struct {
    Dialog dialog;
    PurchaseFlow flow;
    PurchasePlan plan;
    int focus; /* 0 cancel, 1 buy */
    HitTap tap;
    Press press[2];
    HitRect rects[2];
} ConfirmDialog;

void confirm_init(ConfirmDialog *c);

/* Opens S4 for a plan the model accepted. */
void confirm_open(ConfirmDialog *c, const PurchasePlan *plan);

/* Open, whether asking or buying. */
bool confirm_active(const ConfirmDialog *c);

/* True while the purchase runs (S5): nothing may be cancelled or left. */
bool confirm_buying(const ConfirmDialog *c);

/*
 * Once per frame while active. `keys` are the keys after the focus model;
 * touches are tracked here.
 */
ConfirmEvent confirm_update(ConfirmDialog *c, const InputFrame *in, u32 keys);

/* The scene ran the purchase. */
void confirm_ran(ConfirmDialog *c);

/* The bar was shown long enough after the purchase: the result may be shown. */
bool confirm_finished(const ConfirmDialog *c);

/* Dims the top screen with the dialog's scrim; call after the top screen's content. */
void confirm_draw_top(const ConfirmDialog *c);

/* Scrim and dialog over the bottom screen's content. */
void confirm_draw(ConfirmDialog *c, bool focus_visible);

#endif
