#include "fake_storage.h"

#include <stdlib.h>
#include <string.h>

static FakeEntry *find(FakeStorage *f, const char *path)
{
    for (unsigned i = 0; i < f->entry_count; i++) {
        if (strcmp(f->entries[i].path, path) == 0)
            return &f->entries[i];
    }
    return NULL;
}

const FakeEntry *fake_find(const FakeStorage *f, const char *path)
{
    return find((FakeStorage *)f, path);
}

/* Parent folder of path must exist (the app root "" always does). */
static bool parent_exists(FakeStorage *f, const char *path)
{
    const char *slash = strrchr(path, '/');
    if (slash == NULL)
        return true;
    char parent[FAKE_PATH_MAX];
    size_t n = (size_t)(slash - path);
    memcpy(parent, path, n);
    parent[n] = '\0';
    FakeEntry *e = find(f, parent);
    return e != NULL && e->is_dir;
}

static FakeEntry *add(FakeStorage *f, const char *path, bool is_dir)
{
    if (f->entry_count == FAKE_MAX_ENTRIES || strlen(path) >= FAKE_PATH_MAX)
        return NULL;
    FakeEntry *e = &f->entries[f->entry_count++];
    memset(e, 0, sizeof(*e));
    strcpy(e->path, path);
    e->is_dir = is_dir;
    return e;
}

static void drop(FakeStorage *f, FakeEntry *e)
{
    free(e->data);
    *e = f->entries[--f->entry_count];
}

/* Counts the call and decides whether it fails. */
static bool fails(FakeStorage *f, FakeOp op, int game_file)
{
    f->calls++;
    f->op_calls[op]++;
    if (f->power_off_call != 0 && f->calls >= f->power_off_call)
        f->powered_off = true;
    if (f->powered_off)
        return true;
    if (f->fail_call != 0 && f->calls == f->fail_call)
        return true;
    if (f->fail_op == (int)op && f->op_calls[op] == f->fail_op_nth &&
        (f->fail_game_file < 0 || f->fail_game_file == game_file))
        return true;
    return false;
}

static StoreResult game_read(void *ctx, GameFile file, uint8_t *buf, size_t cap, size_t *len)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_GAME_READ, (int)file))
        return STORE_ERR_IO;
    if (!f->game_present[file])
        return STORE_ERR_NOT_FOUND;
    *len = f->game_len[file];
    if (*len > cap)
        return STORE_ERR_SIZE;
    memcpy(buf, f->game[file], *len);
    return STORE_OK;
}

static StoreResult game_write(void *ctx, GameFile file, const uint8_t *buf, size_t len)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_GAME_WRITE, (int)file)) {
        /* Losing power mid-write leaves a mix of old and new bytes. */
        if (f->powered_off && f->game_present[file] && len == f->game_len[file])
            memcpy(f->game[file], buf, len / 2);
        return STORE_ERR_IO;
    }
    if (!f->game_present[file])
        return STORE_ERR_NOT_FOUND;
    if (len != f->game_len[file])
        return STORE_ERR_SIZE;
    memcpy(f->game[file], buf, len);
    return STORE_OK;
}

static StoreResult sd_read(void *ctx, const char *path, uint8_t *buf, size_t cap, size_t *len)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_SD_READ, -1))
        return STORE_ERR_IO;
    FakeEntry *e = find(f, path);
    if (e == NULL || e->is_dir)
        return STORE_ERR_NOT_FOUND;
    *len = e->len;
    if (e->len > cap)
        return STORE_ERR_SIZE;
    if (e->len)
        memcpy(buf, e->data, e->len);
    return STORE_OK;
}

bool fake_put(FakeStorage *f, const char *path, const void *data, size_t len)
{
    FakeEntry *e = find(f, path);
    if (e != NULL && e->is_dir)
        return false;
    if (e == NULL && (!parent_exists(f, path) || (e = add(f, path, false)) == NULL))
        return false;
    free(e->data);
    e->data = malloc(len ? len : 1);
    if (len)
        memcpy(e->data, data, len);
    e->len = len;
    return true;
}

static StoreResult sd_write(void *ctx, const char *path, const uint8_t *buf, size_t len)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_SD_WRITE, -1)) {
        /* A cut-off write leaves a truncated file. */
        if (f->powered_off)
            fake_put(f, path, buf, len / 2);
        return STORE_ERR_IO;
    }
    return fake_put(f, path, buf, len) ? STORE_OK : STORE_ERR_IO;
}

static StoreResult sd_remove(void *ctx, const char *path)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_SD_REMOVE, -1))
        return STORE_ERR_IO;
    FakeEntry *e = find(f, path);
    if (e == NULL || e->is_dir)
        return STORE_ERR_NOT_FOUND;
    drop(f, e);
    return STORE_OK;
}

bool fake_mkdir(FakeStorage *f, const char *path)
{
    if (find(f, path) != NULL || !parent_exists(f, path))
        return false;
    return add(f, path, true) != NULL;
}

static StoreResult sd_mkdir(void *ctx, const char *path)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_SD_MKDIR, -1))
        return STORE_ERR_IO;
    if (find(f, path) != NULL)
        return STORE_ERR_EXISTS;
    if (!parent_exists(f, path))
        return STORE_ERR_NOT_FOUND;
    return add(f, path, true) ? STORE_OK : STORE_ERR_IO;
}

