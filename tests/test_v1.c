/* ColorfulTalk V1 のテスト（10 種の声をすべて含むビルドで実行） */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colorful_talk_v1.h"
#include "ct_test_audio.h"

void ct1_test_select_voice(int id);

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)
#define FS CT1_SAMPLE_RATE

static const char* ALL_MORA[] = {
    "あ","い","う","え","お","か","き","く","け","こ","が","ぎ","ぐ","げ","ご",
    "さ","し","す","せ","そ","ざ","じ","ず","ぜ","ぞ","た","ち","つ","て","と",
    "だ","ぢ","づ","で","ど","な","に","ぬ","ね","の","は","ひ","ふ","へ","ほ",
    "ば","び","ぶ","べ","ぼ","ぱ","ぴ","ぷ","ぺ","ぽ","ま","み","む","め","も",
    "や","ゆ","よ","ら","り","る","れ","ろ","わ","を","ん","ゔ",
    "きゃ","きゅ","きょ","ぎゃ","しゃ","しゅ","しょ","じゃ","ちゃ","ちゅ","ちょ",
    "にゃ","ひゃ","びゃ","ぴゃ","みゃ","りゃ","ふぁ","てぃ","でぃ","とぅ","うぃ","うぇ","うぉ",
    "あっか","あった","あっさ","あっぱ","かーど","さんぽ","さんか","ほんや",
};

static uint8_t* g_work;

static int synth(const char* koe, int speed, int16_t** out, int* n, int chunk)
{
    int16_t buf[1024];
    int got, cap = 1 << 15, err;
    *n = 0;
    *out = NULL;
    err = ct1_start(g_work, koe, speed);
    if (err) return err;
    *out = (int16_t*)malloc((size_t)cap * 2);
    while ((got = ct1_read(g_work, buf, chunk)) > 0) {
        if (*n + got > cap) { cap *= 2; *out = (int16_t*)realloc(*out, (size_t)cap * 2); }
        memcpy(*out + *n, buf, (size_t)got * 2);
        *n += got;
    }
    return CT_OK;
}

static void test_sizes(void)
{
    printf("work size: %u bytes\n", (unsigned)ct1_work_size());
    CHECK(ct1_work_size() <= 8 * 1024);
}

static void test_every_mora(void)
{
    size_t k;
    for (k = 0; k < sizeof ALL_MORA / sizeof ALL_MORA[0]; k++) {
        int16_t* x;
        int n, err = synth(ALL_MORA[k], 100, &x, &n, 256);
        double r;
        if (err) { printf("FAIL %s: err %d\n", ALL_MORA[k], err); failures++; continue; }
        r = ta_rms(x, n);
        if (r < 300 || ta_clipped(x, n) > n / 1000) {
            printf("FAIL %s: rms=%.0f clipped=%d/%d\n", ALL_MORA[k], r, ta_clipped(x, n), n);
            failures++;
        }
        free(x);
    }
}

static void test_streaming_chunks(void)
{
    int16_t *a, *b;
    int na, nb;
    CHECK(synth("こんにちわ、せか'い。", 100, &a, &na, 1000) == CT_OK);
    CHECK(synth("こんにちわ、せか'い。", 100, &b, &nb, 7) == CT_OK);
    CHECK(na == nb && na > 0 && memcmp(a, b, (size_t)na * 2) == 0);
    free(a);
    free(b);
}

static void test_wav(void)
{
    int size = 0, err = -1;
    unsigned char* w = ct1_synthe_wav("こんにちわ", 100, &size, &err);
    CHECK(err == CT_OK && w != NULL);
    if (!w) return;
    CHECK(memcmp(w, "RIFF", 4) == 0 && memcmp(w + 8, "WAVE", 4) == 0);
    CHECK((w[24] | (w[25] << 8)) == 8000);
    ct1_free(w);
}

static void test_speed(void)
{
    int16_t *a, *b;
    int na, nb;
    CHECK(synth("あいうえお、かきくけこ", 100, &a, &na, 256) == CT_OK);
    CHECK(synth("あいうえお、かきくけこ", 200, &b, &nb, 256) == CT_OK);
    printf("speed x2 length ratio %.2f\n", (double)nb / na);
    CHECK((double)nb / na > 0.45 && (double)nb / na < 0.65);
    free(a);
    free(b);
}

