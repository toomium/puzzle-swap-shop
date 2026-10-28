#include "app/boot.h"

#include <string.h>

#include "core/version.h"
#include "platform/clock.h"
#include "platform/language.h"

BootResult app_boot(AppSystem *sys, Model *m)
{
    memset(sys, 0, sizeof(*sys));
    memset(m, 0, sizeof(*m));

    TitleStatus ts = title_detect(&sys->title);
    if (ts == TITLE_NOT_INSTALLED)
        return BOOT_ERR_NOT_INSTALLED;
    if (ts != TITLE_FOUND)
        return BOOT_ERR_ACCESS;
    if (fs_archives_init(&sys->fs, sys->title.title) != STORE_OK)
        return BOOT_ERR_ACCESS;
    sys->fs_open = true;
    fs_archives_storage(&sys->fs, &sys->storage);

    unsigned system_language = LANG_MAP_SYS_EN; /* names fall back to English */
    language_system_code(&system_language);
    ModelEnv env = {&sys->storage, sys->title.title, clock_now, version_string(), system_language};
    model_load(m, &env);
    return BOOT_OK;
}

void app_shutdown(AppSystem *sys, Model *m)
{
    model_free(m);
    if (sys->fs_open)
        fs_archives_exit(&sys->fs);
    sys->fs_open = false;
}
