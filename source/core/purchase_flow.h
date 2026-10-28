#ifndef PSS_CORE_PURCHASE_FLOW_H
#define PSS_CORE_PURCHASE_FLOW_H

#include <stdbool.h>

/*
 * Timing of S5 "Kauf läuft". The purchase
 * blocks for a moment, so the dialog first shows the progress bar for a few
 * frames, the scene then runs the purchase, and the bar stays long enough to
 * be seen before the result screen follows. Frames at 60 fps; the scene
 * owns the purchase call, this only keeps the sequence.
 */

#define PF_WORK_DELAY 4  /* frames of "working" before the purchase runs */
#define PF_MIN_FRAMES 24 /* the bar is shown at least this long in total */

typedef enum {
    PF_IDLE,    /* S4 or no purchase */
    PF_RUNNING, /* S5 */
} PurchasePhase;

typedef struct {
    PurchasePhase phase;
    unsigned frame;
    bool ran; /* the purchase was run and its result reported */
} PurchaseFlow;

void purchase_flow_init(PurchaseFlow *f);

/* A in S4. Only from PF_IDLE; returns whether it started. */
bool purchase_flow_start(PurchaseFlow *f);

/* True once, on the frame the scene should call the purchase. */
bool purchase_flow_work_due(const PurchaseFlow *f);

/* The scene ran the purchase. */
void purchase_flow_ran(PurchaseFlow *f);

/* Advance one frame. */
void purchase_flow_step(PurchaseFlow *f);

/* The purchase ran and the bar was shown long enough: the result screen may follow. */
bool purchase_flow_done(const PurchaseFlow *f);

/* The progress bar's position 0..1 for a bar that fills while it waits and holds at the end. */
float purchase_flow_progress(const PurchaseFlow *f);

#endif
