#include "platform/backup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/crc32.h"

#define FILE_MEET     "meet.dat"
#define FILE_COIN     "gamecoin.dat"
#define FILE_MANIFEST "manifest.txt"
#define PATH_MAX_LEN  64

static void path_of(char out[PATH_MAX_LEN], const char *name, const char *file)
{
    if (file != NULL)
        snprintf(out, PATH_MAX_LEN, "%s/%s/%s", BACKUP_DIR, name, file);
    else
        snprintf(out, PATH_MAX_LEN, "%s/%s", BACKUP_DIR, name);
}

/* Removes the known files and the folder. Missing files are fine. */
static StoreResult remove_folder(const Storage *s, const char *name)
{
    static const char *const files[] = {FILE_MANIFEST, FILE_MEET, FILE_COIN};
    char path[PATH_MAX_LEN];
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        path_of(path, name, files[i]);
        StoreResult r = s->sd_remove(s->ctx, path);
        if (r != STORE_OK && r != STORE_ERR_NOT_FOUND)
            return r;
    }
    path_of(path, name, NULL);
    StoreResult r = s->sd_rmdir(s->ctx, path);
    return r == STORE_ERR_NOT_FOUND ? STORE_OK : r;
}

/* Writes a file and reads it back into scratch (cap >= len) to compare. */
static BackupResult write_verified(const Storage *s, const char *path, const uint8_t *data,
                                   size_t len, uint8_t *scratch)
{
    size_t got = 0;
    if (s->sd_write(s->ctx, path, data, len) != STORE_OK)
        return BACKUP_ERR_WRITE;
    if (s->sd_read(s->ctx, path, scratch, len, &got) != STORE_OK || got != len ||
        memcmp(scratch, data, len) != 0)
        return BACKUP_ERR_VERIFY;
    return BACKUP_OK;
}

typedef struct {
    char (*names)[BACKUP_NAME_MAX];
    unsigned count;
    unsigned cap;
    bool failed;
} NameList;

static bool collect(const char *name, bool is_dir, uint64_t size, void *arg)
{
    NameList *list = arg;
    (void)size;
    if (!is_dir || !backup_name_valid(name))
        return true;
    if (list->count == list->cap) {
        unsigned cap = list->cap ? list->cap * 2 : 32;
        void *grown = realloc(list->names, cap * sizeof(*list->names));
        if (grown == NULL) {
            list->failed = true;
            return false;
        }
        list->names = grown;
        list->cap = cap;
    }
    strcpy(list->names[list->count++], name);
    return true;
}

static int newest_first(const void *a, const void *b)
{
    return strcmp((const char *)b, (const char *)a);
}

/* Valid backup folder names, newest first. Caller frees list->names. */
static bool list_names(const Storage *s, NameList *list)
{
    memset(list, 0, sizeof(*list));
    StoreResult r = s->sd_list(s->ctx, BACKUP_DIR, collect, list);
    if (r == STORE_ERR_NOT_FOUND)
        return true;
    if (r != STORE_OK || list->failed) {
        free(list->names);
        list->names = NULL;
        return false;
    }
    if (list->count > 1)
        qsort(list->names, list->count, sizeof(*list->names), newest_first);
    return true;
}

bool backup_exists(const Storage *s, const char *name)
{
    NameList list;
    bool found = false;
    if (!backup_name_valid(name))
        return false;
    /* If the folder cannot be listed, treat the name as taken. */
    if (!list_names(s, &list))
        return true;
    for (unsigned i = 0; i < list.count && !found; i++)
        found = strcmp(list.names[i], name) == 0;
    free(list.names);
    return found;
}

BackupResult backup_choose_name(const Storage *s, const Timestamp *t, char out[BACKUP_NAME_MAX])
{
    for (unsigned suffix = 0; suffix <= BACKUP_MAX_SUFFIX; suffix++) {
        if (suffix == 1)
            continue;
        if (!backup_name_make(t, suffix, out))
            return BACKUP_ERR_NAME;
        if (!backup_exists(s, out))
            return BACKUP_OK;
    }
    return BACKUP_ERR_EXISTS;
}

