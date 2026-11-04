/* ColorfulTalk V1 の内蔵の声  SPDX-License-Identifier: MIT */
#ifndef CT1_VOICES_H
#define CT1_VOICES_H

#include "ct_fx.h"

/* 組み込む声を 1 つ選ぶ: 0 F1, 1 F2, 2 F3, 3 M1, 4 M2, 5 C1, 6 E1, 7 W1, 8 R1, 9 K1 */
#ifndef CT1_VOICE_ID
#  define CT1_VOICE_ID 0
#endif
/* テスト用: 全部の声を入れる */
#ifndef CT1_VOICE_ALL
#  define CT1_VOICE_ALL 0
#endif

extern const CtFxVoice CT1_VOICE_F1, CT1_VOICE_F2, CT1_VOICE_F3, CT1_VOICE_M1, CT1_VOICE_M2,
                       CT1_VOICE_C1, CT1_VOICE_E1, CT1_VOICE_W1, CT1_VOICE_R1, CT1_VOICE_K1;

#endif
