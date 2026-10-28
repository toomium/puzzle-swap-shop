#ifndef PSS_APP_PNEL_LOAD_H
#define PSS_APP_PNEL_LOAD_H

#include <stddef.h>
#include <stdint.h>

#include "core/pnel_index.h"
#include "platform/storage.h"

/*
 * Finds the PNEL files of the SpotPass puzzles and reads their
 * headers, never their pictures. Where the files lie is still open:
 * the loader does not rely on a file name but
 * checks every file in the BOSS archive for the magic "PNEL", so it works
 * wherever the files turn out to be, and finds nothing if they are not there.
 * Read only.
 *
 * On the author's console the only file in the BOSS archive is
 * 26 MB and does not start with "PNEL", so a file is searched for the magic at
 * any offset; the offsets inside the entries are taken as relative to where
 * the magic was found (hypothesis).
 */

#define PNEL_MAX_FILES 4
#define PNEL_PATH_MAX  96

typedef enum {
    PNEL_LOAD_NO_ARCHIVE = 0, /* console gives no access to the SpotPass archive */
    PNEL_LOAD_NO_FILE,        /* archive readable, but no valid PNEL file in it */
    PNEL_LOAD_OK,
} PnelLoadResult;

typedef struct {
    PnelIndex index;
    char path[PNEL_MAX_FILES][PNEL_PATH_MAX];
    uint64_t base[PNEL_MAX_FILES]; /* where the PNEL data starts inside the file */
    unsigned files;
    unsigned candidates; /* files looked at, for the diagnosis build */
    unsigned scanned;    /* files that had to be searched for the magic */
} PnelSet;

/*
 * `system_language` is the console language as a CFG_Language number; names
 * fall back to English (1) when the console language has none.
 */
PnelLoadResult pnel_load(const Storage *s, unsigned system_language, PnelSet *out);

/* Reads the picture block (CGFX) of a SpotPass panel; STORE_ERR_NOT_FOUND if the set has none. */
StoreResult pnel_read_image(const Storage *s, const PnelSet *set, unsigned panel, uint8_t *buf,
                            size_t cap, size_t *len);

#endif