BackupResult backup_create(const Storage *s, const char *name, const Manifest *meta,
                           const uint8_t *meet, size_t meet_len, const uint8_t *coin,
                           size_t coin_len, Manifest *written)
{
    char path[PATH_MAX_LEN];
    char text[MANIFEST_TEXT_MAX];
    size_t text_len;
    Manifest m = *meta;

    if (!backup_name_valid(name))
        return BACKUP_ERR_NAME;

    m.meet_size = (uint32_t)meet_len;
    m.meet_crc = crc32(meet, meet_len);
    m.coin_size = (uint32_t)coin_len;
    m.coin_crc = crc32(coin, coin_len);
    if (manifest_format(&m, text, sizeof(text), &text_len) != MANIFEST_OK)
        return BACKUP_ERR_WRITE;

    StoreResult sr = s->sd_mkdir(s->ctx, BACKUP_DIR);
    if (sr != STORE_OK && sr != STORE_ERR_EXISTS)
        return BACKUP_ERR_WRITE;
    path_of(path, name, NULL);
    sr = s->sd_mkdir(s->ctx, path);
    if (sr == STORE_ERR_EXISTS)
        return BACKUP_ERR_EXISTS;
    if (sr != STORE_OK)
        return BACKUP_ERR_WRITE;

    size_t scratch_len = meet_len > coin_len ? meet_len : coin_len;
    if (scratch_len < text_len)
        scratch_len = text_len;
    uint8_t *scratch = malloc(scratch_len ? scratch_len : 1);
    if (scratch == NULL) {
        remove_folder(s, name);
        return BACKUP_ERR_MEMORY;
    }

    BackupResult r;
    path_of(path, name, FILE_MEET);
    r = write_verified(s, path, meet, meet_len, scratch);
    if (r == BACKUP_OK) {
        path_of(path, name, FILE_COIN);
        r = write_verified(s, path, coin, coin_len, scratch);
    }
    if (r == BACKUP_OK) {
        /* Last: a manifest marks the backup as complete. */
        path_of(path, name, FILE_MANIFEST);
        r = write_verified(s, path, (const uint8_t *)text, text_len, scratch);
    }
    free(scratch);

    if (r != BACKUP_OK) {
        remove_folder(s, name);
        return r;
    }
    if (written != NULL)
        *written = m;
    return BACKUP_OK;
}

static BackupResult read_manifest(const Storage *s, const char *name, Manifest *m)
{
    char path[PATH_MAX_LEN];
    char text[MANIFEST_TEXT_MAX];
    size_t len;

    path_of(path, name, FILE_MANIFEST);
    StoreResult r = s->sd_read(s->ctx, path, (uint8_t *)text, sizeof(text), &len);
    if (r == STORE_ERR_NOT_FOUND)
        return BACKUP_ERR_NOT_FOUND;
    if (r != STORE_OK || manifest_parse(text, len, m) != MANIFEST_OK)
        return BACKUP_ERR_MANIFEST;
    return BACKUP_OK;
}

static BackupResult read_checked(const Storage *s, const char *name, const char *file, uint8_t *buf,
                                 size_t cap, size_t *len, uint32_t size, uint32_t crc)
{
    char path[PATH_MAX_LEN];
    path_of(path, name, file);
    StoreResult r = s->sd_read(s->ctx, path, buf, cap, len);
    if (r == STORE_ERR_NOT_FOUND || r == STORE_ERR_SIZE)
        return BACKUP_ERR_DAMAGED;
    if (r != STORE_OK)
        return BACKUP_ERR_IO;
    if (*len != size || crc32(buf, *len) != crc)
        return BACKUP_ERR_DAMAGED;
    return BACKUP_OK;
}

BackupResult backup_load(const Storage *s, const char *name, Manifest *m, uint8_t *meet,
                         size_t meet_cap, size_t *meet_len, uint8_t *coin, size_t coin_cap,
                         size_t *coin_len)
{
    if (!backup_name_valid(name))
        return BACKUP_ERR_NAME;
    BackupResult r = read_manifest(s, name, m);
    if (r != BACKUP_OK)
        return r;
    r = read_checked(s, name, FILE_MEET, meet, meet_cap, meet_len, m->meet_size, m->meet_crc);
    if (r != BACKUP_OK)
        return r;
    return read_checked(s, name, FILE_COIN, coin, coin_cap, coin_len, m->coin_size, m->coin_crc);
}

unsigned backup_list(const Storage *s, BackupInfo *out, unsigned max)
{
    NameList list;
    unsigned total = 0;
    if (!list_names(s, &list))
        return 0;
    for (unsigned i = 0; i < list.count; i++) {
        Manifest m;
        if (read_manifest(s, list.names[i], &m) != BACKUP_OK)
            continue;
        if (total < max) {
            strcpy(out[total].name, list.names[i]);
            out[total].manifest = m;
        }
        total++;
    }
    free(list.names);
    return total;
}

BackupResult backup_remove(const Storage *s, const char *name)
{
    if (!backup_name_valid(name))
        return BACKUP_ERR_NAME;
    if (backup_name_is_original(name))
        return BACKUP_ERR_PROTECTED;
    return remove_folder(s, name) == STORE_OK ? BACKUP_OK : BACKUP_ERR_IO;
}

unsigned backup_prune(const Storage *s, unsigned keep)
{
    NameList list;
    unsigned removed = 0;
    if (!list_names(s, &list) || list.count == 0) {
        free(list.names);
        return 0;
    }
    const char **names = malloc(list.count * sizeof(*names));
    bool *drop = malloc(list.count * sizeof(*drop));
    if (names != NULL && drop != NULL) {
        for (unsigned i = 0; i < list.count; i++)
            names[i] = list.names[i];
        backup_prune_select(names, list.count, keep, drop);
        for (unsigned i = 0; i < list.count; i++) {
            if (drop[i] && backup_remove(s, names[i]) == BACKUP_OK)
                removed++;
        }
    }
    free(names);
    free(drop);
    free(list.names);
    return removed;
}
