#include "ui/scenes/scenes.h"

#include "app/boot.h"
#include "core/strings.h"
#include "core/version.h"
#include "platform/language.h"
#include "ui/anim.h"
#include "ui/art_source.h"
#include "ui/draw.h"
#include "ui/smoke/smoke.h"
#include "ui/stereo_view.h"
#include "ui/theme.h"
#include "ui/widgets/sign.h"

static AppSystem s_sys;
static Model s_model;
static AudioResult s_audio;
static bool s_quit;

void scenes_init(AudioResult audio)
{
    s_audio = audio;
    s_quit = false;
    /* The start sign needs a language before settings.ini is read: follow the console. */
    scenes_apply_language(SETTINGS_LANG_AUTO);
}

Model *scenes_model(void)
{
    return &s_model;
}

void scenes_shutdown(void)
{
    art_source_fini();
    app_shutdown(&s_sys, &s_model);
}

void scenes_request_quit(void)
{
    s_quit = true;
}

bool scenes_quit_requested(void)
{
    return s_quit;
}

void scenes_apply_language(SettingsLanguage setting)
{
    Language lang = LANG_DE;
    /* If the console cannot tell, German stays, the first language of the app. */
    if (R_SUCCEEDED(language_resolve((LanguageSetting)setting, &lang)))
        strings_set_language(lang);
}

void scenes_handle_start(const InputFrame *in)
{
    if (!(in->down & KEY_START))
        return;
    Scene *active = scene_active();
    if (active == scene_settings()) {
        audio_play(SFX_BACK);
        scene_pop();
        return;
    }
    if (active->blocks_start && active->blocks_start(active))
        return;
    audio_play(SFX_CONFIRM);
    scene_push(scene_settings());
}

void scenes_push_smoke(void)
{
    scene_push(smoke_start(s_audio));
}

void scenes_go(Route route)
{
    switch (route) {
    case ROUTE_GALLERY:
        scene_replace(scene_gallery());
        break;
    case ROUTE_ONBOARDING:
        scene_replace(scene_onboarding());
        break;
    default:
        scene_replace(scene_notice(route));
        break;
    }
}

/* --- S0 Start ---------------------------------------------------------------- */

typedef struct {
    Scene base;
    unsigned frame;
    bool booted;
    Route route;
} BootScene;

static void boot_on_enter(Scene *self)
{
    BootScene *s = (BootScene *)self;
    s->frame = 0;
    s->booted = false;
}

/* The load blocks for a moment, so the sign is on screen before it starts. */
static void boot_load(BootScene *s)
{
    BootResult br = app_boot(&s_sys, &s_model);
    if (br == BOOT_ERR_NOT_INSTALLED) {
        s->route = ROUTE_E02;
    } else if (br != BOOT_OK) {
        s->route = ROUTE_E01;
    } else {
        scenes_apply_language(s_model.settings.language);
        art_source_init(&s_model);
        s->route = route_after_load(s_model.status);
    }
    s->booted = true;
}

static void boot_update(Scene *self, const InputFrame *in)
{
    BootScene *s = (BootScene *)self;
    (void)in;
    s->frame++;
    if (!s->booted && s->frame >= 2)
        boot_load(s);
    if (s->booted && s->frame >= ANIM_BOOT_FADE)
        scenes_go(s->route);
}

static void boot_draw_top(Scene *self, const StereoView *view)
{
    BootScene *s = (BootScene *)self;
    unsigned fade = s->frame >= ANIM_BOOT_FADE ? ANIM_BOOT_FADE : s->frame;
    u8 alpha = (u8)(fade * THEME_ALPHA_OPAQUE / ANIM_BOOT_FADE);

    stereo_layer_begin(view, STEREO_LAYER_FRONT);
    sign_draw(str(STR_APP_NAME), str(STR_APP_TAGLINE), alpha);
    stereo_layer_end();

    stereo_layer_begin(view, STEREO_LAYER_BOARD);
    draw_opacity_push(alpha);
    sign_version_draw(version_string());
    draw_opacity_pop();
    stereo_layer_end();
}

static void boot_draw_bottom(Scene *self)
{
    (void)self;
}

static bool boot_blocks_start(const Scene *self)
{
    (void)self;
    return true; /* the model is not loaded yet */
}

static BootScene s_boot = {.base = {"S0 Start", boot_on_enter, NULL, boot_update, boot_draw_top,
                                    boot_draw_bottom, boot_blocks_start}};

Scene *scenes_start(void)
{
    return &s_boot.base;
}
