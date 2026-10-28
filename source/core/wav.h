#ifndef PSS_CORE_WAV_H
#define PSS_CORE_WAV_H

#include <stddef.h>
#include <stdint.h>

/*
 * Minimal RIFF/WAVE reader for the sound effects in romfs/sfx/: 16-bit PCM,
 * mono or stereo. Other formats are rejected rather than played wrong.
 */

typedef enum {
    WAV_OK = 0,
    WAV_ERR_HEADER,  /* not a RIFF/WAVE file, or truncated */
    WAV_ERR_FORMAT,  /* not 16-bit PCM with 1 or 2 channels */
    WAV_ERR_NO_DATA, /* fmt or data chunk missing or empty */
} WavResult;

typedef struct {
    uint16_t channels;
    uint32_t sample_rate;
    size_t data_offset; /* byte offset of the first sample in the buffer */
    size_t data_size;   /* bytes of sample data */
    uint32_t frames;    /* samples per channel */
} WavInfo;

WavResult wav_parse(const uint8_t *buf, size_t len, WavInfo *info);

/* Length in milliseconds, rounded up. */
uint32_t wav_duration_ms(const WavInfo *info);

#endif
