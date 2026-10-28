#ifndef PSS_PLATFORM_FS_ARCHIVES_H
#define PSS_PLATFORM_FS_ARCHIVES_H

#include <3ds.h>

#include "core/mii_plaza.h"
#include "platform/storage.h"

/*
 * The Storage table on the console:
 *
 *   GAME_MEET  /meet.dat in ARCHIVE_SYSTEM_SAVEDATA {NAND, title save id}
 *   GAME_COIN  /gamecoin.dat in ARCHIVE_SHARED_EXTDATA {NAND, 0xF000000B}
 *   sd_*       sdmc:/3ds/PuzzleSwapShop/<path>
 *   boss_*     ARCHIVE_BOSS_EXTDATA {SD, Mii Plaza extdata id}, read only
 *
 * Game archives are opened for each call and closed again, so every read sees
 * what is on the NAND and every write ends with commit or flush. Only
 * backup.c, journal.c and transaction.c use the game files and the SD folder,
 * and the model reads the SpotPass files (boss_*); only transaction.c writes
 * game files.
 */

#define FS_APP_DIR "/3ds/PuzzleSwapShop"

typedef struct {
    const PlazaTitle *title;
    FS_Archive sdmc;
    bool sdmc_open;
    FS_Archive boss; /* SpotPass downloads, opened on first use (hypothesis) */
    bool boss_open;
    bool boss_tried;
    Result last_error; /* last failing libctru result, shown on error screens */
} FsArchives;

/* Opens the SD card and creates the app folder. */
StoreResult fs_archives_init(FsArchives *fs, const PlazaTitle *title);
void fs_archives_exit(FsArchives *fs);

/* Points out at fs; out stays valid while fs does. */
void fs_archives_storage(FsArchives *fs, Storage *out);

/* Size of a game file without reading it, to tell E-01 (no access) from E-03. */
StoreResult fs_archives_game_size(FsArchives *fs, GameFile file, uint64_t *size);

#endif
