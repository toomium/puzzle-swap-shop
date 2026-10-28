#include "core/journal_record.h"

#include <stdio.h>
#include <string.h>

#include "core/kvtext.h"

static const char *const op_names[JOURNAL_OP_COUNT] = {"purchase", "restore", "diag"};

const char *journal_op_name(JournalOp op)
{
    return (unsigned)op < JOURNAL_OP_COUNT ? op_names[op] : NULL;
}

JournalRecResult journal_record_format(const JournalRecord *rec, char *out, size_t cap, size_t *len)
{
    const char *op = journal_op_name(rec->op);
    if (op == NULL || !backup_name_valid(rec->backup))
        return JOURNAL_REC_ERR_MISSING;
    if (cap == 0)
        return JOURNAL_REC_ERR_SPACE;

    char num[16];
    size_t n = 0;
    out[0] = '\0';

#define APPEND(key, value)                                                                         \
    do {                                                                                           \
        if (kv_append(out, cap, &n, key, value) != KV_OK) {                                        \
            out[0] = '\0';                                                                         \
            return JOURNAL_REC_ERR_SPACE;                                                          \
        }                                                                                          \
    } while (0)
#define APPEND_CRC(key, value)                                                                     \
    do {                                                                                           \
        snprintf(num, sizeof(num), "%08lx", (unsigned long)(value));                               \
        APPEND(key, num);                                                                          \
    } while (0)

    snprintf(num, sizeof(num), "%d", JOURNAL_FORMAT);
    APPEND("format", num);
    APPEND("op", op);
    APPEND("backup", rec->backup);
    APPEND_CRC("meet_crc_before", rec->meet_crc_before);
    APPEND_CRC("coin_crc_before", rec->coin_crc_before);
    APPEND_CRC("meet_crc_after", rec->meet_crc_after);
    APPEND_CRC("coin_crc_after", rec->coin_crc_after);
    APPEND("end", "1");
#undef APPEND_CRC
#undef APPEND

    *len = n;
    return JOURNAL_REC_OK;
}

static int find(const char *const *names, unsigned count, const char *value)
{
    if (value == NULL)
        return -1;
    for (unsigned i = 0; i < count; i++) {
        if (strcmp(names[i], value) == 0)
            return (int)i;
    }
    return -1;
}

JournalRecResult journal_record_parse(const char *text, size_t len, JournalRecord *rec)
{
    KvList kv;
    JournalRecord r;
    memset(&r, 0, sizeof(r));

    if (kv_parse(text, len, &kv) != KV_OK)
        return JOURNAL_REC_ERR_SYNTAX;

    uint32_t format, end;
    if (!kv_get_u32(&kv, "format", &format))
        return JOURNAL_REC_ERR_MISSING;
    if (format != JOURNAL_FORMAT)
        return JOURNAL_REC_ERR_FORMAT;
    if (!kv_get_u32(&kv, "end", &end) || end != 1)
        return JOURNAL_REC_ERR_MISSING;

    int op = find(op_names, JOURNAL_OP_COUNT, kv_get(&kv, "op"));
    const char *backup = kv_get(&kv, "backup");
    if (op < 0 || backup == NULL || !backup_name_valid(backup))
        return JOURNAL_REC_ERR_MISSING;
    r.op = (JournalOp)op;
    strcpy(r.backup, backup);

    if (!kv_get_hex32(&kv, "meet_crc_before", &r.meet_crc_before) ||
        !kv_get_hex32(&kv, "coin_crc_before", &r.coin_crc_before) ||
        !kv_get_hex32(&kv, "meet_crc_after", &r.meet_crc_after) ||
        !kv_get_hex32(&kv, "coin_crc_after", &r.coin_crc_after))
        return JOURNAL_REC_ERR_MISSING;

    *rec = r;
    return JOURNAL_REC_OK;
}
