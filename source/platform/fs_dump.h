#ifndef PSS_PLATFORM_FS_DUMP_H
#define PSS_PLATFORM_FS_DUMP_H

#include <3ds.h>

#include "core/mii_plaza.h"

/*
 * Diagnosis build only: lists and reads every file in the archives
 * around Mii Plaza to find where the PNEL files live. Opens archives read only
 * and never writes to them.
 */

typedef enum {
    DUMP_PLAZA_EXTDATA = 0, /* ARCHIVE_EXTDATA, SD, Mii Plaza extdata id */
    DUMP_PLAZA_BOSS,        /* ARCHIVE_BOSS_EXTDATA, same id: SpotPass downloads */
    DUMP_PLAZA_SAVE,        /* ARCHIVE_SYSTEM_SAVEDATA, NAND, holds meet.dat */
    DUMP_SHARED_COIN,       /* ARCHIVE_SHARED_EXTDATA 0xF000000B, holds gamecoin.dat */
    DUMP_ARCHIVE_COUNT,
} DumpArchive;

/* Folder name below dump/ on the SD card, for example "plaza-boss". */
const char *fs_dump_label(DumpArchive which);

/* Called for every file and folder; path starts with '/'. Return false to stop. */
typedef bool (*DumpEntryFn)(const char *path, bool is_dir, u64 size, void *arg);

/*
 * Walks the archive recursively. *id_used receives the archive id that
 * opened (the USA extdata id is not certain, so both candidates are tried).
 */
Result fs_dump_walk(const PlazaTitle *title, DumpArchive which, DumpEntryFn fn, void *arg,
                    u32 *id_used);

/* Reads one whole file into a new buffer that the caller frees. */
Result fs_dump_read(const PlazaTitle *title, DumpArchive which, const char *path, u8 **data,
                    u32 *len);

#endif
