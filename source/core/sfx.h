#ifndef PSS_CORE_SFX_H
#define PSS_CORE_SFX_H

/* Sound effects stored as romfs/sfx/<name>.wav. */

typedef enum {
    SFX_MOVE = 0, /* focus moves */
    SFX_SELECT,   /* piece selected */
    SFX_DESELECT, /* piece deselected */
    SFX_CONFIRM,  /* A on a button */
    SFX_BACK,     /* B */
    SFX_PAGE,     /* L or R */
    SFX_BUY,      /* purchase done */
    SFX_COIN,     /* every 5 counted coins */
    SFX_COMPLETE, /* puzzle finished */
    SFX_ERROR,    /* blocked action */
    SFX_COUNT,
} Sfx;

/* Upper bound for every effect. */
#define SFX_MAX_MS 300

/* File name without directory, e.g. "move.wav"; NULL for an invalid id. */
const char *sfx_file_name(Sfx id);

#endif
