#include "core/scene_stack.h"

#include <stddef.h>

void scene_stack_init(SceneStack *s, uint16_t frames, Easing curve)
{
    s->count = 0;
    s->outgoing = NULL;
    s->nav = SCENE_NAV_NONE;
    s->frames = frames;
    s->curve = curve;
    tween_start(&s->progress, 0.0f, 1.0f, frames, 0, curve);
    tween_finish(&s->progress);
}

static void begin(SceneStack *s, SceneNav nav, void *outgoing)
{
    s->nav = nav;
    s->outgoing = outgoing;
    tween_start(&s->progress, 0.0f, 1.0f, s->frames, 0, s->curve);
    if (outgoing == NULL || s->frames == 0) {
        tween_finish(&s->progress);
        s->outgoing = NULL;
        s->nav = SCENE_NAV_NONE;
    }
}

static void settle(SceneStack *s)
{
    tween_finish(&s->progress);
    s->outgoing = NULL;
    s->nav = SCENE_NAV_NONE;
}

bool scene_stack_push(SceneStack *s, void *item)
{
    if (item == NULL || s->count >= SCENE_STACK_MAX)
        return false;
    settle(s);
    void *outgoing = scene_stack_top(s);
    s->items[s->count++] = item;
    begin(s, SCENE_NAV_PUSH, outgoing);
    return true;
}

bool scene_stack_pop(SceneStack *s)
{
    if (s->count <= 1)
        return false;
    settle(s);
    void *outgoing = scene_stack_top(s);
    s->count--;
    begin(s, SCENE_NAV_POP, outgoing);
    return true;
}

bool scene_stack_replace(SceneStack *s, void *item)
{
    if (item == NULL || s->count == 0)
        return false;
    settle(s);
    void *outgoing = scene_stack_top(s);
    s->items[s->count - 1] = item;
    begin(s, SCENE_NAV_REPLACE, outgoing);
    return true;
}

void *scene_stack_top(const SceneStack *s)
{
    return s->count > 0 ? s->items[s->count - 1] : NULL;
}

bool scene_stack_transitioning(const SceneStack *s)
{
    return s->outgoing != NULL;
}

void scene_stack_step(SceneStack *s)
{
    if (s->outgoing == NULL)
        return;
    if (!tween_step(&s->progress))
        settle(s);
}

static int round_px(float v)
{
    return v >= 0.0f ? (int)(v + 0.5f) : -(int)(-v + 0.5f);
}

SceneBlend scene_stack_blend(const SceneStack *s, int shift_px)
{
    SceneBlend b = {NULL, scene_stack_top(s), 0, 0, 255, 0};
    if (s->outgoing == NULL)
        return b;

    float p = tween_value(&s->progress);
    /* New scenes come in from the right, a scene revealed by pop from the left. */
    int dir = s->nav == SCENE_NAV_POP ? -1 : 1;
    b.outgoing = s->outgoing;
    b.dx_in = round_px((float)dir * (float)shift_px * (1.0f - p));
    b.dx_out = round_px((float)-dir * (float)shift_px * p);
    /* The old scene fades out in the first half while the new one fades in twice as fast,
     * so the two are never both nearly opaque and the old one cannot show through. */
    float in = p * 2.0f * 255.0f + 0.5f;
    float out = (1.0f - p * 2.0f) * 255.0f + 0.5f;
    b.alpha_in = in >= 255.0f ? 255 : (uint8_t)in;
    b.alpha_out = out <= 0.0f ? 0 : out >= 255.0f ? 255 : (uint8_t)out;
    return b;
}
