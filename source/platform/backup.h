#ifndef PSS_PLATFORM_BACKUP_H
#define PSS_PLATFORM_BACKUP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/backup_names.h"
#include "core/manifest.h"
#include "platform/storage.h"

/*
 * Backups on the SD card: backups/<name>/ with meet.dat, gamecoin.dat and
 * manifest.txt. Only writes to the SD
 * card; restoring a backup onto the game data is transaction_restore().
 */

#define BACKUP_DIR          "backups"
#define BACKUP_KEEP_DEFAULT 20

typedef enum {
    BACKUP_OK = 0,
    BACKUP_ERR_NAME,      /* not a name this app creates */
    BACKUP_ERR_EXISTS,    /* folder already exists, or no free name for this second */
    BACKUP_ERR_WRITE,     /* writing failed; the partial folder was removed */
    BACKUP_ERR_VERIFY,    /* read-back differs; the folder was removed */
    BACKUP_ERR_NOT_FOUND, /* no such backup */
    BACKUP_ERR_MANIFEST,  /* manifest.txt missing or unreadable */
    BACKUP_ERR_DAMAGED,   /* a file does not match its manifest entry */
    BACKUP_ERR_PROTECTED, /* the original backup cannot be removed */
    BACKUP_ERR_IO,        /* other storage error */
    BACKUP_ERR_MEMORY,
} BackupResult;

typedef struct {
    char name[BACKUP_NAME_MAX];
    Manifest manifest;
} BackupInfo;

/* First free name for this time: the plain name, then "-2" to "-9". */
BackupResult backup_choose_name(const Storage *s, const Timestamp *t, char out[BACKUP_NAME_MAX]);

/*
 * Writes a backup and reads every file back. meta supplies app_version,
 * created, reason and title_id; sizes and CRCs are filled in here and returned
 * in *written if it is not NULL. On failure nothing is left behind.
 */
BackupResult backup_create(const Storage *s, const char *name, const Manifest *meta,
                           const uint8_t *meet, size_t meet_len, const uint8_t *coin,
                           size_t coin_len, Manifest *written);

/* Reads a backup; both files must match size and CRC in its manifest. */
BackupResult backup_load(const Storage *s, const char *name, Manifest *m, uint8_t *meet,
                         size_t meet_cap, size_t *meet_len, uint8_t *coin, size_t coin_cap,
                         size_t *coin_len);

/*
 * Complete backups (valid name and manifest), newest first, so the original
 * comes last. Fills at most max entries and returns how many exist in total.
 */
unsigned backup_list(const Storage *s, BackupInfo *out, unsigned max);

bool backup_exists(const Storage *s, const char *name);

/* Removes one backup folder. Refuses the original. */
BackupResult backup_remove(const Storage *s, const char *name);

/* Removes all but the newest `keep` backups, never the original. Returns how many went. */
unsigned backup_prune(const Storage *s, unsigned keep);

#endif
