#ifndef PSS_UI_SMOKE_SMOKE_H
#define PSS_UI_SMOKE_SMOKE_H

#include "platform/audio.h"
#include "ui/scene.h"

/*
 * Smoke test on the console: two test scenes that exercise the
 * scene stack, the transition, the stereo layers and every widget in all of
 * its states. Replaced by the real scenes in M4.
 */

/* The first scene. `audio` is the result of audio_init(), for the sound test. */
Scene *smoke_start(AudioResult audio);

#endif
