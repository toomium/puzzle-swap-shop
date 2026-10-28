#include "platform/fs_dump.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DUMP_PATH_MAX  256
#define DUMP_MAX_DEPTH 8
#define DUMP_MAX_FILE  (16u * 1024 * 1024)

static const char *const labels[DUMP_ARCHIVE_COUNT] = {
    "plaza-extdata",
    "plaza-boss",
    "plaza-save",
    "shared-f000000b",
};

const char *fs_dump_label(DumpArchive which)
{
    return (unsigned)which < DUMP_ARCHIVE_COUNT ? labels[which] : "?";
}

static Result open_with_id(DumpArchive which, u32 id, FS_Archive *arc)
{
    switch (which) {
    case DUMP_PLAZA_EXTDATA:
    case DUMP_PLAZA_BOSS: {
        u32 path[3] = {MEDIATYPE_SD, id, 0};
        FS_ArchiveID kind = which == DUMP_PLAZA_BOSS ? ARCHIVE_BOSS_EXTDATA : ARCHIVE_EXTDATA;
        return FSUSER_OpenArchive(arc, kind, (FS_Path){PATH_BINARY, sizeof(path), path});
    }
    case DUMP_PLAZA_SAVE: {
        u32 path[2] = {MEDIATYPE_NAND, id};
        return FSUSER_OpenArchive(arc, ARCHIVE_SYSTEM_SAVEDATA,
                                  (FS_Path){PATH_BINARY, sizeof(path), path});
    }
    default: {
        u32 path[3] = {MEDIATYPE_NAND, id, 0};
        return FSUSER_OpenArchive(arc, ARCHIVE_SHARED_EXTDATA,
                                  (FS_Path){PATH_BINARY, sizeof(path), path});
    }
    }
}

static Result open_archive(const PlazaTitle *title, DumpArchive which, FS_Archive *arc,
                           u32 *id_used)
{
    u32 ids[2] = {0, 0};
    switch (which) {
    case DUMP_PLAZA_EXTDATA:
    case DUMP_PLAZA_BOSS:
        ids[0] = title->extdata_ids[0];
        ids[1] = title->extdata_ids[1];
        break;
    case DUMP_PLAZA_SAVE:
        ids[0] = title->save_id;
        break;
    case DUMP_SHARED_COIN:
        ids[0] = PLAZA_GAMECOIN_EXTDATA;
        break;
    default:
        return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_ENUM_VALUE);
    }
    Result res = 0;
    for (int i = 0; i < 2 && ids[i] != 0; i++) {
        res = open_with_id(which, ids[i], arc);
        if (R_SUCCEEDED(res)) {
            if (id_used != NULL)
                *id_used = ids[i];
            return res;
        }
    }
    return res;
}

typedef struct {
    FS_Archive arc;
    DumpEntryFn fn;
    void *arg;
    bool stop;
} Walk;

static Result walk_dir(Walk *w, char *path, size_t len, int depth)
{
    Handle h;
    Result res = FSUSER_OpenDirectory(&h, w->arc, fsMakePath(PATH_ASCII, len ? path : "/"));
    if (R_FAILED(res))
        return res;
    FS_DirectoryEntry e;
    u32 count = 0;
    while (!w->stop && R_SUCCEEDED(res = FSDIR_Read(h, &count, 1, &e)) && count == 1) {
        char name[64];
        size_t n = 0;
        for (; n + 1 < sizeof(name) && e.name[n] != 0; n++)
            name[n] = e.name[n] < 0x80 ? (char)e.name[n] : '?';
        name[n] = '\0';
        if (len + 1 + n >= DUMP_PATH_MAX)
            continue;
        path[len] = '/';
        memcpy(path + len + 1, name, n + 1);
        bool is_dir = (e.attributes & FS_ATTRIBUTE_DIRECTORY) != 0;
        if (!w->fn(path, is_dir, e.fileSize, w->arg))
            w->stop = true;
        else if (is_dir && depth < DUMP_MAX_DEPTH)
            res = walk_dir(w, path, len + 1 + n, depth + 1);
        path[len] = '\0';
        if (R_FAILED(res))
            break;
    }
    FSDIR_Close(h);
    return res;
}

Result fs_dump_walk(const PlazaTitle *title, DumpArchive which, DumpEntryFn fn, void *arg,
                    u32 *id_used)
{
    Walk w = {0, fn, arg, false};
    char path[DUMP_PATH_MAX] = "";
    Result res = open_archive(title, which, &w.arc, id_used);
    if (R_FAILED(res))
        return res;
    res = walk_dir(&w, path, 0, 0);
    FSUSER_CloseArchive(w.arc);
    return res;
}

Result fs_dump_read(const PlazaTitle *title, DumpArchive which, const char *path, u8 **data,
                    u32 *len)
{
    FS_Archive arc;
    Handle h;
    u64 size = 0;
    u32 got = 0;
    *data = NULL;
    *len = 0;
    Result res = open_archive(title, which, &arc, NULL);
    if (R_FAILED(res))
        return res;
    res = FSUSER_OpenFile(&h, arc, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0);
    if (R_SUCCEEDED(res)) {
        res = FSFILE_GetSize(h, &size);
        if (R_SUCCEEDED(res) && size > DUMP_MAX_FILE)
            res = MAKERESULT(RL_PERMANENT, RS_OUTOFRESOURCE, RM_APPLICATION, RD_TOO_LARGE);
        u8 *buf = NULL;
        if (R_SUCCEEDED(res)) {
            buf = malloc(size ? (size_t)size : 1);
            if (buf == NULL)
                res = MAKERESULT(RL_PERMANENT, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY);
        }
        if (R_SUCCEEDED(res) && size > 0)
            res = FSFILE_Read(h, &got, 0, buf, (u32)size);
        if (R_SUCCEEDED(res) && got != size)
            res = MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_NO_DATA);
        FSFILE_Close(h);
        if (R_SUCCEEDED(res)) {
            *data = buf;
            *len = (u32)size;
        } else {
            free(buf);
        }
    }
    FSUSER_CloseArchive(arc);
    return res;
}
