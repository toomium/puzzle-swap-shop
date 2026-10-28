#ifndef PSS_PLATFORM_JOURNAL_H
#define PSS_PLATFORM_JOURNAL_H

#include "core/journal_record.h"
#include "platform/storage.h"

/* journal.txt in the app folder on the SD card. */

#define JOURNAL_FILE "journal.txt"

typedef enum {
    JOURNAL_NONE = 0, /* no journal: nothing was interrupted */
    JOURNAL_FOUND,    /* *rec holds the journal */
    JOURNAL_DAMAGED,  /* a journal exists but cannot be read */
    JOURNAL_IO,       /* the SD card cannot be read */
} JournalStatus;

/* Writes the whole journal and reads it back. */
StoreResult journal_write(const Storage *s, const JournalRecord *rec);

JournalStatus journal_read(const Storage *s, JournalRecord *rec);

/* Removes the journal; a missing journal is fine. */
StoreResult journal_clear(const Storage *s);

#endif
