#include "core/kvtext.h"

#include <string.h>

static bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

static KvResult copy_trimmed(char *dst, size_t cap, const char *start, const char *end)
{
    while (start < end && is_space(*start))
        start++;
    while (end > start && is_space(end[-1]))
        end--;
    size_t n = (size_t)(end - start);
    if (n >= cap)
        return KV_ERR_TOO_LONG;
    memcpy(dst, start, n);
    dst[n] = '\0';
    return KV_OK;
}

static KvResult parse_line(const char *line, const char *end, KvList *out)
{
    const char *p = line;
    while (p < end && is_space(*p))
        p++;
    if (p == end || *p == '#' || *p == ';')
        return KV_OK;

    /* Cut a trailing "; comment" that follows whitespace. */
    for (const char *c = p; c < end; c++) {
        if (*c == ';' && c > p && is_space(c[-1])) {
            end = c;
            break;
        }
    }

    const char *eq = memchr(p, '=', (size_t)(end - p));
    if (eq == NULL)
        return KV_ERR_SYNTAX;
    if (out->count >= KV_MAX_PAIRS)
        return KV_ERR_TOO_MANY;

    KvPair *pair = &out->pairs[out->count];
    KvResult r = copy_trimmed(pair->key, sizeof(pair->key), p, eq);
    if (r != KV_OK)
        return r;
    if (pair->key[0] == '\0')
        return KV_ERR_SYNTAX;
    r = copy_trimmed(pair->value, sizeof(pair->value), eq + 1, end);
    if (r != KV_OK)
        return r;
    if (kv_get(out, pair->key) != NULL)
        return KV_ERR_DUPLICATE;
    out->count++;
    return KV_OK;
}

KvResult kv_parse(const char *text, size_t len, KvList *out)
{
    out->count = 0;
    if (memchr(text, '\0', len) != NULL)
        return KV_ERR_SYNTAX;

    const char *p = text;
    const char *end = text + len;
    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        const char *line_end = nl ? nl : end;
        const char *content_end = line_end;
        if (content_end > p && content_end[-1] == '\r')
            content_end--;
        KvResult r = parse_line(p, content_end, out);
        if (r != KV_OK)
            return r;
        p = nl ? nl + 1 : end;
    }
    return KV_OK;
}

const char *kv_get(const KvList *list, const char *key)
{
    for (unsigned i = 0; i < list->count; i++) {
        if (strcmp(list->pairs[i].key, key) == 0)
            return list->pairs[i].value;
    }
    return NULL;
}

static bool parse_number(const char *s, unsigned base, unsigned max_digits, uint64_t *out)
{
    if (s == NULL || *s == '\0')
        return false;
    uint64_t v = 0;
    unsigned digits = 0;
    for (; *s; s++) {
        unsigned d;
        if (*s >= '0' && *s <= '9')
            d = (unsigned)(*s - '0');
        else if (base == 16 && *s >= 'a' && *s <= 'f')
            d = (unsigned)(*s - 'a' + 10);
        else if (base == 16 && *s >= 'A' && *s <= 'F')
            d = (unsigned)(*s - 'A' + 10);
        else
            return false;
        if (++digits > max_digits)
            return false;
        v = v * base + d;
    }
    *out = v;
    return true;
}

bool kv_get_u32(const KvList *list, const char *key, uint32_t *out)
{
    uint64_t v;
    if (!parse_number(kv_get(list, key), 10, 10, &v) || v > UINT32_MAX)
        return false;
    *out = (uint32_t)v;
    return true;
}

bool kv_get_hex32(const KvList *list, const char *key, uint32_t *out)
{
    uint64_t v;
    if (!parse_number(kv_get(list, key), 16, 8, &v))
        return false;
    *out = (uint32_t)v;
    return true;
}

bool kv_get_hex64(const KvList *list, const char *key, uint64_t *out)
{
    return parse_number(kv_get(list, key), 16, 16, out);
}

static bool clean_text(const char *s, bool is_key)
{
    size_t n = strlen(s);
    if (n == 0)
        return !is_key;
    if (is_space(s[0]) || is_space(s[n - 1]))
        return false;
    if (is_key && (s[0] == '#' || s[0] == ';'))
        return false;
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '\n' || s[i] == '\r' || (is_key && s[i] == '='))
            return false;
        if (s[i] == ';' && i > 0 && is_space(s[i - 1]))
            return false;
    }
    return true;
}

KvResult kv_append(char *buf, size_t cap, size_t *len, const char *key, const char *value)
{
    if (!clean_text(key, true) || !clean_text(value, false))
        return KV_ERR_SYNTAX;
    size_t klen = strlen(key);
    size_t vlen = strlen(value);
    if (klen >= KV_KEY_MAX || vlen >= KV_VALUE_MAX)
        return KV_ERR_TOO_LONG;
    size_t need = klen + 1 + vlen + 1;
    if (*len + need + 1 > cap)
        return KV_ERR_TOO_LONG;
    char *p = buf + *len;
    memcpy(p, key, klen);
    p[klen] = '=';
    memcpy(p + klen + 1, value, vlen);
    p[klen + 1 + vlen] = '\n';
    p[need] = '\0';
    *len += need;
    return KV_OK;
}
