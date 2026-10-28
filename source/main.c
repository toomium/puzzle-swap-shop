/*
 * Puzzle Swap Shop
 *
 * Starts at S0 (source/ui/scenes), which loads the model and goes on to S1,
 * an error screen or the gallery. START opens the settings (S8) wherever
 * that is safe.
 */
#include <3ds.h>
#include <citro2d.h>

#include "platform/audio.h"
#include "ui/anim.h"
#include "ui/draw.h"
#include "ui/input.h"
#include "ui/scene.h"
#include "ui/scenes/scenes.h"
#include "ui/stereo_view.h"
#include "ui/theme.h"

static void draw_top_eye(C3D_RenderTarget *target, StereoEye eye, float slider)
{
    C2D_TargetClear(target, theme_color(THEME_SKY_LIGHT, THEME_ALPHA_OPAQUE));
    draw_screen_begin(target, THEME_TOP_W);
    StereoView view = {eye, slider};
    scene_host_draw_top(&view);
}

int main(void)
{
    gfxInitDefault();
    gfxSet3D(true);
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS * 2);
    C2D_Prepare();
    theme_init();
    AudioResult audio = audio_init();

    C3D_RenderTarget *top_left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *top_right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    Result rc = draw_init();
    if (R_SUCCEEDED(rc)) {
        input_init();
        scenes_init(audio);
        scene_host_init(scenes_start());
    }

    while (aptMainLoop()) {
        InputFrame in = {0};
        input_update(&in);
        if (scenes_quit_requested())
            break;

        if (R_SUCCEEDED(rc)) {
            scenes_handle_start(&in);
            scene_host_update(&in);
            anim_tick();
        }

        /* One image when the slider is at 0, two otherwise. */
        float slider = stereo_slider();
        bool stereo = slider > 0.0f;
        gfxSet3D(stereo);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if (R_FAILED(rc)) {
            /* No atlas, no UI: a plain warning color is all that can be shown. */
            /* A target is only shown if a scene was begun on it this frame. */
            C2D_TargetClear(top_left, theme_color(THEME_DANGER, THEME_ALPHA_OPAQUE));
            C2D_SceneBegin(top_left);
            C2D_TargetClear(bottom, theme_color(THEME_DANGER, THEME_ALPHA_OPAQUE));
            C2D_SceneBegin(bottom);
        } else {
            draw_frame_begin();
            draw_top_eye(top_left, STEREO_EYE_LEFT, slider);
            if (stereo)
                draw_top_eye(top_right, STEREO_EYE_RIGHT, slider);
            C2D_TargetClear(bottom, theme_color(THEME_STRIPE_A, THEME_ALPHA_OPAQUE));
            draw_screen_begin(bottom, THEME_BOTTOM_W);
            scene_host_draw_bottom();
        }
        C3D_FrameEnd(0);
    }

    if (R_SUCCEEDED(rc))
        scenes_shutdown();
    audio_exit();
    draw_fini();
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
