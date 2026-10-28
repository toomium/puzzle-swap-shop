#include "platform/fs_archives.h"

#include <stdio.h>
#include <string.h>

#define SD_PATH_MAX 256
#define LIST_BATCH  4 /* entries are 0x228 bytes and live on the stack */

static const char *const game_paths[] = {"/meet.dat", "/gamecoin.dat"};

static StoreResult fail(FsArchives *fs, Result res, StoreResult kind)
{
    fs->last_error = res;
    return kind;
}

/*
 * FS result descriptions come in ranges (3dbrew, "Filesystem services"):
 * 100-179 not found, 180-199 already exists.
 */
static StoreResult map_error(FsArchives *fs, Result res)
{
    unsigned desc = (unsigned)R_DESCRIPTION(res);
    if (desc >= 100 && desc < 180)
        return fail(fs, res, STORE_ERR_NOT_FOUND);
    if (desc >= 180 && desc < 200)
        return fail(fs, res, STORE_ERR_EXISTS);
    return fail(fs, res, STORE_ERR_IO);
}

static Result open_game_archive(const FsArchives *fs, GameFile file, FS_Archive *arc)
{
    if (file == GAME_MEET) {
        u32 path[2] = {MEDIATYPE_NAND, fs->title->save_id};
        return FSUSER_OpenArchive(arc, ARCHIVE_SYSTEM_SAVEDATA,
                                  (FS_Path){PATH_BINARY, sizeof(path), path});
    }
    u32 path[3] = {MEDIATYPE_NAND, PLAZA_GAMECOIN_EXTDATA, 0};
    return FSUSER_OpenArchive(arc, ARCHIVE_SHARED_EXTDATA,
                              (FS_Path){PATH_BINARY, sizeof(path), path});
}

/* Opens archive and file; on failure nothing stays open. */
static StoreResult open_game(FsArchives *fs, GameFile file, u32 flags, FS_Archive *arc, Handle *h)
{
    if (fs->title == NULL || (unsigned)file > GAME_COIN)
        return STORE_ERR_ACCESS;
    Result res = open_game_archive(fs, file, arc);
    if (R_FAILED(res))
        return fail(fs, res, STORE_ERR_ACCESS);
    res = FSUSER_OpenFile(h, *arc, fsMakePath(PATH_ASCII, game_paths[file]), flags, 0);
    if (R_FAILED(res)) {
        FSUSER_CloseArchive(*arc);
        return map_error(fs, res);
    }
    return STORE_OK;
}

static StoreResult read_open_file(FsArchives *fs, Handle h, uint8_t *buf, size_t cap, size_t *len)
{
    u64 size = 0;
    u32 got = 0;
    Result res = FSFILE_GetSize(h, &size);
    if (R_FAILED(res))
        return fail(fs, res, STORE_ERR_IO);
    *len = (size_t)size;
    if (size > cap)
        return STORE_ERR_SIZE;
    if (size == 0)
        return STORE_OK;
    res = FSFILE_Read(h, &got, 0, buf, (u32)size);
    if (R_FAILED(res))
        return fail(fs, res, STORE_ERR_IO);
    return got == size ? STORE_OK : STORE_ERR_IO;
}

static StoreResult game_read(void *ctx, GameFile file, uint8_t *buf, size_t cap, size_t *len)
{
    FsArchives *fs = ctx;
    FS_Archive arc;
    Handle h;
    StoreResult r = open_game(fs, file, FS_OPEN_READ, &arc, &h);
    if (r != STORE_OK)
        return r;
    r = read_open_file(fs, h, buf, cap, len);
    FSFILE_Close(h);
    FSUSER_CloseArchive(arc);
    return r;
}

