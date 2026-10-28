#include <string.h>

#include "greatest.h"
#include "core/kvtext.h"

static KvResult parse(const char *text, KvList *out)
{
    return kv_parse(text, strlen(text), out);
}

TEST parses_pairs_comments_and_crlf(void)
{
    KvList kv;
    const char *text = "# comment\r\n"
                       "price_per_piece = 2\r\n"
                       "\n"
                       "price_rare_piece=10    ; 0 = never\n"
                       "; another comment\n"
                       "path=backups/a;b\n"
                       "empty=\n"
                       "last=1";
    ASSERT_EQ(KV_OK, parse(text, &kv));
    ASSERT_EQ(5, kv.count);
    ASSERT_STR_EQ("2", kv_get(&kv, "price_per_piece"));
    ASSERT_STR_EQ("10", kv_get(&kv, "price_rare_piece"));
    ASSERT_STR_EQ("backups/a;b", kv_get(&kv, "path"));
    ASSERT_STR_EQ("", kv_get(&kv, "empty"));
    ASSERT_STR_EQ("1", kv_get(&kv, "last"));
    ASSERT_EQ(NULL, kv_get(&kv, "missing"));
    PASS();
}

TEST rejects_bad_input(void)
{
    KvList kv;
    ASSERT_EQ(KV_ERR_SYNTAX, parse("no equals sign\n", &kv));
    ASSERT_EQ(KV_ERR_SYNTAX, parse("=value\n", &kv));
    ASSERT_EQ(KV_ERR_DUPLICATE, parse("a=1\na=2\n", &kv));
    ASSERT_EQ(KV_ERR_TOO_LONG, parse("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa=1\n", &kv));
    ASSERT_EQ(KV_ERR_SYNTAX, kv_parse("a=1\0b=2", 7, &kv));

    char many[KV_MAX_PAIRS * 8 + 16] = "";
    for (int i = 0; i <= KV_MAX_PAIRS; i++) {
        char line[16];
        snprintf(line, sizeof(line), "k%d=1\n", i);
        strcat(many, line);
    }
    ASSERT_EQ(KV_ERR_TOO_MANY, parse(many, &kv));
    PASS();
}

TEST numbers_are_strict(void)
{
    KvList kv;
    uint32_t u;
    uint64_t q;
    ASSERT_EQ(KV_OK, parse("a=42\nb=4x\nc=-1\nd=ff00AA11\ne=123456789\n"
                           "f=0004001000022800\ng=4294967296\nh=\n",
                           &kv));
    ASSERT(kv_get_u32(&kv, "a", &u));
    ASSERT_EQ(42u, u);
    ASSERT_FALSE(kv_get_u32(&kv, "b", &u));
    ASSERT_FALSE(kv_get_u32(&kv, "c", &u));
    ASSERT_FALSE(kv_get_u32(&kv, "g", &u));
    ASSERT_FALSE(kv_get_u32(&kv, "h", &u));
    ASSERT_FALSE(kv_get_u32(&kv, "missing", &u));
    ASSERT(kv_get_hex32(&kv, "d", &u));
    ASSERT_EQ(0xff00aa11u, u);
    ASSERT_FALSE(kv_get_hex32(&kv, "e", &u)); /* nine digits */
    ASSERT(kv_get_hex64(&kv, "f", &q));
    ASSERT_EQ(0x0004001000022800ull, q);
    PASS();
}

TEST append_round_trips(void)
{
    char buf[64];
    size_t len = 0;
    KvList kv;
    buf[0] = '\0';
    ASSERT_EQ(KV_OK, kv_append(buf, sizeof(buf), &len, "a", "x=y"));
    ASSERT_EQ(KV_OK, kv_append(buf, sizeof(buf), &len, "b", ""));
    ASSERT_EQ(KV_OK, kv_append(buf, sizeof(buf), &len, "c", "2026-10-26 19:24:00"));
    ASSERT_STR_EQ("a=x=y\nb=\nc=2026-10-26 19:24:00\n", buf);
    ASSERT_EQ(strlen(buf), len);
    ASSERT_EQ(KV_OK, kv_parse(buf, len, &kv));
    ASSERT_STR_EQ("x=y", kv_get(&kv, "a"));
    ASSERT_STR_EQ("2026-10-26 19:24:00", kv_get(&kv, "c"));
    PASS();
}

TEST append_refuses_what_would_not_read_back(void)
{
    char buf[32] = "";
    size_t len = 0;
    ASSERT_EQ(KV_ERR_SYNTAX, kv_append(buf, sizeof(buf), &len, "a=b", "1"));
    ASSERT_EQ(KV_ERR_SYNTAX, kv_append(buf, sizeof(buf), &len, "#a", "1"));
    ASSERT_EQ(KV_ERR_SYNTAX, kv_append(buf, sizeof(buf), &len, "", "1"));
    ASSERT_EQ(KV_ERR_SYNTAX, kv_append(buf, sizeof(buf), &len, "a", "line\nbreak"));
    ASSERT_EQ(KV_ERR_SYNTAX, kv_append(buf, sizeof(buf), &len, "a", " padded"));
    ASSERT_EQ(KV_ERR_SYNTAX, kv_append(buf, sizeof(buf), &len, "a", "x ; comment"));
    ASSERT_EQ(KV_ERR_TOO_LONG,
              kv_append(buf, sizeof(buf), &len, "a", "0123456789012345678901234567890"));
    ASSERT_EQ(0, len);
    ASSERT_STR_EQ("", buf);
    PASS();
}

SUITE(kvtext_suite)
{
    RUN_TEST(parses_pairs_comments_and_crlf);
    RUN_TEST(rejects_bad_input);
    RUN_TEST(numbers_are_strict);
    RUN_TEST(append_round_trips);
    RUN_TEST(append_refuses_what_would_not_read_back);
}
