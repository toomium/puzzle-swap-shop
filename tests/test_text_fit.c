#include <string.h>

#include "greatest.h"
#include "core/text_fit.h"

/* Every character is 5 px wide, whatever its encoded length. */
static int measure_chars(const char *s, size_t len, void *ctx)
{
    int *calls = ctx;
    if (calls)
        (*calls)++;
    int w = 0;
    for (size_t i = 0; i < len;) {
        i += text_utf8_len((unsigned char)s[i]);
        w += 5;
    }
    return w;
}

TEST fits_whole(void)
{
    bool cut = true;
    ASSERT_EQ(5u, text_fit("Wiese", 25, 5, measure_chars, NULL, &cut));
    ASSERT_FALSE(cut);
    ASSERT_EQ(0u, text_fit("", 0, 5, measure_chars, NULL, &cut));
    ASSERT_FALSE(cut);
    PASS();
}

TEST cuts_with_room_for_ellipsis(void)
{
    bool cut = false;
    /* 10 chars = 50 px into 30 px: 5 chars + ellipsis (5) would be 30, so 5 chars. */
    ASSERT_EQ(5u, text_fit("Sommerwies", 30, 5, measure_chars, NULL, &cut));
    ASSERT(cut);
    PASS();
}

TEST cuts_on_character_boundary(void)
{
    bool cut = false;
    /* "Größe" has two-byte umlauts; 3 chars fit: G r ö = 4 bytes. */
    const char *s = "Gr\xC3\xB6\xC3\x9F"
                    "e im Gl\xC3\xBC"
                    "ck";
    ASSERT_EQ(4u, text_fit(s, 20, 5, measure_chars, NULL, &cut));
    ASSERT(cut);
    PASS();
}

TEST drops_trailing_spaces(void)
{
    bool cut = false;
    /* Best prefix "Neue " is trimmed to "Neue". */
    ASSERT_EQ(4u, text_fit("Neue Teile", 30, 5, measure_chars, NULL, &cut));
    ASSERT(cut);
    PASS();
}

TEST nothing_fits(void)
{
    bool cut = false;
    ASSERT_EQ(0u, text_fit("Sommerwiese", 7, 5, measure_chars, NULL, &cut));
    ASSERT(cut);
    PASS();
}

TEST few_measurements(void)
{
    char s[201];
    memset(s, 'a', 200);
    s[200] = '\0';
    int calls = 0;
    bool cut = false;
    ASSERT_EQ(19u, text_fit(s, 100, 5, measure_chars, &calls, &cut));
    ASSERT(calls <= 10);
    PASS();
}

TEST utf8_lengths(void)
{
    ASSERT_EQ(1u, text_utf8_len('a'));
    ASSERT_EQ(2u, text_utf8_len(0xC3));
    ASSERT_EQ(3u, text_utf8_len(0xE2));
    ASSERT_EQ(3u, text_utf8_len(0xEE)); /* button glyphs U+E000.. */
    ASSERT_EQ(4u, text_utf8_len(0xF0));
    ASSERT_EQ(1u, text_utf8_len(0x80));
    ASSERT_EQ(1u, text_utf8_len(0xFF));
    PASS();
}

TEST truncated_sequence_at_end(void)
{
    bool cut = false;
    /* A lone lead byte at the end must not run past the terminator. */
    ASSERT_EQ(2u, text_fit("ab\xE2", 10, 0, measure_chars, NULL, &cut));
    ASSERT(cut);
    PASS();
}

static bool line_is(const char *s, TextLine l, const char *expected)
{
    return l.len == strlen(expected) && strncmp(s + l.start, expected, l.len) == 0;
}

TEST wrap_at_spaces(void)
{
    const char *s = "Diese 3 Teile f\xC3\xBCr 6 Spielm\xC3\xBCnzen kaufen?";
    TextLine lines[4];
    bool cut = true;
    /* 20 characters per line. */
    int n = text_wrap(s, 100, measure_chars, NULL, lines, 4, &cut);
    ASSERT_EQ(2, n);
    ASSERT_FALSE(cut);
    ASSERT(line_is(s, lines[0], "Diese 3 Teile f\xC3\xBCr 6"));
    ASSERT(line_is(s, lines[1], "Spielm\xC3\xBCnzen kaufen?"));
    PASS();
}

TEST wrap_single_line(void)
{
    TextLine lines[2];
    bool cut = true;
    ASSERT_EQ(1, text_wrap("  OK", 100, measure_chars, NULL, lines, 2, &cut));
    ASSERT(line_is("  OK", lines[0], "OK"));
    ASSERT_FALSE(cut);
    ASSERT_EQ(0, text_wrap("   ", 100, measure_chars, NULL, lines, 2, &cut));
    PASS();
}

TEST wrap_breaks_long_word(void)
{
    const char *s = "Spielm\xC3\xBCnzenstand";
    TextLine lines[3];
    bool cut = true;
    /* 6 characters per line; the umlaut stays whole. */
    int n = text_wrap(s, 30, measure_chars, NULL, lines, 3, &cut);
    ASSERT_EQ(3, n);
    ASSERT_FALSE(cut);
    ASSERT(line_is(s, lines[0], "Spielm"));
    ASSERT(line_is(s, lines[1], "\xC3\xBCnzens"));
    ASSERT(line_is(s, lines[2], "tand"));
    PASS();
}

TEST wrap_newline(void)
{
    const char *s = "Eins\n\nDrei vier";
    TextLine lines[4];
    bool cut = true;
    int n = text_wrap(s, 100, measure_chars, NULL, lines, 4, &cut);
    ASSERT_EQ(3, n);
    ASSERT(line_is(s, lines[0], "Eins"));
    ASSERT(line_is(s, lines[1], ""));
    ASSERT(line_is(s, lines[2], "Drei vier"));
    ASSERT_FALSE(cut);
    PASS();
}

TEST wrap_too_many_lines(void)
{
    const char *s = "eins zwei drei vier";
    TextLine lines[2];
    bool cut = false;
    int n = text_wrap(s, 25, measure_chars, NULL, lines, 2, &cut);
    ASSERT_EQ(2, n);
    ASSERT(cut);
    ASSERT(line_is(s, lines[0], "eins"));
    /* The last line holds the rest, to be shortened by the caller. */
    ASSERT(line_is(s, lines[1], "zwei drei vier"));
    PASS();
}

SUITE(text_fit_suite)
{
    RUN_TEST(fits_whole);
    RUN_TEST(cuts_with_room_for_ellipsis);
    RUN_TEST(cuts_on_character_boundary);
    RUN_TEST(drops_trailing_spaces);
    RUN_TEST(nothing_fits);
    RUN_TEST(few_measurements);
    RUN_TEST(utf8_lengths);
    RUN_TEST(truncated_sequence_at_end);
    RUN_TEST(wrap_at_spaces);
    RUN_TEST(wrap_single_line);
    RUN_TEST(wrap_breaks_long_word);
    RUN_TEST(wrap_newline);
    RUN_TEST(wrap_too_many_lines);
}
