#include <string.h>

#include "greatest.h"
#include "core/settings.h"

static bool parse(const char *text, Settings *s, unsigned *bad)
{
    return settings_parse(text, strlen(text), s, bad);
}

TEST defaults_and_example_file(void)
{
    Settings s;
    unsigned bad;
    Settings d = settings_default();
    ASSERT_EQ(5, d.prices.per_piece);
    ASSERT_EQ(10, d.prices.per_rare_piece);
    ASSERT(d.sound);
    ASSERT_EQ(SETTINGS_LANG_AUTO, d.language);

    /* The example settings file. */
    ASSERT(parse("price_per_piece=5\n"
                 "price_rare_piece=10    ; 0 = seltene Teile nicht verkaufen\n"
                 "sound=on\n"
                 "language=auto          ; auto | de | en\n",
                 &s, &bad));
    ASSERT_EQ(0, bad);
    ASSERT_EQ(0, memcmp(&d, &s, sizeof(s)));
    PASS();
}

TEST values_and_bad_values(void)
{
    Settings s;
    unsigned bad;
    ASSERT(parse("price_per_piece=3\nprice_rare_piece=0\nsound=off\nlanguage=en\nother=1\n", &s,
                 &bad));
    ASSERT_EQ(0, bad);
    ASSERT_EQ(3, s.prices.per_piece);
    ASSERT_EQ(0, s.prices.per_rare_piece);
    ASSERT_FALSE(s.sound);
    ASSERT_EQ(SETTINGS_LANG_EN, s.language);

    ASSERT(parse("price_per_piece=0\nprice_rare_piece=301\nsound=yes\nlanguage=fr\n", &s, &bad));
    ASSERT_EQ(4, bad);
    Settings d = settings_default();
    ASSERT_EQ(0, memcmp(&d, &s, sizeof(s)));

    ASSERT_FALSE(parse("not a settings file", &s, NULL));
    ASSERT_EQ(0, memcmp(&d, &s, sizeof(s)));
    PASS();
}

TEST format_round_trips(void)
{
    char text[SETTINGS_TEXT_MAX];
    size_t len;
    Settings s = settings_default(), back;
    s.prices.per_piece = 5;
    s.sound = false;
    s.language = SETTINGS_LANG_DE;
    ASSERT(settings_format(&s, text, sizeof(text), &len));
    ASSERT_EQ(strlen(text), len);
    unsigned bad;
    ASSERT(settings_parse(text, len, &back, &bad));
    ASSERT_EQ(0, bad);
    ASSERT_EQ(0, memcmp(&s, &back, sizeof(s)));
    ASSERT_FALSE(settings_format(&s, text, 20, &len));
    PASS();
}

SUITE(settings_suite)
{
    RUN_TEST(defaults_and_example_file);
    RUN_TEST(values_and_bad_values);
    RUN_TEST(format_round_trips);
}
