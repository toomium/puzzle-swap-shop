#ifndef PSS_CORE_ONBOARDING_H
#define PSS_CORE_ONBOARDING_H

#include <stdbool.h>

/*
 * Timing of S1 "Erste Sicherung". The backup
 * itself blocks for a moment, so the screen first shows "working" for a few
 * frames, the scene then runs the backup and reports the result, and after
 * a success one dot per file turns green, one after the other. Frames at
 * 60 fps; the scene owns the backup call, this only keeps the sequence.
 */

#define ONB_FILES      2  /* meet.dat, gamecoin.dat */
#define ONB_WORK_DELAY 3  /* frames of "working" before the backup runs */
#define ONB_DOT_STEP   8  /* frames between two dots turning green */
#define ONB_HOLD       12 /* frames to look at the result before leaving */

typedef enum {
    ONB_WAIT,    /* waiting for A */
    ONB_WORKING, /* A was pressed, the backup is due after ONB_WORK_DELAY frames */
    ONB_REVEAL,  /* backup done, dots turn green */
    ONB_DONE,    /* the scene may move on */
    ONB_FAILED,  /* backup failed; A tries again */
} OnbPhase;

typedef struct {
    OnbPhase phase;
    unsigned frame; /* frames since the current phase began */
} Onboarding;

void onboarding_init(Onboarding *o);

/* A: only from ONB_WAIT or ONB_FAILED. Returns whether it started. */
bool onboarding_start(Onboarding *o);

/* True while the backup should be run now (ONB_WORKING, delay over). */
bool onboarding_work_due(const Onboarding *o);

/* Reports the backup result; only in ONB_WORKING. */
void onboarding_finish(Onboarding *o, bool ok);

/* Advance one frame. */
void onboarding_step(Onboarding *o);

/* Whether the dot of file `file` (0-based) is green. */
bool onboarding_dot_green(const Onboarding *o, unsigned file);

/* Nothing else is accepted while working or revealing (B must not leave in between). */
bool onboarding_busy(const Onboarding *o);

#endif
