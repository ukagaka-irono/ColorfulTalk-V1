/*
 * ColorfulTalk V1 — 16bit CPU から動く日本語音声合成エンジン
 *
 * 入力: カナ＋アクセント記号の記号列（UTF-8, docs/koe-kana.md）
 * 出力: 16bit 符号付き PCM, モノラル, 8kHz
 * 整数演算のみ。声はビルド時に 1 種類を組み込む（CMake の CT1_VOICE で選ぶ。既定は女声 F1）。
 * SPDX-License-Identifier: MIT
 */
#ifndef COLORFUL_TALK_V1_H
#define COLORFUL_TALK_V1_H

#include <stddef.h>
#include <stdint.h>
#include "colorful_talk_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CT1_VERSION     "1.0.0"
#define CT1_SAMPLE_RATE 8000

/* ---- ストリーミング API（動的メモリ不使用） ----------------------------
 *   static uint8_t work[...];   // ct1_work_size() バイト以上, 4 バイト境界
 *   ct1_start(work, koe, 100);
 *   while ((n = ct1_read(work, buf, 64)) > 0) { 再生(buf, n); }
 */
CT_API size_t      ct1_work_size(void);
CT_API int         ct1_start(void* work, const char* koe_utf8, int speed);  /* speed: 50〜300（100 = 標準） */
CT_API int         ct1_read(void* work, int16_t* buf, int n);               /* 書いたサンプル数。0 で終了 */
CT_API int         ct1_error_pos(const void* work);
CT_API const char* ct1_voice_name(void);                                    /* 組み込んだ声（"F1" など） */

/* ---- PC 向けの便利関数（malloc を使う） --------------------------------- */
CT_API unsigned char* ct1_synthe_wav(const char* koe_utf8, int speed, int* size, int* err);
CT_API void           ct1_free(void* buf);

#ifdef __cplusplus
}
#endif

#endif
