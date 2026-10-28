#ifndef PSS_APP_BOOT_H
#define PSS_APP_BOOT_H

#include "app/model.h"
#include "platform/fs_archives.h"
#include "platform/title_detect.h"

/* S0 on the console: find Mii Plaza, open the SD card, load the model. */

typedef enum {
    BOOT_OK = 0,
    BOOT_ERR_ACCESS,        /* E-01: title list or SD card not usable */
    BOOT_ERR_NOT_INSTALLED, /* E-02 */
} BootResult;

typedef struct {
    TitleInfo title;
    FsArchives fs;
    Storage storage;
    bool fs_open;
} AppSystem;

/* On BOOT_OK the model is loaded; its status tells which screen comes next. */
BootResult app_boot(AppSystem *sys, Model *m);

void app_shutdown(AppSystem *sys, Model *m);

#endif
