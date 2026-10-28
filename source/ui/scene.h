#ifndef PSS_UI_SCENE_H
#define PSS_UI_SCENE_H

#include <stdbool.h>

#include "ui/input.h"
#include "ui/stereo_view.h"

/*
 * Scenes and the scene stack. A scene is one full screen pair: what
 * the top screen shows and what the bottom screen offers. The host draws the
 * shared backdrop, then the scenes, and cross-fades them on a change.
 *
 * A scene is a struct that starts with a Scene and carries its own state;
 * scenes are static objects, because the one that was just left is still
 * drawn during the transition.
 */

typedef struct Scene Scene;

struct Scene {
    const char *name; /* for debugging */
    /* Became the active scene, by push or because the one above it was popped. */
    void (*on_enter)(Scene *self);
    /* Is covered by another scene or removed. */
    void (*on_leave)(Scene *self);
    /* Once per frame while active, with this frame's input. */
    void (*update)(Scene *self, const InputFrame *in);
    /* The content of one eye of the top screen; the backdrop is already drawn. */
    void (*draw_top)(Scene *self, const StereoView *view);
    /* The content of the bottom screen; the backdrop is already drawn. */
    void (*draw_bottom)(Scene *self);
    /* Optional: true while START must not open the settings (a purchase or restore is running). */
    bool (*blocks_start)(const Scene *self);
};

/* Starts with `root` as the only scene, shown without a transition. */
void scene_host_init(Scene *root);

/* Changes. Safe to call from update(). A held key does not repeat on the new scene. */
bool scene_push(Scene *scene);
bool scene_pop(void);
bool scene_replace(Scene *scene);

Scene *scene_active(void);

/* Per frame: update the active scene, then advance the transition. */
void scene_host_update(const InputFrame *in);

/* Backdrop and scenes, for one eye of the top screen / for the bottom screen. */
void scene_host_draw_top(const StereoView *view);
void scene_host_draw_bottom(void);

#endif
