#include "platform/audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "core/wav.h"

/* ndsp channels 0..AUDIO_VOICES-1 belong to the UI sounds. */
#define AUDIO_VOICES 4
#define AUDIO_VOLUME 0.8f
#define SFX_DIR      "romfs:/sfx/"

typedef struct {
    s16 *pcm; /* linear memory, NULL if not loaded */
    u32 frames;
    u32 rate;
    u16 channels;
} SfxData;

static SfxData s_sfx[SFX_COUNT];
static ndspWaveBuf s_wavebuf[AUDIO_VOICES];
static u32 s_started[AUDIO_VOICES]; /* play counter at start, to find the oldest voice */
static u32 s_play_count;
static bool s_available;
static bool s_enabled = true;

static AudioResult load_sfx(Sfx id)
{
    char path[64];
    snprintf(path, sizeof(path), SFX_DIR "%s", sfx_file_name(id));

    FILE *f = fopen(path, "rb");
    if (!f)
        return AUDIO_ERR_FILE;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) {
        fclose(f);
        return AUDIO_ERR_FILE;
    }

    u8 *file = malloc((size_t)len);
    if (!file) {
        fclose(f);
        return AUDIO_ERR_MEM;
    }
    size_t got = fread(file, 1, (size_t)len, f);
    fclose(f);

    WavInfo info;
    if (got != (size_t)len || wav_parse(file, got, &info) != WAV_OK) {
        free(file);
        return AUDIO_ERR_FILE;
    }

    s16 *pcm = linearAlloc(info.data_size);
    if (!pcm) {
        free(file);
        return AUDIO_ERR_MEM;
    }
    memcpy(pcm, file + info.data_offset, info.data_size);
    free(file);
    DSP_FlushDataCache(pcm, info.data_size);

    s_sfx[id].pcm = pcm;
    s_sfx[id].frames = info.frames;
    s_sfx[id].rate = info.sample_rate;
    s_sfx[id].channels = info.channels;
    return AUDIO_OK;
}

AudioResult audio_init(void)
{
    memset(s_sfx, 0, sizeof(s_sfx));
    memset(s_wavebuf, 0, sizeof(s_wavebuf));
    s_play_count = 0;

    if (R_FAILED(ndspInit())) {
        s_available = false;
        return AUDIO_ERR_DSP;
    }
    s_available = true;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    float mix[12] = {AUDIO_VOLUME, AUDIO_VOLUME};
    for (int ch = 0; ch < AUDIO_VOICES; ch++) {
        ndspChnReset(ch);
        ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
        ndspChnSetMix(ch, mix);
        s_wavebuf[ch].status = NDSP_WBUF_DONE;
    }

    AudioResult result = AUDIO_OK;
    for (int id = 0; id < SFX_COUNT; id++) {
        AudioResult r = load_sfx((Sfx)id);
        if (r != AUDIO_OK && result == AUDIO_OK)
            result = r;
    }
    return result;
}

static void stop_all(void)
{
    for (int ch = 0; ch < AUDIO_VOICES; ch++)
        ndspChnWaveBufClear(ch);
}

void audio_exit(void)
{
    if (s_available) {
        stop_all();
        ndspExit();
        s_available = false;
    }
    for (int id = 0; id < SFX_COUNT; id++) {
        if (s_sfx[id].pcm)
            linearFree(s_sfx[id].pcm);
    }
    memset(s_sfx, 0, sizeof(s_sfx));
}

static int pick_voice(void)
{
    int oldest = 0;
    for (int ch = 0; ch < AUDIO_VOICES; ch++) {
        u8 st = s_wavebuf[ch].status;
        if (st == NDSP_WBUF_DONE || st == NDSP_WBUF_FREE)
            return ch;
        if ((s32)(s_started[ch] - s_started[oldest]) < 0)
            oldest = ch;
    }
    return oldest;
}

void audio_play(Sfx id)
{
    if (!s_available || !s_enabled || (unsigned)id >= SFX_COUNT || !s_sfx[id].pcm)
        return;

    const SfxData *sfx = &s_sfx[id];
    int ch = pick_voice();
    ndspChnWaveBufClear(ch);
    ndspChnSetRate(ch, (float)sfx->rate);
    ndspChnSetFormat(ch, sfx->channels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);

    ndspWaveBuf *wb = &s_wavebuf[ch];
    memset(wb, 0, sizeof(*wb));
    wb->data_vaddr = sfx->pcm;
    wb->nsamples = sfx->frames;
    wb->looping = false;
    s_started[ch] = ++s_play_count;
    ndspChnWaveBufAdd(ch, wb);
}

void audio_set_enabled(bool enabled)
{
    s_enabled = enabled;
    if (!enabled && s_available)
        stop_all();
}

bool audio_is_enabled(void)
{
    return s_enabled;
}

bool audio_is_available(void)
{
    return s_available;
}