static StoreResult game_write(void *ctx, GameFile file, const uint8_t *buf, size_t len)
{
    FsArchives *fs = ctx;
    FS_Archive arc;
    Handle h;
    u64 size = 0;
    u32 written = 0;
    StoreResult r = open_game(fs, file, FS_OPEN_WRITE, &arc, &h);
    if (r != STORE_OK)
        return r;

    /* Never resize game files: extdata files cannot grow, and size is part of the format. */
    Result res = FSFILE_GetSize(h, &size);
    if (R_FAILED(res))
        r = fail(fs, res, STORE_ERR_IO);
    else if (size != len)
        r = STORE_ERR_SIZE;
    else {
        res = FSFILE_Write(h, &written, 0, buf, (u32)len, FS_WRITE_FLUSH);
        if (R_FAILED(res))
            r = fail(fs, res, STORE_ERR_IO);
        else if (written != len)
            r = STORE_ERR_IO;
    }
    res = FSFILE_Close(h);
    if (r == STORE_OK && R_FAILED(res))
        r = fail(fs, res, STORE_ERR_IO);

    /* System saves keep changes only after a commit; without one the old data stays. */
    if (r == STORE_OK && file == GAME_MEET) {
        res = FSUSER_ControlArchive(arc, ARCHIVE_ACTION_COMMIT_SAVE_DATA, NULL, 0, NULL, 0);
        if (R_FAILED(res))
            r = fail(fs, res, STORE_ERR_IO);
    }
    FSUSER_CloseArchive(arc);
    return r;
}

static bool sd_path(char out[SD_PATH_MAX], const char *rel)
{
    if (rel[0] == '/' || strstr(rel, "..") != NULL)
        return false;
    int n = snprintf(out, SD_PATH_MAX, "%s%s%s", FS_APP_DIR, rel[0] ? "/" : "", rel);
    return n > 0 && n < SD_PATH_MAX;
}

static StoreResult sd_read(void *ctx, const char *path, uint8_t *buf, size_t cap, size_t *len)
{
    FsArchives *fs = ctx;
    char full[SD_PATH_MAX];
    Handle h;
    if (!fs->sdmc_open || !sd_path(full, path))
        return STORE_ERR_IO;
    Result res = FSUSER_OpenFile(&h, fs->sdmc, fsMakePath(PATH_ASCII, full), FS_OPEN_READ, 0);
    if (R_FAILED(res))
        return map_error(fs, res);
    StoreResult r = read_open_file(fs, h, buf, cap, len);
    FSFILE_Close(h);
    return r;
}

static StoreResult sd_write(void *ctx, const char *path, const uint8_t *buf, size_t len)
{
    FsArchives *fs = ctx;
    char full[SD_PATH_MAX];
    Handle h;
    u32 written = 0;
    if (!fs->sdmc_open || !sd_path(full, path))
        return STORE_ERR_IO;
    Result res = FSUSER_OpenFile(&h, fs->sdmc, fsMakePath(PATH_ASCII, full),
                                 FS_OPEN_CREATE | FS_OPEN_WRITE, 0);
    if (R_FAILED(res))
        return map_error(fs, res);
    StoreResult r = STORE_OK;
    res = FSFILE_SetSize(h, len);
    if (R_FAILED(res))
        r = fail(fs, res, STORE_ERR_IO);
    if (r == STORE_OK && len > 0) {
        res = FSFILE_Write(h, &written, 0, buf, (u32)len, FS_WRITE_FLUSH);
        if (R_FAILED(res))
            r = fail(fs, res, STORE_ERR_IO);
        else if (written != len)
            r = STORE_ERR_IO;
    }
    res = FSFILE_Close(h);
    if (r == STORE_OK && R_FAILED(res))
        r = fail(fs, res, STORE_ERR_IO);
    return r;
}

static StoreResult sd_remove(void *ctx, const char *path)
{
    FsArchives *fs = ctx;
    char full[SD_PATH_MAX];
    if (!fs->sdmc_open || !sd_path(full, path))
        return STORE_ERR_IO;
    Result res = FSUSER_DeleteFile(fs->sdmc, fsMakePath(PATH_ASCII, full));
    return R_SUCCEEDED(res) ? STORE_OK : map_error(fs, res);
}

static bool dir_exists(FsArchives *fs, const char *full)
{
    Handle h;
    if (R_FAILED(FSUSER_OpenDirectory(&h, fs->sdmc, fsMakePath(PATH_ASCII, full))))
        return false;
    FSDIR_Close(h);
    return true;
}

static StoreResult sd_mkdir(void *ctx, const char *path)
{
    FsArchives *fs = ctx;
    char full[SD_PATH_MAX];
    if (!fs->sdmc_open || !sd_path(full, path))
        return STORE_ERR_IO;
    Result res = FSUSER_CreateDirectory(fs->sdmc, fsMakePath(PATH_ASCII, full), 0);
    if (R_SUCCEEDED(res))
        return STORE_OK;
    /* Backup names rely on EXISTS, so do not trust the error code alone. */
    if (dir_exists(fs, full))
        return fail(fs, res, STORE_ERR_EXISTS);
    return map_error(fs, res);
}

