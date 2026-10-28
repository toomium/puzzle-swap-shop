#include "greatest.h"
#include "core/lang_map.h"

TEST german_console_gets_german(void)
{
    ASSERT_EQ(LANG_DE, lang_map_system(LANG_MAP_SYS_DE));
    PASS();
}

TEST other_consoles_get_english(void)
{
    for (unsigned sys = LANG_MAP_SYS_JP; sys <= LANG_MAP_SYS_TW; sys++) {
        if (sys != LANG_MAP_SYS_DE)
            ASSERT_EQ(LANG_EN, lang_map_system(sys));
    }
    ASSERT_EQ(LANG_EN, lang_map_system(255));
    PASS();
}

TEST fixed_setting_ignores_console(void)
{
    ASSERT_EQ(LANG_DE, lang_map_resolve(LANG_SETTING_DE, LANG_MAP_SYS_EN));
    ASSERT_EQ(LANG_EN, lang_map_resolve(LANG_SETTING_EN, LANG_MAP_SYS_DE));
    PASS();
}

TEST auto_follows_console(void)
{
    ASSERT_EQ(LANG_DE, lang_map_resolve(LANG_SETTING_AUTO, LANG_MAP_SYS_DE));
    ASSERT_EQ(LANG_EN, lang_map_resolve(LANG_SETTING_AUTO, LANG_MAP_SYS_FR));
    ASSERT_EQ(LANG_DE, lang_map_resolve(LANG_SETTING_COUNT, LANG_MAP_SYS_DE));
    PASS();
}

SUITE(lang_map_suite)
{
    RUN_TEST(german_console_gets_german);
    RUN_TEST(other_consoles_get_english);
    RUN_TEST(fixed_setting_ignores_console);
    RUN_TEST(auto_follows_console);
}
