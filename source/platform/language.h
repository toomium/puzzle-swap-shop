#ifndef PSS_PLATFORM_LANGUAGE_H
#define PSS_PLATFORM_LANGUAGE_H

#include <3ds.h>

#include "core/lang_map.h"

/*
 * Reads the console's system language via CFGU and maps it with
 * lang_map_system(). On failure *out is left untouched and the CFGU error is
 * returned, so the caller decides the fallback.
 */
Result language_system(Language *out);

/* The raw CFG_Language number, for the puzzle names in the PNEL files. */
Result language_system_code(unsigned *out);

/* Resolves a settings.ini language value against the console. */
Result language_resolve(LanguageSetting setting, Language *out);

#endif
