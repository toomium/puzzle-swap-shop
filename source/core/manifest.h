#ifndef PSS_CORE_MANIFEST_H
#define PSS_CORE_MANIFEST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/backup_names.h"

/*
 * manifest.txt in every backup folder. Plain key=value text so people can
 * read it, too. It is written last; a folder without a valid manifest is an
 * incomplete backup.
 */

#define MANIFEST_FORMAT   1
#define MANIFEST_TEXT_MAX 512

typedef enum {
    BACKUP_REASON_ORIGINAL = 0, /* first start */
    BACKUP_REASON_PURCHASE,     /* before a purchase */
    BACKUP_REASON_RESTORE,      /* before restoring another backup */
    BACKUP_REASON_DIAG,         /* before a change from the diagnostic build */
    BACKUP_REASON_COUNT,
} BackupReason;

typedef struct {
    char app_version[16];
    Timestamp created;
    BackupReason reason;
    uint64_t title_id;
    uint32_t meet_size;
    uint32_t meet_crc;
    uint32_t coin_size;
    uint32_t coin_crc;

    /* Optional, only for the backup list (S7). Missing or malformed keys read as absent. */
    bool has_coins;
    uint16_t coins;    /* Play Coins stored in this backup */
    bool has_purchase; /* reason PURCHASE: what was bought right after */
    uint8_t panel;
    uint8_t pieces;
} Manifest;

typedef enum {
    MANIFEST_OK = 0,
    MANIFEST_ERR_SYNTAX,  /* not key=value text */
    MANIFEST_ERR_FORMAT,  /* unknown format number */
    MANIFEST_ERR_MISSING, /* a required key is missing or malformed */
    MANIFEST_ERR_SPACE,   /* output buffer too small */
} ManifestResult;

const char *manifest_reason_name(BackupReason reason);

/* Writes NUL-terminated text into out; *len receives its length. */
ManifestResult manifest_format(const Manifest *m, char *out, size_t cap, size_t *len);

ManifestResult manifest_parse(const char *text, size_t len, Manifest *m);

#endif