static bool has_children(FakeStorage *f, const char *path)
{
    size_t n = strlen(path);
    for (unsigned i = 0; i < f->entry_count; i++) {
        if (strncmp(f->entries[i].path, path, n) == 0 && f->entries[i].path[n] == '/')
            return true;
    }
    return false;
}

static StoreResult sd_rmdir(void *ctx, const char *path)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_SD_RMDIR, -1))
        return STORE_ERR_IO;
    FakeEntry *e = find(f, path);
    if (e == NULL || !e->is_dir)
        return STORE_ERR_NOT_FOUND;
    if (has_children(f, path))
        return STORE_ERR_IO;
    drop(f, e);
    return STORE_OK;
}

static StoreResult sd_list(void *ctx, const char *path, StoreListFn fn, void *arg)
{
    FakeStorage *f = ctx;
    if (fails(f, FAKE_SD_LIST, -1))
        return STORE_ERR_IO;
    FakeEntry *dir = find(f, path);
    if (dir == NULL || !dir->is_dir)
        return STORE_ERR_NOT_FOUND;
    size_t n = strlen(path);
    /* Snapshot the names first: the callback must not see a changing table. */
    char(*names)[FAKE_PATH_MAX] = malloc(FAKE_MAX_ENTRIES * sizeof(*names));
    bool dirs[FAKE_MAX_ENTRIES];
    uint64_t sizes[FAKE_MAX_ENTRIES];
    unsigned count = 0;
    if (names == NULL)
        return STORE_ERR_IO;
    for (unsigned i = 0; i < f->entry_count; i++) {
        const char *p = f->entries[i].path;
        if (strncmp(p, path, n) != 0 || p[n] != '/' || strchr(p + n + 1, '/') != NULL)
            continue;
        strcpy(names[count], p + n + 1);
        dirs[count] = f->entries[i].is_dir;
        sizes[count] = f->entries[i].len;
        count++;
    }
    for (unsigned i = 0; i < count; i++) {
        if (!fn(names[i], dirs[i], sizes[i], arg))
            break;
    }
    free(names);
    return STORE_OK;
}

static StoreResult boss_list(void *ctx, StoreListFn fn, void *arg)
{
    FakeStorage *f = ctx;
    for (unsigned i = 0; i < f->boss_count; i++) {
        if (!fn(f->boss[i].path, false, f->boss[i].len, arg))
            break;
    }
    return STORE_OK;
}

static StoreResult boss_read(void *ctx, const char *path, uint64_t offset, uint8_t *buf,
                             size_t want, size_t *got)
{
    FakeStorage *f = ctx;
    for (unsigned i = 0; i < f->boss_count; i++) {
        const FakeEntry *e = &f->boss[i];
        if (strcmp(e->path, path) != 0)
            continue;
        size_t n = offset >= e->len ? 0 : e->len - (size_t)offset;
        if (n > want)
            n = want;
        if (n)
            memcpy(buf, e->data + offset, n);
        *got = n;
        return STORE_OK;
    }
    return STORE_ERR_NOT_FOUND;
}

bool fake_put_boss(FakeStorage *f, const char *path, const void *data, size_t len)
{
    if (f->boss_count == FAKE_MAX_BOSS || strlen(path) >= FAKE_PATH_MAX)
        return false;
    FakeEntry *e = &f->boss[f->boss_count++];
    memset(e, 0, sizeof(*e));
    strcpy(e->path, path);
    e->data = malloc(len ? len : 1);
    memcpy(e->data, data, len);
    e->len = len;
    return true;
}

void fake_storage_init(FakeStorage *f, const uint8_t *meet, size_t meet_len, const uint8_t *coin,
                       size_t coin_len)
{
    memset(f, 0, sizeof(*f));
    const uint8_t *src[2] = {meet, coin};
    size_t len[2] = {meet_len, coin_len};
    for (int i = 0; i < 2; i++) {
        f->game_present[i] = src[i] != NULL;
        f->game_len[i] = len[i];
        f->game[i] = malloc(len[i] ? len[i] : 1);
        if (src[i] != NULL && len[i])
            memcpy(f->game[i], src[i], len[i]);
    }
    f->storage.ctx = f;
    f->storage.game_read = game_read;
    f->storage.game_write = game_write;
    f->storage.sd_read = sd_read;
    f->storage.sd_write = sd_write;
    f->storage.sd_remove = sd_remove;
    f->storage.sd_mkdir = sd_mkdir;
    f->storage.sd_rmdir = sd_rmdir;
    f->storage.sd_list = sd_list;
    f->storage.boss_list = boss_list;
    f->storage.boss_read = boss_read;
    fake_storage_reboot(f);
}

void fake_storage_reboot(FakeStorage *f)
{
    f->calls = 0;
    memset(f->op_calls, 0, sizeof(f->op_calls));
    f->fail_call = 0;
    f->power_off_call = 0;
    f->fail_op = -1;
    f->fail_op_nth = 0;
    f->fail_game_file = -1;
    f->powered_off = false;
}

void fake_storage_free(FakeStorage *f)
{
    for (int i = 0; i < 2; i++)
        free(f->game[i]);
    for (unsigned i = 0; i < f->entry_count; i++)
        free(f->entries[i].data);
    for (unsigned i = 0; i < f->boss_count; i++)
        free(f->boss[i].data);
    memset(f, 0, sizeof(*f));
}

const uint8_t *fake_game(const FakeStorage *f, GameFile file, size_t *len)
{
    if (len != NULL)
        *len = f->game_len[file];
    return f->game[file];
}
