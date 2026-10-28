#include <string.h>

#include "greatest.h"
#include "core/strings.h"

static void reset_language(void *arg)
{
    (void)arg;
    strings_set_language(LANG_DE);
}

TEST every_german_string_present(void)
{
    for (int id = 0; id < STR_COUNT; id++) {
        const char *s = str_lang(LANG_DE, (StringId)id);
        ASSERT(s != NULL);
        ASSERT(s[0] != '\0');
    }
    PASS();
}

TEST every_english_string_present(void)
{
    for (int id = 0; id < STR_COUNT; id++) {
        const char *s = str_lang(LANG_EN, (StringId)id);
        ASSERTm("English text missing", s != NULL);
        ASSERT(s[0] != '\0');
    }
    PASS();
}

TEST english_used_when_selected(void)
{
    strings_set_language(LANG_EN);
    ASSERT_EQ(LANG_EN, strings_language());
    ASSERT_STR_EQ("Puzzle Swap Shop", str(STR_APP_NAME));
    ASSERT_STR_EQ("Buy", str(STR_BTN_BUY));
    strings_set_language(LANG_DE);
    ASSERT_STR_EQ("Kaufen", str(STR_BTN_BUY));
    PASS();
}

/* Writes the conversion characters of a printf format, e.g. "%u von %u" -> "uu". */
static void format_signature(const char *fmt, char *out, size_t size)
{
    size_t n = 0;
    for (const char *p = fmt; *p; p++) {
        if (*p != '%')
            continue;
        p++;
        if (*p == '\0')
            break;
        if (n + 1 < size)
            out[n++] = *p;
    }
    out[n] = '\0';
}

TEST same_placeholders_in_every_language(void)
{
    for (int id = 0; id < STR_COUNT; id++) {
        char de[16], en[16];
        format_signature(str_lang(LANG_DE, (StringId)id), de, sizeof(de));
        format_signature(str_lang(LANG_EN, (StringId)id), en, sizeof(en));
        ASSERT_STR_EQm(str_lang(LANG_EN, (StringId)id), de, en);
    }
    PASS();
}

TEST english_follows_text_rules(void)
{
    for (int id = 0; id < STR_COUNT; id++) {
        const char *s = str_lang(LANG_EN, (StringId)id);
        /* Exclamation marks only on the success screen. */
        if (id != STR_NEW_PIECES_BANNER && id != STR_BOUGHT_HEADLINE && id != STR_PUZZLE_COMPLETE)
            ASSERTm(s, strchr(s, '!') == NULL);
        ASSERT(strstr(s, "TODO") == NULL);
        ASSERT(strstr(s, "Lorem") == NULL);
        ASSERT(strstr(s, "coins") == NULL);
    }
    PASS();
}

TEST plurals(void)
{
    char buf[32];
    strings_pieces(buf, sizeof(buf), 1);
    ASSERT_STR_EQ("1 Teil", buf);
    strings_pieces(buf, sizeof(buf), 3);
    ASSERT_STR_EQ("3 Teile", buf);
    strings_new_pieces(buf, sizeof(buf), 1);
    ASSERT_STR_EQ("1 neues Teil", buf);
    strings_new_pieces(buf, sizeof(buf), 3);
    ASSERT_STR_EQ("3 neue Teile", buf);
    strings_coins(buf, sizeof(buf), 1);
    ASSERT_STR_EQ("1 Spielmünze", buf);
    strings_coins(buf, sizeof(buf), 6);
    ASSERT_STR_EQ("6 Spielmünzen", buf);
    PASS();
}

TEST confirm_sentence(void)
{
    char pieces[32], coins[32], line[96];
    strings_pieces(pieces, sizeof(pieces), 3);
    strings_coins(coins, sizeof(coins), 6);
    snprintf(line, sizeof(line), str(STR_CONFIRM_QUESTION), pieces, coins);
    ASSERT_STR_EQ("Diese 3 Teile für 6 Spielmünzen kaufen?", line);
    PASS();
}

TEST english_plurals(void)
{
    char buf[32];
    strings_set_language(LANG_EN);
    strings_pieces(buf, sizeof(buf), 1);
    ASSERT_STR_EQ("1 piece", buf);
    strings_pieces(buf, sizeof(buf), 3);
    ASSERT_STR_EQ("3 pieces", buf);
    strings_new_pieces(buf, sizeof(buf), 1);
    ASSERT_STR_EQ("1 new piece", buf);
    strings_new_pieces(buf, sizeof(buf), 3);
    ASSERT_STR_EQ("3 new pieces", buf);
    strings_coins(buf, sizeof(buf), 1);
    ASSERT_STR_EQ("1 Play Coin", buf);
    strings_coins(buf, sizeof(buf), 6);
    ASSERT_STR_EQ("6 Play Coins", buf);
    PASS();
}

TEST english_sentences(void)
{
    char pieces[32], coins[32], line[96];
    strings_set_language(LANG_EN);

    strings_pieces(pieces, sizeof(pieces), 1);
    strings_coins(coins, sizeof(coins), 2);
    snprintf(line, sizeof(line), str(STR_CONFIRM_QUESTION), pieces, coins);
    ASSERT_STR_EQ("Buy 1 piece for 2 Play Coins?", line);

    strings_new_pieces(pieces, sizeof(pieces), 3);
    snprintf(line, sizeof(line), str(STR_BOUGHT_HEADLINE), pieces);
    ASSERT_STR_EQ("You got 3 new pieces!", line);

    strings_coins(coins, sizeof(coins), 114);
    snprintf(line, sizeof(line), str(STR_COINS_LEFT), coins);
    ASSERT_STR_EQ("You have 114 Play Coins left.", line);
    PASS();
}

TEST no_placeholders_left(void)
{
    for (int id = 0; id < STR_COUNT; id++) {
        const char *s = str_lang(LANG_DE, (StringId)id);
        ASSERT(strstr(s, "TODO") == NULL);
        ASSERT(strstr(s, "Lorem") == NULL);
    }
    PASS();
}

SUITE(strings_suite)
{
    SET_TEARDOWN(reset_language, NULL);
    RUN_TEST(every_german_string_present);
    RUN_TEST(every_english_string_present);
    RUN_TEST(english_used_when_selected);
    RUN_TEST(plurals);
    RUN_TEST(confirm_sentence);
    RUN_TEST(english_plurals);
    RUN_TEST(english_sentences);
    RUN_TEST(no_placeholders_left);
    RUN_TEST(same_placeholders_in_every_language);
    RUN_TEST(english_follows_text_rules);
}
