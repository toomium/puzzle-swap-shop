#include "platform/title_detect.h"

#include <stdlib.h>
#include <string.h>

static int console_region(void)
{
    u8 region = 0;
    if (R_FAILED(cfguInit()))
        return -1;
    Result res = CFGU_SecureInfoGetRegion(&region);
    cfguExit();
    return R_SUCCEEDED(res) ? region : -1;
}

TitleStatus title_detect(TitleInfo *out)
{
    memset(out, 0, sizeof(*out));
    out->console_region = console_region();

    Result res = amInit();
    if (R_FAILED(res)) {
        out->error = res;
        return TITLE_ERR_SERVICE;
    }
    u32 count = 0;
    res = AM_GetTitleCount(MEDIATYPE_NAND, &count);
    u64 *ids = NULL;
    u32 read = 0;
    if (R_SUCCEEDED(res) && count > 0) {
        ids = malloc(count * sizeof(*ids));
        if (ids == NULL)
            res = MAKERESULT(RL_PERMANENT, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY);
        else
            res = AM_GetTitleList(&read, MEDIATYPE_NAND, count, ids);
    }
    amExit();
    if (R_FAILED(res)) {
        free(ids);
        out->error = res;
        return TITLE_ERR_SERVICE;
    }
    out->title = mii_plaza_find(ids, read, out->console_region);
    free(ids);
    return out->title != NULL ? TITLE_FOUND : TITLE_NOT_INSTALLED;
}
