#ifndef PSS_PLATFORM_AUDIO_H
#define PSS_PLATFORM_AUDIO_H

#include <stdbool.h>

#include "core/sfx.h"

/*
 * UI sound effects over ndsp. Sounds are
 * loaded once from romfs:/sfx/ into linear memory; romfs must be mounted.
 *
 * ndsp needs the DSP firmware (sdmc:/3ds/dspfirm.cdc). Without it the app
 * stays silent but works normally; every call below is then a no-op.
 */

typedef enum {
    AUDIO_OK = 0,
    AUDIO_ERR_DSP,  /* ndspInit failed, usually no dspfirm.cdc */
    AUDIO_ERR_FILE, /* at least one sound missing or unreadable; the others play */
    AUDIO_ERR_MEM,  /* linear memory exhausted */
} AudioResult;

AudioResult audio_init(void);
void audio_exit(void);

/* Plays an effect. Up to AUDIO_VOICES sounds overlap; the oldest is cut. */
void audio_play(Sfx id);

/* Sound switch from the settings. Disabling also stops what is playing. */
void audio_set_enabled(bool enabled);
bool audio_is_enabled(void);

/* False if ndsp could not start. */
bool audio_is_available(void);

#endif
