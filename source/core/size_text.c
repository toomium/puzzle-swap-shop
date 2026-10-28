#include "core/size_text.h"

#include <stdio.h>

int size_text_format(char *out, size_t size, uint32_t bytes)
{
    const uint32_t kb = 1024u, mb = 1024u * 1024u;
    if (bytes < kb)
        return snprintf(out, size, "%u B", (unsigned)bytes);
    if (bytes < mb)
        return snprintf(out, size, "%u KB", (unsigned)((bytes + kb - 1) / kb));
    return snprintf(out, size, "%u MB", (unsigned)((bytes + mb - 1) / mb));
}