static StoreResult sd_rmdir(void *ctx, const char *path)
{
    FsArchives *fs = ctx;
    char full[SD_PATH_MAX];
    if (!fs->sdmc_open || !sd_path(full, path))
        return STORE_ERR_IO;
    Result res = FSUSER_DeleteDirectory(fs->sdmc, fsMakePath(PATH_ASCII, full));
    return R_SUCCEEDED(res) ? STORE_OK : map_error(fs, res);
}

/* UTF-16 to ASCII; other characters become '?', which no name of ours contains. */
static void entry_name(const FS_DirectoryEntry *e, char *out, size_t cap)
{
    size_t i = 0;
    for (; i + 1 < cap && e->name[i] != 0; i++)
        out[i] = e->name[i] < 0x80 ? (char)e->name[i] : '?';
    out[i] = '\0';
}

static StoreResult sd_list(void *ctx, const char *path, StoreListFn fn, void *arg)
{
    FsArchives *fs = ctx;
    char full[SD_PATH_MAX];
    FS_DirectoryEntry entries[LIST_BATCH];
    Handle h;
    if (!fs->sdmc_open || !sd_path(full, path))
        return STORE_ERR_IO;
    Result res = FSUSER_OpenDirectory(&h, fs->sdmc, fsMakePath(PATH_ASCII, full));
    if (R_FAILED(res))
        return map_error(fs, res);

    StoreResult r = STORE_OK;
    bool more = true;
    while (more) {
        u32 count = 0;
        res = FSDIR_Read(h, &count, LIST_BATCH, entries);
        if (R_FAILED(res)) {
            r = fail(fs, res, STORE_ERR_IO);
            break;
        }
        if (count == 0)
            break;
        for (u32 i = 0; i < count && more; i++) {
            char name[64];
            entry_name(&entries[i], name, sizeof(name));
            more = fn(name, (entries[i].attributes & FS_ATTRIBUTE_DIRECTORY) != 0,
                      entries[i].fileSize, arg);
        }
    }
    FSDIR_Close(h);
    return r;
}

#define BOSS_MAX_DEPTH 4

/* Opens the BOSS archive once; tries both extdata ids (the USA one is uncertain). */
static StoreResult open_boss(FsArchives *fs)
{
    if (fs->boss_open)
        return STORE_OK;
    if (fs->boss_tried || fs->title == NULL)
        return STORE_ERR_ACCESS;
    fs->boss_tried = true;
    Result res = 0;
    for (int i = 0; i < 2 && fs->title->extdata_ids[i] != 0; i++) {
        u32 path[3] = {MEDIATYPE_SD, fs->title->extdata_ids[i], 0};
        res = FSUSER_OpenArchive(&fs->boss, ARCHIVE_BOSS_EXTDATA,
                                 (FS_Path){PATH_BINARY, sizeof(path), path});
        if (R_SUCCEEDED(res)) {
            fs->boss_open = true;
            return STORE_OK;
        }
    }
    return fail(fs, res, STORE_ERR_ACCESS);
}

typedef struct {
    FsArchives *fs;
    StoreListFn fn;
    void *arg;
    bool stop;
} BossWalk;

