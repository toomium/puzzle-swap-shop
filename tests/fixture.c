#include "fixture.h"

#include <stdio.h>
#include <stdlib.h>

uint8_t *fixture_load_path(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(size > 0 ? (size_t)size : 1);
    if (buf && fread(buf, 1, (size_t)size, f) != (size_t)size) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    if (buf)
        *len = (size_t)size;
    return buf;
}

uint8_t *fixture_load(const char *name, size_t *len)
{
    char path[256];
    snprintf(path, sizeof(path), "tests/fixtures/%s", name);
    uint8_t *buf = fixture_load_path(path, len);
    if (!buf)
        fprintf(stderr, "missing fixture %s; run tools/make_fixture.py\n", path);
    return buf;
}

void fixture_free(uint8_t *buf)
{
    free(buf);
}
