#ifndef PSS_TESTS_FAKE_STORAGE_H
#define PSS_TESTS_FAKE_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "platform/storage.h"

/*
 * In-memory Storage for host tests. Any call can be made to fail:
 *
 *   fail_call      the n-th call (1-based) fails once
 *   power_off_call from the n-th call on every call fails, as if the console
 *                  lost power; a failing game write then leaves the first half
 *                  of the new data behind (torn write)
 *   fail_op/fail_op_nth  the n-th call of one operation fails
 *
 * fake_storage_reboot() turns all faults off, like starting the app again.
 */

typedef enum {
    FAKE_GAME_READ = 0,
    FAKE_GAME_WRITE,
    FAKE_SD_READ,
    FAKE_SD_WRITE,
    FAKE_SD_REMOVE,
    FAKE_SD_MKDIR,
    FAKE_SD_RMDIR,
    FAKE_SD_LIST,
    FAKE_OP_COUNT,
} FakeOp;

#define FAKE_MAX_ENTRIES 256
#define FAKE_PATH_MAX    96
#define FAKE_MAX_BOSS    8

typedef struct {
    char path[FAKE_PATH_MAX];
    bool is_dir;
    uint8_t *data;
    size_t len;
} FakeEntry;

typedef struct {
    uint8_t *game[2];
    size_t game_len[2];
    bool game_present[2];

    FakeEntry entries[FAKE_MAX_ENTRIES];
    unsigned entry_count;

    FakeEntry boss[FAKE_MAX_BOSS]; /* SpotPass files, read only, no fault injection */
    unsigned boss_count;

    unsigned calls;
    unsigned op_calls[FAKE_OP_COUNT];

    unsigned fail_call;
    unsigned power_off_call;
    int fail_op; /* FakeOp or -1 */
    unsigned fail_op_nth;
    int fail_game_file; /* restrict fail_op to GAME_MEET/GAME_COIN, or -1 */
    bool powered_off;

    Storage storage;
} FakeStorage;

/* Starts with the given game files (copied) and an empty SD card. */
void fake_storage_init(FakeStorage *f, const uint8_t *meet, size_t meet_len, const uint8_t *coin,
                       size_t coin_len);
void fake_storage_free(FakeStorage *f);

/* Clears all faults and counters. */
void fake_storage_reboot(FakeStorage *f);

/* Direct access for assertions; no fault injection, not counted. */
const uint8_t *fake_game(const FakeStorage *f, GameFile file, size_t *len);
const FakeEntry *fake_find(const FakeStorage *f, const char *path);
bool fake_put(FakeStorage *f, const char *path, const void *data, size_t len);
bool fake_mkdir(FakeStorage *f, const char *path);

/* Adds a file to the SpotPass (BOSS) archive; path like "/pnel.dat". */
bool fake_put_boss(FakeStorage *f, const char *path, const void *data, size_t len);

#endif
