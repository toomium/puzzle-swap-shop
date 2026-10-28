#include "core/lang_map.h"

Language lang_map_system(unsigned system_language)
{
    return system_language == LANG_MAP_SYS_DE ? LANG_DE : LANG_EN;
}

Language lang_map_resolve(LanguageSetting setting, unsigned system_language)
{
    switch (setting) {
    case LANG_SETTING_DE:
        return LANG_DE;
    case LANG_SETTING_EN:
        return LANG_EN;
    default:
        return lang_map_system(system_language);
    }
}
