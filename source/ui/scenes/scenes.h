#ifndef PSS_UI_SCENES_SCENES_H
#define PSS_UI_SCENES_SCENES_H

#include <stdbool.h>

#include "app/model.h"
#include "app/route.h"
#include "platform/audio.h"
#include "ui/input.h"
#include "ui/scene.h"

/*
 * The screens of the app and what they share: the
 * model, the quit request and the way from one screen to the next. Scenes
 * reach the game data only through the model.
 */

/* Before scene_host_init. `audio` is the result of audio_init(). */
void scenes_init(AudioResult audio);

/* S0: shows the start sign while the model loads, then goes on by route. */
Scene *scenes_start(void);

/* Closes the model and the archives. */
void scenes_shutdown(void);

/* The model once S0 has loaded it. */
Model *scenes_model(void);

/* A screen asks to leave the app; main ends the loop. */
void scenes_request_quit(void);
bool scenes_quit_requested(void);

/* Goes to the screen of a route without keeping the current one. */
void scenes_go(Route route);

/*
 * START opens S8 on every screen except while a purchase or restore runs and
 * during the start; START in S8 closes it. Call once per frame before the
 * scene host updates.
 */
void scenes_handle_start(const InputFrame *in);

/* Sets the language from a setting of settings.ini and the console. */
void scenes_apply_language(SettingsLanguage setting);

/* S2, S3: `focus_visible` shows the focus ring from the start (opened with a key). */
Scene *scene_gallery(void);
Scene *scene_picker(unsigned panel_id, bool focus_visible);

/*
 * S6 after a purchase that was written. The gallery is below it on the stack:
 * "Weiter" goes to S3 again, or back to the gallery if the puzzle is complete now.
 * `backup` is the name of the backup made before the purchase.
 */
Scene *scene_success(unsigned panel_id, uint64_t pieces, unsigned cost, unsigned coins_before,
                     const char *backup);

/* S8: sound and language switches and the info. B or START return. */
Scene *scene_settings(void);

/* S7: the backups; A restores one after asking. B returns to where it was opened. */
Scene *scene_backups(void);

/* Developer view of all widgets (the M3 smoke test), opened with SELECT in the gallery. */
void scenes_push_smoke(void);

/* S1, errors: their scene objects, each reset by on_enter. */
Scene *scene_onboarding(void);
Scene *scene_notice(Route route);

/*
 * Developer view of all error screens, opened with the chip "Fehler" in the smoke test. The
 * notice is shown as a preview: its buttons only go back. `journal_readable` picks the variant of
 * E-05.
 */
Scene *scene_notice_preview(Route route, bool journal_readable);
Scene *scene_error_preview(void);

#endif
