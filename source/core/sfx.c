#include "core/sfx.h"

static const char *const s_files[SFX_COUNT] = {
    [SFX_MOVE] = "move.wav",       [SFX_SELECT] = "select.wav", [SFX_DESELECT] = "deselect.wav",
    [SFX_CONFIRM] = "confirm.wav", [SFX_BACK] = "back.wav",     [SFX_PAGE] = "page.wav",
    [SFX_BUY] = "buy.wav",         [SFX_COIN] = "coin.wav",     [SFX_COMPLETE] = "complete.wav",
    [SFX_ERROR] = "error.wav",
};

const char *sfx_file_name(Sfx id)
{
    if ((unsigned)id >= SFX_COUNT)
        return 0;
    return s_files[id];
}