static void test_pitch(void)
{
    int16_t* x;
    int n, a, b, len = FS / 20, w;
    double f1[2], f2[2], fq, fst;
    static const char* words[2] = { "あ'い", "あい'" };
    ct1_test_select_voice(3);   /* M1 */

    CHECK(synth("あー", 100, &x, &n, 256) == CT_OK);
    ta_span(x, n, 500, &a, &b);
    f1[0] = ta_f0(x, n, FS, (a + b) / 2 - len / 2, len);
    printf("M1 f0 %.0f Hz\n", f1[0]);
    CHECK(f1[0] > 100 && f1[0] < 160);
    free(x);

    for (w = 0; w < 2; w++) {
        CHECK(synth(words[w], 100, &x, &n, 256) == CT_OK);
        ta_span(x, n, 500, &a, &b);
        f1[w] = ta_f0(x, n, FS, a + (b - a) / 4 - len / 2, len);
        f2[w] = ta_f0(x, n, FS, a + (b - a) * 2 / 3 - len / 2, len);
        free(x);
    }
    printf("accent: あ'い %.0f -> %.0f Hz, あい' %.0f -> %.0f Hz\n", f1[0], f2[0], f1[1], f2[1]);
    CHECK(f1[0] > f2[0] * 1.08);
    CHECK(f2[1] > f1[1] * 1.08);

    CHECK(synth("そうですか？", 100, &x, &n, 256) == CT_OK);
    ta_span(x, n, 500, &a, &b);
    fq = ta_f0_max(x, n, FS, b - FS / 8, b);
    free(x);
    CHECK(synth("そうですか。", 100, &x, &n, 256) == CT_OK);
    ta_span(x, n, 500, &a, &b);
    fst = ta_f0_max(x, n, FS, b - FS / 8, b);
    free(x);
    printf("question end %.0f Hz, statement end %.0f Hz\n", fq, fst);
    CHECK(fq > fst * 1.2);
    ct1_test_select_voice(0);
}

static void test_errors(void)
{
    int size = 0, err = 0;
    CHECK(ct1_start(g_work, "あいう漢字", 100) == CT_ERR_CHAR);
    CHECK(ct1_error_pos(g_work) == 3);
    CHECK(ct1_read(g_work, (int16_t*)&size, 1) == 0);
    CHECK(ct1_synthe_wav(NULL, 100, &size, &err) == NULL && err == CT_ERR_NULL);
    CHECK(ct1_synthe_wav("あ", 10, &size, &err) == NULL && err == CT_ERR_PARAM);
    CHECK(ct1_synthe_wav("あ", 301, &size, &err) == NULL && err == CT_ERR_PARAM);
}

static void test_voices(void)
{
    static const char* NAMES[10] = { "F1", "F2", "F3", "M1", "M2", "C1", "E1", "W1", "R1", "K1" };
    int v;
    for (v = 0; v < 10; v++) {
        int16_t* x;
        int n;
        ct1_test_select_voice(v);
        CHECK(strcmp(ct1_voice_name(), NAMES[v]) == 0);
        CHECK(synth("こんにちわ、せか'い。", 100, &x, &n, 256) == CT_OK);
        printf("voice %s rms %.0f clipped %d\n", NAMES[v], ta_rms(x, n), ta_clipped(x, n));
        CHECK(ta_rms(x, n) > 1000 && ta_clipped(x, n) < n / 1000);
        free(x);
    }
    ct1_test_select_voice(0);
}

static void test_long_input(void)
{
    static char buf[3 * 300 + 8];
    int16_t* x;
    int n, i;
    for (i = 0; i < 250; i++) memcpy(buf + 3 * i, i % 20 == 19 ? "、" : "か", 3);
    buf[3 * 250] = 0;
    CHECK(synth(buf, 100, &x, &n, 256) == CT_OK);
    CHECK(n > FS * 20);
    free(x);
    for (i = 0; i < 300; i++) memcpy(buf + 3 * i, "か", 3);
    buf[3 * 300] = 0;
    CHECK(ct1_start(g_work, buf, 100) == CT_ERR_TOOLONG);
}

int main(void)
{
    g_work = (uint8_t*)malloc(ct1_work_size());
    test_sizes();
    test_every_mora();
    test_streaming_chunks();
    test_wav();
    test_speed();
    test_pitch();
    test_errors();
    test_voices();
    test_long_input();
    free(g_work);
    if (failures) { printf("%d failure(s)\n", failures); return 1; }
    printf("v1: all tests passed\n");
    return 0;
}
