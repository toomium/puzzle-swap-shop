#ifndef PSS_CORE_KVTEXT_H
#define PSS_CORE_KVTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * "key=value" text files (manifest.txt, journal.txt, settings.ini).
 * One pair per line, LF or CRLF. Blank lines and lines starting with '#' or
 * ';' are ignored, as is a ';' comment that follows whitespace. Spaces around
 * keys and values are trimmed.
 */

#define KV_KEY_MAX   32
#define KV_VALUE_MAX 96
#define KV_MAX_PAIRS 32

typedef struct {
    char key[KV_KEY_MAX];
    char value[KV_VALUE_MAX];
} KvPair;

typedef struct {
    KvPair pairs[KV_MAX_PAIRS];
    unsigned count;
} KvList;

typedef enum {
    KV_OK = 0,
    KV_ERR_SYNTAX,    /* line without '=', empty key, or a NUL byte */
    KV_ERR_TOO_LONG,  /* key or value does not fit */
    KV_ERR_TOO_MANY,  /* more than KV_MAX_PAIRS pairs */
    KV_ERR_DUPLICATE, /* key appears twice */
} KvResult;

KvResult kv_parse(const char *text, size_t len, KvList *out);

/* NULL if the key is missing. */
const char *kv_get(const KvList *list, const char *key);

/* Strict number parsing of a whole value; false if missing or malformed. */
bool kv_get_u32(const KvList *list, const char *key, uint32_t *out);
bool kv_get_hex32(const KvList *list, const char *key, uint32_t *out);
bool kv_get_hex64(const KvList *list, const char *key, uint64_t *out);

/*
 * Appends "key=value\n" to buf (NUL-terminated, *len bytes used). Rejects
 * keys or values containing '=', ';', '#' at the start, line breaks, or text
 * that would not read back unchanged. On error buf is unchanged.
 */
KvResult kv_append(char *buf, size_t cap, size_t *len, const char *key, const char *value);

#endif
