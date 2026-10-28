#ifndef PSS_CORE_SCENE_STACK_H
#define PSS_CORE_SCENE_STACK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/tween.h"

/*
 * Stack of scenes with the screen-change transition .
 * section 8: the new scene slides in sideways and fades in over the old one.
 * Items are opaque pointers; the stack never owns or frees them, and an item
 * that was just removed must stay valid until the transition has ended.
 */

#define SCENE_STACK_MAX 8

typedef enum {
    SCENE_NAV_NONE,
    SCENE_NAV_PUSH,
    SCENE_NAV_POP,
    SCENE_NAV_REPLACE,
} SceneNav;

typedef struct {
    void *items[SCENE_STACK_MAX];
    int count;
    void *outgoing; /* scene leaving during a transition, else NULL */
    SceneNav nav;
    Tween progress;
    uint16_t frames;
    Easing curve;
} SceneStack;

/* What to draw this frame. Without a transition `outgoing` is NULL, offsets are 0, alpha_in 255. */
typedef struct {
    void *outgoing;
    void *incoming;
    int dx_out;
    int dx_in;
    uint8_t alpha_in;  /* draw the incoming scene over the outgoing one at this opacity */
    uint8_t alpha_out; /* the outgoing scene's opacity: gone by mid-transition, so it never shows
                          through the incoming one */
} SceneBlend;

void scene_stack_init(SceneStack *s, uint16_t frames, Easing curve);

/*
 * The first scene on an empty stack appears without a transition. Changing
 * scenes while a transition runs ends that one at once, so the stack never
 * shows more than two scenes. Each returns false and changes nothing if it
 * cannot be done: the stack is full or the item is NULL (push, replace), only
 * one scene is left (pop), or the stack is empty (replace).
 */
bool scene_stack_push(SceneStack *s, void *item);
bool scene_stack_pop(SceneStack *s);
bool scene_stack_replace(SceneStack *s, void *item);

/* The active scene, or NULL if empty. */
void *scene_stack_top(const SceneStack *s);

bool scene_stack_transitioning(const SceneStack *s);

/* Advance the transition by one frame; call once per frame. */
void scene_stack_step(SceneStack *s);

/* shift_px: how far the bottom screen slides. */
SceneBlend scene_stack_blend(const SceneStack *s, int shift_px);

#endif