static StoreResult boss_walk(BossWalk *w, char *path, size_t len, int depth)
{
    FS_DirectoryEntry entries[LIST_BATCH];
    Handle h;
    Result res = FSUSER_OpenDirectory(&h, w->fs->boss, fsMakePath(PATH_ASCII, len ? path : "/"));
    if (R_FAILED(res))
        return map_error(w->fs, res);
    StoreResult r = STORE_OK;
    while (!w->stop && r == STORE_OK) {
        u32 count = 0;
        res = FSDIR_Read(h, &count, LIST_BATCH, entries);
        if (R_FAILED(res)) {
            r = fail(w->fs, res, STORE_ERR_IO);
            break;
        }
        if (count == 0)
            break;
        for (u32 i = 0; i < count && !w->stop && r == STORE_OK; i++) {
            char name[64];
            entry_name(&entries[i], name, sizeof(name));
            size_t n = strlen(name);
            if (len + 1 + n >= SD_PATH_MAX)
                continue;
            path[len] = '/';
            memcpy(path + len + 1, name, n + 1);
            if ((entries[i].attributes & FS_ATTRIBUTE_DIRECTORY) != 0) {
                if (depth < BOSS_MAX_DEPTH)
                    r = boss_walk(w, path, len + 1 + n, depth + 1);
            } else {
                /* Directory entries of extdata archives report size 0. */
                u64 size = entries[i].fileSize;
                if (size == 0) {
                    Handle fh;
                    if (R_SUCCEEDED(FSUSER_OpenFile(&fh, w->fs->boss, fsMakePath(PATH_ASCII, path),
                                                    FS_OPEN_READ, 0))) {
                        FSFILE_GetSize(fh, &size);
                        FSFILE_Close(fh);
                    }
                }
                if (!w->fn(path, false, size, w->arg))
                    w->stop = true;
            }
            path[len] = '\0';
        }
    }
    FSDIR_Close(h);
    return r;
}

static StoreResult boss_list(void *ctx, StoreListFn fn, void *arg)
{
    FsArchives *fs = ctx;
    StoreResult r = open_boss(fs);
    if (r != STORE_OK)
        return r;
    BossWalk w = {fs, fn, arg, false};
    char path[SD_PATH_MAX] = "";
    return boss_walk(&w, path, 0, 0);
}

static StoreResult boss_read(void *ctx, const char *path, uint64_t offset, uint8_t *buf,
                             size_t want, size_t *got)
{
    FsArchives *fs = ctx;
    *got = 0;
    StoreResult r = open_boss(fs);
    if (r != STORE_OK)
        return r;
    Handle h;
    Result res = FSUSER_OpenFile(&h, fs->boss, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0);
    if (R_FAILED(res))
        return map_error(fs, res);
    u32 n = 0;
    res = want == 0 ? 0 : FSFILE_Read(h, &n, offset, buf, (u32)want);
    FSFILE_Close(h);
    if (R_FAILED(res))
        return fail(fs, res, STORE_ERR_IO);
    *got = n;
    return STORE_OK;
}

StoreResult fs_archives_init(FsArchives *fs, const PlazaTitle *title)
{
    memset(fs, 0, sizeof(*fs));
    fs->title = title;
    Result res = FSUSER_OpenArchive(&fs->sdmc, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    if (R_FAILED(res))
        return fail(fs, res, STORE_ERR_ACCESS);
    fs->sdmc_open = true;

    static const char *const dirs[] = {"/3ds", FS_APP_DIR};
    for (size_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]); i++) {
        res = FSUSER_CreateDirectory(fs->sdmc, fsMakePath(PATH_ASCII, dirs[i]), 0);
        if (R_FAILED(res) && !dir_exists(fs, dirs[i]))
            return fail(fs, res, STORE_ERR_IO);
    }
    return STORE_OK;
}

void fs_archives_exit(FsArchives *fs)
{
    if (fs->boss_open)
        FSUSER_CloseArchive(fs->boss);
    fs->boss_open = false;
    if (fs->sdmc_open)
        FSUSER_CloseArchive(fs->sdmc);
    fs->sdmc_open = false;
}

StoreResult fs_archives_game_size(FsArchives *fs, GameFile file, uint64_t *size)
{
    FS_Archive arc;
    Handle h;
    u64 s = 0;
    StoreResult r = open_game(fs, file, FS_OPEN_READ, &arc, &h);
    if (r != STORE_OK)
        return r;
    Result res = FSFILE_GetSize(h, &s);
    FSFILE_Close(h);
    FSUSER_CloseArchive(arc);
    if (R_FAILED(res))
        return fail(fs, res, STORE_ERR_IO);
    *size = s;
    return STORE_OK;
}

void fs_archives_storage(FsArchives *fs, Storage *out)
{
    out->ctx = fs;
    out->game_read = game_read;
    out->game_write = game_write;
    out->sd_read = sd_read;
    out->sd_write = sd_write;
    out->sd_remove = sd_remove;
    out->sd_mkdir = sd_mkdir;
    out->sd_rmdir = sd_rmdir;
    out->sd_list = sd_list;
    out->boss_list = boss_list;
    out->boss_read = boss_read;
}
