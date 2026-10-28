#include "platform/language.h"

Result language_system(Language *out)
{
    Result rc = cfguInit();
    if (R_FAILED(rc))
        return rc;

    u8 system_language = 0;
    rc = CFGU_GetSystemLanguage(&system_language);
    cfguExit();
    if (R_FAILED(rc))
        return rc;

    *out = lang_map_system(system_language);
    return 0;
}

Result language_system_code(unsigned *out)
{
    Result rc = cfguInit();
    if (R_FAILED(rc))
        return rc;

    u8 system_language = 0;
    rc = CFGU_GetSystemLanguage(&system_language);
    cfguExit();
    if (R_SUCCEEDED(rc))
        *out = system_language;
    return rc;
}

Result language_resolve(LanguageSetting setting, Language *out)
{
    if (setting == LANG_SETTING_DE || setting == LANG_SETTING_EN) {
        *out = lang_map_resolve(setting, 0);
        return 0;
    }

    Language system = LANG_DE;
    Result rc = language_system(&system);
    if (R_FAILED(rc))
        return rc;
    *out = system;
    return 0;
}
