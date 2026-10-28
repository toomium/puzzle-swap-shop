#include "app/pnel_load.h"

#include <string.h>

#define PNEL_MAX_CANDIDATES 64
#define PNEL_LANG_EN        1u

typedef struct {
    char path[PNEL_MAX_CANDIDATES][PNEL_PATH_MAX];
    uint64_t size[PNEL_MAX_CANDIDATES];
    unsigned count;
} Candidates;

static bool collect(const char *name, bool is_dir, uint64_t size, void *arg)
{
    Candidates *c = arg;
    if (!is_dir && size >= PNEL_HEADER_SIZE + PNEL_ENTRY_SIZE && strlen(name) < PNEL_PATH_MAX &&
        c->count < PNEL_MAX_CANDIDATES) {
        strcpy(c->path[c->count], name);
        c->size[c->count] = size;
        c->count++;
    }
    return true;
}

/* Reads the table of the PNEL data that starts at `base`; false if it is not valid there. */
static bool read_table(const Storage *s, const char *path, uint64_t base, uint64_t size,
                       unsigned lang, unsigned file, PnelSet *set)
{
    uint8_t head[PNEL_HEADER_SIZE];
    size_t got = 0;
    PnelHeader h;
    uint64_t rest = size - base;
    if (rest > UINT32_MAX || rest < PNEL_HEADER_SIZE ||
        s->boss_read(s->ctx, path, base, head, sizeof(head), &got) != STORE_OK ||
        got != sizeof(head) || pnel_read_head(head, got, (size_t)rest, &h) != PNEL_OK)
        return false;

    for (uint32_t i = 0; i < h.count; i++) {
        uint8_t e[PNEL_ENTRY_SIZE];
        PnelEntry entry;
        char name[PANEL_NAME_MAX], name_en[PANEL_NAME_MAX];
        uint64_t at = base + PNEL_HEADER_SIZE + (uint64_t)i * PNEL_ENTRY_SIZE;
        if (s->boss_read(s->ctx, path, at, e, sizeof(e), &got) != STORE_OK || got != sizeof(e) ||
            pnel_read_entry_at(e, (size_t)rest, &entry) != PNEL_OK ||
            pnel_entry_name_at(e, lang, name, sizeof(name)) != PNEL_OK ||
            pnel_entry_name_at(e, PNEL_LANG_EN, name_en, sizeof(name_en)) != PNEL_OK) {
            set->index.ignored++;
            continue;
        }
        /* image_offset stays relative to base; pnel_read_image adds it. */
        pnel_index_add(&set->index, file, &entry, name, name_en);
    }
    set->base[file] = base;
    return true;
}

#define SCAN_CHUNK 65536u
#define SCAN_MAX   (96ull * 1024 * 1024)

/* Offset of the first valid PNEL header in the file, or UINT64_MAX. */
static uint64_t find_magic(const Storage *s, const char *path, uint64_t size)
{
    static uint8_t buf[SCAN_CHUNK + 4];
    uint64_t pos = 0;
    if (size > SCAN_MAX)
        return UINT64_MAX;
    while (pos + PNEL_HEADER_SIZE <= size) {
        size_t got = 0;
        if (s->boss_read(s->ctx, path, pos, buf, SCAN_CHUNK + 4, &got) != STORE_OK || got < 4)
            return UINT64_MAX;
        size_t last = got >= SCAN_CHUNK + 4 ? SCAN_CHUNK : got - 3;
        for (size_t i = 0; i < last; i++) {
            if (buf[i] == 'P' && buf[i + 1] == 'N' && buf[i + 2] == 'E' && buf[i + 3] == 'L') {
                uint8_t head[PNEL_HEADER_SIZE];
                size_t hgot = 0;
                PnelHeader h;
                uint64_t rest = size - pos - i;
                if (s->boss_read(s->ctx, path, pos + i, head, sizeof(head), &hgot) == STORE_OK &&
                    hgot == sizeof(head) &&
                    pnel_read_head(head, hgot, rest > UINT32_MAX ? UINT32_MAX : (size_t)rest, &h) ==
                        PNEL_OK)
                    return pos + i;
            }
        }
        if (got < SCAN_CHUNK + 4)
            break;
        pos += SCAN_CHUNK;
    }
    return UINT64_MAX;
}

/* Reads one file's table, searching for the magic if it is not at the start. */
static bool read_file(const Storage *s, const char *path, uint64_t size, unsigned lang,
                      unsigned file, PnelSet *set)
{
    if (read_table(s, path, 0, size, lang, file, set))
        return true;
    uint64_t at = find_magic(s, path, size);
    set->scanned++;
    return at != UINT64_MAX && at != 0 && read_table(s, path, at, size, lang, file, set);
}

PnelLoadResult pnel_load(const Storage *s, unsigned system_language, PnelSet *out)
{
    memset(out, 0, sizeof(*out));
    pnel_index_init(&out->index);
    if (s->boss_list == NULL || s->boss_read == NULL)
        return PNEL_LOAD_NO_ARCHIVE;

    static Candidates cand; /* 6 KB, not for the stack */
    cand.count = 0;
    if (s->boss_list(s->ctx, collect, &cand) != STORE_OK)
        return PNEL_LOAD_NO_ARCHIVE;
    out->candidates = cand.count;

    for (unsigned i = 0; i < cand.count && out->files < PNEL_MAX_FILES; i++) {
        unsigned file = out->files;
        if (read_file(s, cand.path[i], cand.size[i], system_language, file, out))
            strcpy(out->path[out->files++], cand.path[i]);
    }
    return out->files > 0 ? PNEL_LOAD_OK : PNEL_LOAD_NO_FILE;
}

StoreResult pnel_read_image(const Storage *s, const PnelSet *set, unsigned panel, uint8_t *buf,
                            size_t cap, size_t *len)
{
    *len = 0;
    if (s->boss_read == NULL || panel >= PANEL_COUNT || !set->index.panel[panel].present)
        return STORE_ERR_NOT_FOUND;
    const PnelPanel *p = &set->index.panel[panel];
    if (p->file >= set->files)
        return STORE_ERR_NOT_FOUND;
    if (p->image_size > cap)
        return STORE_ERR_SIZE;
    StoreResult r = s->boss_read(s->ctx, set->path[p->file], set->base[p->file] + p->image_offset,
                                 buf, p->image_size, len);
    if (r == STORE_OK && *len != p->image_size)
        return STORE_ERR_IO;
    return r;
}
