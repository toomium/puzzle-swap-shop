#ifndef PSS_PLATFORM_STORAGE_H
#define PSS_PLATFORM_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Everything backup, journal and transaction need from the file system. On the
 * console fs_archives.c implements it; host tests use an in-memory fake that
 * can fail any call. Keeping this the only way to files is what makes every
 * error path of a purchase testable on the PC.
 *
 * Game files are written only by transaction.c.
 */

typedef enum {
    GAME_MEET = 0, /* meet.dat in the Mii Plaza system save (NAND) */
    GAME_COIN,     /* gamecoin.dat in shared extdata 0xF000000B (NAND) */
} GameFile;

typedef enum {
    STORE_OK = 0,
    STORE_ERR_NOT_FOUND, /* file or folder does not exist */
    STORE_ERR_EXISTS,    /* folder already exists */
    STORE_ERR_SIZE,      /* file larger than the buffer, or write size differs */
    STORE_ERR_ACCESS,    /* archive cannot be opened (no CFW rights, not installed) */
    STORE_ERR_IO,        /* any other failure */
} StoreResult;

/* Called once per entry; return false to stop listing. */
typedef bool (*StoreListFn)(const char *name, bool is_dir, uint64_t size, void *arg);

typedef struct Storage {
    void *ctx;

    /* Reads a whole game file. *len receives its size; STORE_ERR_SIZE if it exceeds cap. */
    StoreResult (*game_read)(void *ctx, GameFile file, uint8_t *buf, size_t cap, size_t *len);

    /*
     * Overwrites a whole game file with exactly its current size and makes the
     * change permanent (commit for the system save, flush for extdata).
     */
    StoreResult (*game_write)(void *ctx, GameFile file, const uint8_t *buf, size_t len);

    /*
     * The app folder on the SD card. Paths are relative to it and use '/',
     * for example "backups/000-original/meet.dat".
     */
    StoreResult (*sd_read)(void *ctx, const char *path, uint8_t *buf, size_t cap, size_t *len);

    /* Creates or replaces a file with exactly these bytes, flushed to the card. */
    StoreResult (*sd_write)(void *ctx, const char *path, const uint8_t *buf, size_t len);

    StoreResult (*sd_remove)(void *ctx, const char *path);

    /* Creates one folder level; STORE_ERR_EXISTS if it is already there. */
    StoreResult (*sd_mkdir)(void *ctx, const char *path);

    /* Removes an empty folder. */
    StoreResult (*sd_rmdir)(void *ctx, const char *path);

    StoreResult (*sd_list)(void *ctx, const char *path, StoreListFn fn, void *arg);

    /*
     * SpotPass downloads of Mii Plaza (BOSS extdata), read only; both are NULL
     * when the console cannot offer them. boss_list calls fn for every file
     * below the archive root, folders included in the name ("/dir/file");
     * folders themselves are not reported. boss_read reads up to `want` bytes
     * at `offset` and reports how many it got.
     */
    StoreResult (*boss_list)(void *ctx, StoreListFn fn, void *arg);
    StoreResult (*boss_read)(void *ctx, const char *path, uint64_t offset, uint8_t *buf,
                             size_t want, size_t *got);
} Storage;

#endif
