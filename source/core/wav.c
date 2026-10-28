#include "core/wav.h"

#include <string.h>

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

WavResult wav_parse(const uint8_t *buf, size_t len, WavInfo *info)
{
    if (!buf || !info || len < 12 || memcmp(buf, "RIFF", 4) != 0 || memcmp(buf + 8, "WAVE", 4) != 0)
        return WAV_ERR_HEADER;

    /* The RIFF size may be wrong in hand-made files; trust the buffer, not the header. */
    size_t pos = 12;
    int have_fmt = 0;
    uint16_t format = 0, channels = 0, bits = 0, block_align = 0;
    uint32_t rate = 0;

    while (pos + 8 <= len) {
        const uint8_t *chunk = buf + pos;
        uint32_t size = rd32(chunk + 4);
        size_t body = pos + 8;
        if (size > len - body)
            return WAV_ERR_HEADER;

        if (memcmp(chunk, "fmt ", 4) == 0) {
            if (size < 16)
                return WAV_ERR_HEADER;
            format = rd16(buf + body);
            channels = rd16(buf + body + 2);
            rate = rd32(buf + body + 4);
            block_align = rd16(buf + body + 12);
            bits = rd16(buf + body + 14);
            have_fmt = 1;
        } else if (memcmp(chunk, "data", 4) == 0) {
            if (!have_fmt || size == 0)
                return WAV_ERR_NO_DATA;
            if (format != 1 || bits != 16 || (channels != 1 && channels != 2) || rate == 0 ||
                block_align != channels * 2)
                return WAV_ERR_FORMAT;
            info->channels = channels;
            info->sample_rate = rate;
            info->data_offset = body;
            info->data_size = size - size % block_align;
            info->frames = (uint32_t)(info->data_size / block_align);
            return info->frames ? WAV_OK : WAV_ERR_NO_DATA;
        }

        /* Chunks are padded to even sizes. */
        pos = body + size + (size & 1u);
    }
    return WAV_ERR_NO_DATA;
}

uint32_t wav_duration_ms(const WavInfo *info)
{
    if (!info || info->sample_rate == 0)
        return 0;
    return (uint32_t)(((uint64_t)info->frames * 1000u + info->sample_rate - 1) / info->sample_rate);
}
