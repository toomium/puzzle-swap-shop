#include <stdio.h>
#include <string.h>

#include "greatest.h"
#include "fixture.h"
#include "core/sfx.h"
#include "core/wav.h"

/* Builds a minimal WAV header in buf; returns the total length. */
static size_t make_wav(uint8_t *buf, uint16_t format, uint16_t channels, uint32_t rate,
                       uint16_t bits, uint32_t data_bytes, int extra_chunk)
{
    uint8_t *p = buf;
#define PUT32(v)                                                                                   \
    do {                                                                                           \
        uint32_t x_ = (v);                                                                         \
        *p++ = x_ & 0xFF;                                                                          \
        *p++ = (x_ >> 8) & 0xFF;                                                                   \
        *p++ = (x_ >> 16) & 0xFF;                                                                  \
        *p++ = (x_ >> 24) & 0xFF;                                                                  \
    } while (0)
#define PUT16(v)                                                                                   \
    do {                                                                                           \
        *p++ = (v) & 0xFF;                                                                         \
        *p++ = ((v) >> 8) & 0xFF;                                                                  \
    } while (0)
    memcpy(p, "RIFF", 4);
    p += 4;
    PUT32(0); /* size, ignored */
    memcpy(p, "WAVE", 4);
    p += 4;
    if (extra_chunk) {
        memcpy(p, "LIST", 4);
        p += 4;
        PUT32(3); /* odd size: padded to 4 */
        memset(p, 'x', 4);
        p += 4;
    }
    memcpy(p, "fmt ", 4);
    p += 4;
    PUT32(16);
    PUT16(format);
    PUT16(channels);
    PUT32(rate);
    PUT32(rate * channels * bits / 8);
    PUT16(channels * bits / 8);
    PUT16(bits);
    memcpy(p, "data", 4);
    p += 4;
    PUT32(data_bytes);
    memset(p, 0, data_bytes);
    p += data_bytes;
#undef PUT32
#undef PUT16
    return (size_t)(p - buf);
}

TEST parses_mono_pcm16(void)
{
    uint8_t buf[256];
    size_t len = make_wav(buf, 1, 1, 32000, 16, 64, 0);
    WavInfo info;
    ASSERT_EQ(WAV_OK, wav_parse(buf, len, &info));
    ASSERT_EQ(1, info.channels);
    ASSERT_EQ(32000u, info.sample_rate);
    ASSERT_EQ(44u, info.data_offset);
    ASSERT_EQ(64u, info.data_size);
    ASSERT_EQ(32u, info.frames);
    ASSERT_EQ(1u, wav_duration_ms(&info));
    PASS();
}

TEST skips_padded_chunks(void)
{
    uint8_t buf[256];
    size_t len = make_wav(buf, 1, 2, 22050, 16, 40, 1);
    WavInfo info;
    ASSERT_EQ(WAV_OK, wav_parse(buf, len, &info));
    ASSERT_EQ(2, info.channels);
    ASSERT_EQ(56u, info.data_offset);
    ASSERT_EQ(10u, info.frames);
    PASS();
}

TEST rejects_bad_files(void)
{
    uint8_t buf[256];
    WavInfo info;
    size_t len = make_wav(buf, 3, 1, 32000, 16, 64, 0); /* float */
    ASSERT_EQ(WAV_ERR_FORMAT, wav_parse(buf, len, &info));
    len = make_wav(buf, 1, 1, 32000, 8, 64, 0);
    ASSERT_EQ(WAV_ERR_FORMAT, wav_parse(buf, len, &info));
    len = make_wav(buf, 1, 3, 32000, 16, 60, 0);
    ASSERT_EQ(WAV_ERR_FORMAT, wav_parse(buf, len, &info));
    len = make_wav(buf, 1, 1, 32000, 16, 0, 0);
    ASSERT_EQ(WAV_ERR_NO_DATA, wav_parse(buf, len, &info));

    len = make_wav(buf, 1, 1, 32000, 16, 64, 0);
    ASSERT_EQ(WAV_ERR_HEADER, wav_parse(buf, len - 1, &info)); /* data cut short */
    ASSERT_EQ(WAV_ERR_NO_DATA, wav_parse(buf, 36, &info));     /* no data chunk */
    buf[0] = 'X';
    ASSERT_EQ(WAV_ERR_HEADER, wav_parse(buf, len, &info));
    ASSERT_EQ(WAV_ERR_HEADER, wav_parse(NULL, len, &info));
    ASSERT_EQ(WAV_ERR_HEADER, wav_parse(buf, 8, &info));
    PASS();
}

TEST names_unique(void)
{
    for (int i = 0; i < SFX_COUNT; i++) {
        ASSERT(sfx_file_name((Sfx)i) != NULL);
        for (int j = 0; j < i; j++)
            ASSERT(strcmp(sfx_file_name((Sfx)i), sfx_file_name((Sfx)j)) != 0);
    }
    ASSERT_EQ(NULL, sfx_file_name(SFX_COUNT));
    PASS();
}

TEST romfs_sounds_exist_and_are_short(void)
{
    for (int i = 0; i < SFX_COUNT; i++) {
        char path[96];
        snprintf(path, sizeof(path), "romfs/sfx/%s", sfx_file_name((Sfx)i));
        size_t len;
        uint8_t *buf = fixture_load_path(path, &len);
        ASSERTm(path, buf != NULL);
        WavInfo info;
        WavResult r = wav_parse(buf, len, &info);
        fixture_free(buf);
        ASSERT_EQm(path, WAV_OK, r);
        ASSERT_EQm(path, 1, info.channels);
        ASSERTm(path, wav_duration_ms(&info) < SFX_MAX_MS);
    }
    PASS();
}

SUITE(sfx_suite)
{
    RUN_TEST(parses_mono_pcm16);
    RUN_TEST(skips_padded_chunks);
    RUN_TEST(rejects_bad_files);
    RUN_TEST(names_unique);
    RUN_TEST(romfs_sounds_exist_and_are_short);
}
