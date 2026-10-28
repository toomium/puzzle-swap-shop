#ifndef PSS_CORE_JOURNAL_RECORD_H
#define PSS_CORE_JOURNAL_RECORD_H

#include <stddef.h>
#include <stdint.h>

#include "core/backup_names.h"

/*
 * Content of journal.txt, which exists while game data is being changed. It is written once before
 * the first game write and removed at the end; it is never rewritten in between, so an interruption
 * cannot damage it. Whether the change went through is decided at start-up from the CRCs. The last
 * line is "end=1", so a file that was cut short while writing is recognised as damaged.
 */

#define JOURNAL_FORMAT   1
#define JOURNAL_TEXT_MAX 512

typedef enum {
    JOURNAL_OP_PURCHASE = 0,
    JOURNAL_OP_RESTORE,
    JOURNAL_OP_DIAG,
    JOURNAL_OP_COUNT,
} JournalOp;

typedef struct {
    JournalOp op;
    /*
     * Backup to restore if the change was interrupted: the state before a
     * purchase or diagnostic change, or the backup that was being restored.
     */
    char backup[BACKUP_NAME_MAX];
    uint32_t meet_crc_before;
    uint32_t coin_crc_before;
    uint32_t meet_crc_after;
    uint32_t coin_crc_after;
} JournalRecord;

typedef enum {
    JOURNAL_REC_OK = 0,
    JOURNAL_REC_ERR_SYNTAX,  /* not key=value text */
    JOURNAL_REC_ERR_FORMAT,  /* unknown format number */
    JOURNAL_REC_ERR_MISSING, /* a key is missing or malformed, or end=1 is absent */
    JOURNAL_REC_ERR_SPACE,   /* output buffer too small */
} JournalRecResult;

const char *journal_op_name(JournalOp op);

JournalRecResult journal_record_format(const JournalRecord *rec, char *out, size_t cap,
                                       size_t *len);

JournalRecResult journal_record_parse(const char *text, size_t len, JournalRecord *rec);

#endif
