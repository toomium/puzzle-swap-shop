#include "core/manifest.h"

#include <stdio.h>
#include <string.h>

#include "core/kvtext.h"

static const char *const reason_names[BACKUP_REASON_COUNT] = {
    "original",
    "purchase",
    "restore",
    "diag",
};

const char *manifest_reason_name(BackupReason reason)
{
    if ((unsigned)reason >= BACKUP_REASON_COUNT)
        return NULL;
    return reason_names[reason];
}

ManifestResult manifest_format(const Manifest *m, char *out, size_t cap, size_t *len)
{
    char created[20], num[24];
    const char *reason = manifest_reason_name(m->reason);
    if (reason == NULL || !timestamp_format(&m->created, created))
        return MANIFEST_ERR_MISSING;

    size_t n = 0;
    if (cap == 0)
        return MANIFEST_ERR_SPACE;
    out[0] = '\0';

#define APPEND(key, value)                                                                         \
    do {                                                                                           \
        if (kv_append(out, cap, &n, key, value) != KV_OK) {                                        \
            out[0] = '\0';                                                                         \
            return MANIFEST_ERR_SPACE;                                                             \
        }                                                                                          \
    } while (0)

    snprintf(num, sizeof(num), "%d", MANIFEST_FORMAT);
    APPEND("format", num);
    APPEND("app", "Puzzle Swap Shop");
    APPEND("app_version", m->app_version);
    APPEND("created", created);
    APPEND("reason", reason);
    snprintf(num, sizeof(num), "%016llx", (unsigned long long)m->title_id);
    APPEND("title_id", num);
    snprintf(num, sizeof(num), "%lu", (unsigned long)m->meet_size);
    APPEND("meet_size", num);
    snprintf(num, sizeof(num), "%08lx", (unsigned long)m->meet_crc);
    APPEND("meet_crc", num);
    snprintf(num, sizeof(num), "%lu", (unsigned long)m->coin_size);
    APPEND("coin_size", num);
    snprintf(num, sizeof(num), "%08lx", (unsigned long)m->coin_crc);
    APPEND("coin_crc", num);
    if (m->has_coins) {
        snprintf(num, sizeof(num), "%u", (unsigned)m->coins);
        APPEND("coins", num);
    }
    if (m->has_purchase) {
        snprintf(num, sizeof(num), "%u", (unsigned)m->panel);
        APPEND("panel", num);
        snprintf(num, sizeof(num), "%u", (unsigned)m->pieces);
        APPEND("pieces", num);
    }
#undef APPEND

    *len = n;
    return MANIFEST_OK;
}

ManifestResult manifest_parse(const char *text, size_t len, Manifest *m)
{
    KvList kv;
    Manifest r;
    memset(&r, 0, sizeof(r));

    if (kv_parse(text, len, &kv) != KV_OK)
        return MANIFEST_ERR_SYNTAX;

    uint32_t format;
    if (!kv_get_u32(&kv, "format", &format))
        return MANIFEST_ERR_MISSING;
    if (format != MANIFEST_FORMAT)
        return MANIFEST_ERR_FORMAT;

    const char *version = kv_get(&kv, "app_version");
    const char *created = kv_get(&kv, "created");
    const char *reason = kv_get(&kv, "reason");
    if (version == NULL || strlen(version) >= sizeof(r.app_version) || created == NULL ||
        !timestamp_parse(created, &r.created) || reason == NULL)
        return MANIFEST_ERR_MISSING;
    strcpy(r.app_version, version);

    r.reason = BACKUP_REASON_COUNT;
    for (unsigned i = 0; i < BACKUP_REASON_COUNT; i++) {
        if (strcmp(reason, reason_names[i]) == 0)
            r.reason = (BackupReason)i;
    }
    if (r.reason == BACKUP_REASON_COUNT)
        return MANIFEST_ERR_MISSING;

    if (!kv_get_hex64(&kv, "title_id", &r.title_id) ||
        !kv_get_u32(&kv, "meet_size", &r.meet_size) ||
        !kv_get_hex32(&kv, "meet_crc", &r.meet_crc) ||
        !kv_get_u32(&kv, "coin_size", &r.coin_size) || !kv_get_hex32(&kv, "coin_crc", &r.coin_crc))
        return MANIFEST_ERR_MISSING;

    uint32_t coins, panel, pieces;
    if (kv_get_u32(&kv, "coins", &coins) && coins <= UINT16_MAX) {
        r.has_coins = true;
        r.coins = (uint16_t)coins;
    }
    if (kv_get_u32(&kv, "panel", &panel) && panel <= UINT8_MAX &&
        kv_get_u32(&kv, "pieces", &pieces) && pieces <= UINT8_MAX) {
        r.has_purchase = true;
        r.panel = (uint8_t)panel;
        r.pieces = (uint8_t)pieces;
    }

    *m = r;
    return MANIFEST_OK;
}
