#include "ui/scene.h"

#include "core/scene_stack.h"
#include "ui/anim.h"
#include "ui/backdrop.h"
#include "ui/draw.h"

static SceneStack s_stack;

static void enter(Scene *s)
{
    if (s && s->on_enter)
        s->on_enter(s);
}

static void leave(Scene *s)
{
    if (s && s->on_leave)
        s->on_leave(s);
}

void scene_host_init(Scene *root)
{
    scene_stack_init(&s_stack, ANIM_SCREEN, EASE_OUT_CUBIC);
    scene_stack_push(&s_stack, root);
    enter(root);
}

Scene *scene_active(void)
{
    return scene_stack_top(&s_stack);
}

bool scene_push(Scene *scene)
{
    Scene *old = scene_active();
    if (!scene_stack_push(&s_stack, scene))
        return false;
    input_reset_repeat();
    leave(old);
    enter(scene);
    return true;
}

bool scene_pop(void)
{
    Scene *old = scene_active();
    if (!scene_stack_pop(&s_stack))
        return false;
    input_reset_repeat();
    leave(old);
    enter(scene_active());
    return true;
}

bool scene_replace(Scene *scene)
{
    Scene *old = scene_active();
    if (!scene_stack_replace(&s_stack, scene))
        return false;
    input_reset_repeat();
    leave(old);
    enter(scene);
    return true;
}

void scene_host_update(const InputFrame *in)
{
    Scene *s = scene_active();
    if (s && s->update)
        s->update(s, in);
    scene_stack_step(&s_stack);
}

void scene_host_draw_top(const StereoView *view)
{
    backdrop_top_draw(view);

    /* The top screen only fades; the incoming scene lies over the outgoing one. */
    SceneBlend b = scene_stack_blend(&s_stack, 0);
    Scene *out = b.outgoing, *in = b.incoming;
    if (out && out->draw_top) {
        draw_opacity_push(b.alpha_out);
        out->draw_top(out, view);
        draw_opacity_pop();
    }
    if (in && in->draw_top) {
        draw_opacity_push(b.alpha_in);
        in->draw_top(in, view);
        draw_opacity_pop();
    }
}

void scene_host_draw_bottom(void)
{
    backdrop_bottom_draw();

    SceneBlend b = scene_stack_blend(&s_stack, ANIM_SCREEN_SHIFT_PX);
    Scene *out = b.outgoing, *in = b.incoming;
    if (out && out->draw_bottom) {
        draw_translate_push(b.dx_out, 0);
        draw_opacity_push(b.alpha_out);
        out->draw_bottom(out);
        draw_opacity_pop();
        draw_translate_pop();
    }
    if (in && in->draw_bottom) {
        draw_translate_push(b.dx_in, 0);
        draw_opacity_push(b.alpha_in);
        in->draw_bottom(in);
        draw_opacity_pop();
        draw_translate_pop();
    }
}
