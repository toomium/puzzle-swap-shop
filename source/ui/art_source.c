#include "ui/art_source.h"

#include <stdlib.h>
#include <string.h>

#include "core/cgfx.h"
#include "core/pica_tex.h"

#define ART_SLOTS    8
#define ART_FILE_MAX (1024u * 1024)
#define ART_SIDE_MAX 512u /* one picture is 512 x 256; bigger ones are not ours */

typedef enum {
    SLOT_EMPTY = 0,
    SLOT_READY,
    SLOT_FAILED,
} SlotState;

typedef struct {
    SlotState state;
    unsigned panel;
    unsigned used; /* frame of the last use */
    C3D_Tex tex;
    Tex3DS_SubTexture sub;
    C2D_Image image;
} Slot;

static Slot s_slot[ART_SLOTS];
static const Model *s_model;
static unsigned s_frame = 1;
static bool s_loaded_this_frame;

static bool pow2(unsigned v)
{
    return v != 0 && (v & (v - 1)) == 0;
}

static void slot_release(Slot *s)
{
    if (s->state == SLOT_READY)
        C3D_TexDelete(&s->tex);
    memset(s, 0, sizeof(*s));
}

void art_source_init(const Model *model)
{
    art_source_fini();
    s_model = model;
    s_frame = 1;
    s_loaded_this_frame = false;
}

void art_source_fini(void)
{
    for (int i = 0; i < ART_SLOTS; i++)
        slot_release(&s_slot[i]);
    s_model = NULL;
}

void art_source_frame(void)
{
    s_frame++;
    s_loaded_this_frame = false;
}

/* Reads, decodes and uploads one picture into `s`. */
static bool slot_load(Slot *s, unsigned panel)
{
    bool ok = false;
    uint8_t *file = malloc(ART_FILE_MAX);
    uint8_t *rgba = NULL;
    size_t len = 0;
    CgfxTexture t;
    if (file == NULL || model_panel_image(s_model, panel, file, ART_FILE_MAX, &len) != STORE_OK ||
        cgfx_find_texture(file, len, &t) != CGFX_OK || !pow2(t.width) || !pow2(t.height) ||
        t.width > ART_SIDE_MAX || t.height > ART_SIDE_MAX)
        goto done;
    rgba = malloc((size_t)t.width * t.height * 4);
    if (rgba == NULL || cgfx_decode(file, len, &t, rgba) != CGFX_OK)
        goto done;
    if (!C3D_TexInit(&s->tex, (u16)t.width, (u16)t.height, GPU_RGBA8))
        goto done;
    if (!pica_tex_encode_rgba8(t.width, t.height, rgba, s->tex.data)) {
        C3D_TexDelete(&s->tex);
        goto done;
    }
    GSPGPU_FlushDataCache(s->tex.data, (u32)t.width * t.height * 4);
    C3D_TexSetFilter(&s->tex, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&s->tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    /* Rows are stored top first and v counts from the bottom. */
    CgfxRect r = cgfx_picture_rect(&t);
    float tw = (float)t.width, th = (float)t.height;
    s->sub = (Tex3DS_SubTexture){(u16)r.w,
                                 (u16)r.h,
                                 (float)r.x / tw,
                                 1.0f - (float)r.y / th,
                                 (float)(r.x + r.w) / tw,
                                 1.0f - (float)(r.y + r.h) / th};
    s->image = (C2D_Image){&s->tex, &s->sub};
    ok = true;
done:
    free(file);
    free(rgba);
    return ok;
}

/* A free slot, else the least recently used one that was not drawn this frame. */
static Slot *slot_for_new(void)
{
    Slot *best = NULL;
    for (int i = 0; i < ART_SLOTS; i++) {
        Slot *s = &s_slot[i];
        if (s->state == SLOT_EMPTY)
            return s;
        if (s->used == s_frame)
            continue; /* queued draws of this frame may still point at its texture */
        if (best == NULL || s->used < best->used)
            best = s;
    }
    return best;
}

const C2D_Image *art_source_get(unsigned panel)
{
    if (s_model == NULL || panel >= PANEL_COUNT)
        return NULL;
    for (int i = 0; i < ART_SLOTS; i++) {
        Slot *s = &s_slot[i];
        if (s->state != SLOT_EMPTY && s->panel == panel) {
            s->used = s_frame;
            return s->state == SLOT_READY ? &s->image : NULL;
        }
    }
    if (!model_panel_has_image(s_model, panel) || s_loaded_this_frame)
        return NULL;
    Slot *s = slot_for_new();
    if (s == NULL)
        return NULL;
    slot_release(s);
    s_loaded_this_frame = true;
    s->panel = panel;
    s->used = s_frame;
    s->state = slot_load(s, panel) ? SLOT_READY : SLOT_FAILED;
    return s->state == SLOT_READY ? &s->image : NULL;
}
