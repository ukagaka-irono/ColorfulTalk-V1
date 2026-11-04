/* ColorfulTalk V1: 公開 API  SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include <string.h>
#include "colorful_talk_v1.h"
#include "ct1_voices.h"
#ifndef CT_NO_WAV
#  include "ct_wav.h"
#endif

#if CT1_VOICE_ID == 0
#  define VOICE CT1_VOICE_F1
#elif CT1_VOICE_ID == 1
#  define VOICE CT1_VOICE_F2
#elif CT1_VOICE_ID == 2
#  define VOICE CT1_VOICE_F3
#elif CT1_VOICE_ID == 3
#  define VOICE CT1_VOICE_M1
#elif CT1_VOICE_ID == 4
#  define VOICE CT1_VOICE_M2
#elif CT1_VOICE_ID == 5
#  define VOICE CT1_VOICE_C1
#elif CT1_VOICE_ID == 6
#  define VOICE CT1_VOICE_E1
#elif CT1_VOICE_ID == 7
#  define VOICE CT1_VOICE_W1
#elif CT1_VOICE_ID == 8
#  define VOICE CT1_VOICE_R1
#elif CT1_VOICE_ID == 9
#  define VOICE CT1_VOICE_K1
#else
#  error "CT1_VOICE_ID must be 0..9"
#endif

static const CtFxVoice* g_voice = &VOICE;

#if CT1_VOICE_ALL
/* テスト用: 声を切り替える（ライブラリとして配布するビルドには入らない） */
const CtFxVoice* const CT1_ALL_VOICES[10] = {
    &CT1_VOICE_F1, &CT1_VOICE_F2, &CT1_VOICE_F3, &CT1_VOICE_M1, &CT1_VOICE_M2,
    &CT1_VOICE_C1, &CT1_VOICE_E1, &CT1_VOICE_W1, &CT1_VOICE_R1, &CT1_VOICE_K1,
};
void ct1_test_select_voice(int id) { g_voice = CT1_ALL_VOICES[id]; }
#endif

size_t ct1_work_size(void)
{
    return sizeof(CtFxWork);
}

int ct1_start(void* work, const char* koe, int speed)
{
    if (!work) return CT_ERR_NULL;
    return ctfx_start((CtFxWork*)work, koe, g_voice, (int16_t)(speed < 0 ? 0 : speed > 300 ? 301 : speed));
}

int ct1_read(void* work, int16_t* buf, int n)
{
    return ctfx_read((CtFxWork*)work, buf, (int16_t)(n > 32767 ? 32767 : n));
}

int ct1_error_pos(const void* work)
{
    return work ? ((const CtFxWork*)work)->err_pos : -1;
}

const char* ct1_voice_name(void)
{
    return g_voice->name;
}

/* PC 向け（組み込みでは CT_NO_WAV を定義して外せる） */
#ifndef CT_NO_WAV
unsigned char* ct1_synthe_wav(const char* koe, int speed, int* size, int* err)
{
    CtFxWork* w = (CtFxWork*)malloc(sizeof *w);
    CtPcm pcm = { 0 };
    unsigned char* wav = NULL;
    int e;

    if (size) *size = 0;
    if (!w) { if (err) *err = CT_ERR_MEMORY; return NULL; }
    e = !size ? CT_ERR_NULL : ct1_start(w, koe, speed);
    if (!e) {
        int16_t buf[512];
        int got;
        while ((got = ct1_read(w, buf, 512)) > 0) {
            if (ct_pcm_append(&pcm, buf, got)) { e = CT_ERR_MEMORY; break; }
        }
        if (!e) {
            wav = ct_wav_build(pcm.data, pcm.n, CT1_SAMPLE_RATE, size);
            if (!wav) e = CT_ERR_MEMORY;
        }
    }
    free(pcm.data);
    free(w);
    if (err) *err = e;
    return wav;
}

void ct1_free(void* buf)
{
    free(buf);
}
#endif
