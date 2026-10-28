#include "platform/journal.h"

#include <string.h>

StoreResult journal_write(const Storage *s, const JournalRecord *rec)
{
    char text[JOURNAL_TEXT_MAX];
    char check[JOURNAL_TEXT_MAX];
    size_t len, got;

    if (journal_record_format(rec, text, sizeof(text), &len) != JOURNAL_REC_OK)
        return STORE_ERR_IO;
    StoreResult r = s->sd_write(s->ctx, JOURNAL_FILE, (const uint8_t *)text, len);
    if (r != STORE_OK)
        return r;
    r = s->sd_read(s->ctx, JOURNAL_FILE, (uint8_t *)check, sizeof(check), &got);
    if (r != STORE_OK)
        return r;
    if (got != len || memcmp(check, text, len) != 0)
        return STORE_ERR_IO;
    return STORE_OK;
}

JournalStatus journal_read(const Storage *s, JournalRecord *rec)
{
    char text[JOURNAL_TEXT_MAX];
    size_t len;

    StoreResult r = s->sd_read(s->ctx, JOURNAL_FILE, (uint8_t *)text, sizeof(text), &len);
    if (r == STORE_ERR_NOT_FOUND)
        return JOURNAL_NONE;
    if (r == STORE_ERR_SIZE)
        return JOURNAL_DAMAGED;
    if (r != STORE_OK)
        return JOURNAL_IO;
    if (journal_record_parse(text, len, rec) != JOURNAL_REC_OK)
        return JOURNAL_DAMAGED;
    return JOURNAL_FOUND;
}

StoreResult journal_clear(const Storage *s)
{
    StoreResult r = s->sd_remove(s->ctx, JOURNAL_FILE);
    return r == STORE_ERR_NOT_FOUND ? STORE_OK : r;
}
