#include "greatest.h"
#include "core/settings_menu.h"
#include "core/strings.h"

TEST rows_stop_at_the_ends(void)
{
    int row = SETTINGS_ROW_SOUND;
    ASSERT_FALSE(settings_menu_move(&row, -1));
    ASSERT_EQ(SETTINGS_ROW_SOUND, row);
    ASSERT(settings_menu_move(&row, 1));
    ASSERT_EQ(SETTINGS_ROW_LANGUAGE, row);
    ASSERT_FALSE(settings_menu_move(&row, 1));
    ASSERT_EQ(SETTINGS_ROW_LANGUAGE, row);
    ASSERT(settings_menu_move(&row, -1));
    ASSERT_EQ(SETTINGS_ROW_SOUND, row);
    PASS();
}

TEST language_cycles_both_ways(void)
{
    ASSERT_EQ(SETTINGS_LANG_DE, settings_menu_cycle_language(SETTINGS_LANG_AUTO, 1));
    ASSERT_EQ(SETTINGS_LANG_EN, settings_menu_cycle_language(SETTINGS_LANG_DE, 1));
    ASSERT_EQ(SETTINGS_LANG_AUTO, settings_menu_cycle_language(SETTINGS_LANG_EN, 1));
    ASSERT_EQ(SETTINGS_LANG_EN, settings_menu_cycle_language(SETTINGS_LANG_AUTO, -1));
    ASSERT_EQ(SETTINGS_LANG_AUTO, settings_menu_cycle_language(SETTINGS_LANG_DE, -1));
    PASS();
}

TEST language_labels(void)
{
    ASSERT_STR_EQ("Automatisch",
                  str_lang(LANG_DE, settings_menu_language_label(SETTINGS_LANG_AUTO)));
    ASSERT_STR_EQ("Automatic", str_lang(LANG_EN, settings_menu_language_label(SETTINGS_LANG_AUTO)));
    /* The names of the languages read the same in every language. */
    ASSERT_STR_EQ("Deutsch", str_lang(LANG_EN, settings_menu_language_label(SETTINGS_LANG_DE)));
    ASSERT_STR_EQ("English", str_lang(LANG_DE, settings_menu_language_label(SETTINGS_LANG_EN)));
    PASS();
}

SUITE(settings_menu_suite)
{
    RUN_TEST(rows_stop_at_the_ends);
    RUN_TEST(language_cycles_both_ways);
    RUN_TEST(language_labels);
}
