/*
 * fxgen: 固定小数点コアの表（fx/src/fx_tables.c）と内蔵の声（src/ct1_voices.c）を作る。
 * PC 上で一度だけ動かす開発用ツール（浮動小数点を使う）。生成物はリポジトリに入れる。
 *   fxgen <fx_tables.c の出力先> <ct1_voices.c の出力先>
 * SPDX-License-Identifier: MIT
 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PI 3.14159265358979323846
#define FS 8000.0
#define PULSE_N 64

static long rnd(double x) { return (long)floor(x + 0.5); }

/* ---- LF モデル（ColorfulTalk V3 と同じ式） ------------------------------- */
static double lf_area(double alpha, double tp, double te, double ta, double eps)
{
    double wg = PI / tp, D = 1.0 - te;
    double s = sin(wg * te), ex = exp(alpha * te);
    double E0 = -1.0 / (ex * s);
    double open = E0 * (ex * (alpha * s - wg * cos(wg * te)) + wg) / (alpha * alpha + wg * wg);
    double ret = -(1.0 / (eps * ta)) * ((1.0 - exp(-eps * D)) / eps - D * exp(-eps * D));
    return open + ret;
}

static void lf_pulse(double rd, double* out, int n, double* te_out)
{
    double Rd = rd < 0.3 ? 0.3 : rd > 2.7 ? 2.7 : rd;
    double Rap = (-1.0 + 4.8 * Rd) / 100.0, Rkp = (22.4 + 11.8 * Rd) / 100.0;
    double Rgp = 0.25 * Rkp / (0.11 * Rd / (0.5 + 1.2 * Rkp) - Rap);
    double tp = 0.5 / Rgp, te = tp * (1.0 + Rkp), ta = Rap, D, eps, alpha = 0, lo, hi, flo, wg, E0, acc, peak;
    int k, it;
    if (te > 0.95) te = 0.95;
    if (ta < 1e-3) ta = 1e-3;
    D = 1.0 - te;
    eps = 1.0 / ta;
    for (it = 0; it < 60; it++) eps = (1.0 - exp(-eps * D)) / ta;
    lo = -20.0;
    flo = lf_area(lo, tp, te, ta, eps);
    for (hi = -19.0; hi <= 200.0; hi += 1.0) {
        double fh = lf_area(hi, tp, te, ta, eps);
        if ((fh > 0) != (flo > 0)) {
            for (it = 0; it < 60; it++) {
                double mid = 0.5 * (lo + hi), fm = lf_area(mid, tp, te, ta, eps);
                if ((fm > 0) == (flo > 0)) { lo = mid; flo = fm; } else hi = mid;
            }
            alpha = 0.5 * (lo + hi);
            break;
        }
        lo = hi;
        flo = fh;
    }
    wg = PI / tp;
    E0 = -1.0 / (exp(alpha * te) * sin(wg * te));
    acc = 0;
    peak = 1e-12;
    for (k = 0; k < n; k++) {
        double t = (k + 0.5) / n, e;
        if (t <= te) e = E0 * exp(alpha * t) * sin(wg * t);
        else e = -(1.0 / (eps * ta)) * (exp(-eps * (t - te)) - exp(-eps * D));
        acc += e / n;
        out[k] = acc;
    }
    for (k = 0; k < n; k++) {
        out[k] -= acc * (k + 1) / n;
        if (out[k] > peak) peak = out[k];
    }
    for (k = 0; k < n; k++) out[k] = out[k] < 0 ? 0 : out[k] / peak;
    *te_out = te;
}

static double slope_energy(double rd)
{
    double p[256], te, e = 0;
    int k;
    lf_pulse(rd, p, 256, &te);
    for (k = 1; k < 256; k++) e += (p[k] - p[k - 1]) * (p[k] - p[k - 1]);
    return e;
}

/* ---- 表 ------------------------------------------------------------------ */
static void row16(FILE* f, const char* decl, const long* v, int n)
{
    int i;
    fprintf(f, "%s[%d] = {", decl, n);
    for (i = 0; i < n; i++) fprintf(f, "%s%ld%s", i % 12 ? " " : "\n    ", v[i], i < n - 1 ? "," : "");
    fprintf(f, "\n};\n\n");
}

static void gen_tables(FILE* f)
{
    long v[300];
    int i;
    fprintf(f, "/* 生成ファイル（tools/fxgen.c）。手で編集しないこと。  SPDX-License-Identifier: MIT */\n");
    fprintf(f, "#include \"fx_internal.h\"\n\n");

    for (i = 0; i <= 256; i++) v[i] = rnd(16384.0 * cos(PI * i / 256.0));
    fprintf(f, "/* cos(pi*i/256)  Q14 */\n");
    row16(f, "const int16_t FX_COS", v, 257);

    for (i = 0; i <= 256; i++) v[i] = rnd(16384.0 * exp(-PI * (i * 16.0) / FS));
    fprintf(f, "/* exp(-pi*bw/fs), bw = i*16 Hz  Q14 */\n");
    row16(f, "const int16_t FX_EXPBW", v, 257);

    for (i = 0; i <= 72; i++) v[i] = i == 0 ? 0 : rnd(4096.0 * pow(10.0, (i - 60) / 20.0));
    fprintf(f, "/* 線形振幅（60dB = 4096）  Q12 */\n");
    row16(f, "const int16_t FX_DB", v, 73);

    for (i = 0; i <= 256; i++) v[i] = rnd(16384.0 * pow(2.0, i / 256.0));
    fprintf(f, "/* 2^(i/256)  Q14 */\n");
    row16(f, "const uint16_t FX_EXP2", v, 257);

    for (i = 0; i < 256; i++) { double t = i * 0.016; v[i] = rnd(4096.0 * 9.0 * t * exp(-3.0 * t)); }
    fprintf(f, "/* 藤崎モデルのフレーズ成分 Gp(t), t = i*16ms, alpha = 3  Q12 */\n");
    row16(f, "const int16_t FX_GP", v, 256);

    for (i = 0; i <= 100; i++) {
        double t = i * 0.004, g = 1.0 - (1.0 + 20.0 * t) * exp(-20.0 * t);
        v[i] = rnd(4096.0 * (g < 0.9 ? g : 0.9));
    }
    fprintf(f, "/* 藤崎モデルのアクセント成分 Ga(t), t = i*4ms, beta = 20, gamma = 0.9  Q12 */\n");
    row16(f, "const int16_t FX_GA", v, 101);

    for (i = 0; i < 64; i++) v[i] = rnd(16384.0 * sin(2.0 * PI * i / 64.0));
    fprintf(f, "/* sin(2*pi*i/64)  Q14 */\n");
    row16(f, "const int16_t FX_SIN", v, 64);

    for (i = 0; i <= 25; i++) v[i] = rnd(256.0 * pow(100.0 / (50 + 10 * i), 0.7));
    fprintf(f, "/* 子音の長さの倍率 (100/speed)^0.7, speed = 50 + 10*i  Q8 */\n");
    row16(f, "const uint16_t FX_CSC", v, 26);
    for (i = 0; i <= 25; i++) v[i] = rnd(256.0 * pow(100.0 / (50 + 10 * i), 1.15));
    fprintf(f, "/* 母音の長さの倍率 (100/speed)^1.15  Q8 */\n");
    row16(f, "const uint16_t FX_VSC", v, 26);
}

/* ---- 声 ------------------------------------------------------------------ */
typedef struct {
    const char* id; const char* desc;
    double f0, range, fscale, rd, breath, jitter; int whisper;
    double level;   /* 8kHz で実測した音量の補正（声ごとに大きさをそろえる） */
} VoiceDef;

static const VoiceDef VOICES[] = {
    { "F1", "女声（標準）",            210, 1.00, 1.15, 1.30, 1.0, 1.0, 0, 1.030 },
    { "F2", "高めの女声",              260, 1.10, 1.22, 1.20, 1.0, 1.0, 0, 0.966 },
    { "F3", "落ち着いた女声",          180, 0.80, 1.10, 1.50, 1.2, 0.8, 0, 1.136 },
    { "M1", "男声",                    115, 1.00, 1.00, 1.00, 0.5, 0.8, 0, 1.074 },
    { "M2", "低い男声",                 95, 0.90, 0.93, 1.10, 0.5, 0.8, 0, 1.149 },
    { "C1", "子ども",                  290, 1.10, 1.28, 1.40, 1.0, 1.0, 0, 0.988 },
    { "E1", "年配の声",                150, 0.70, 1.08, 1.80, 1.6, 2.5, 0, 1.093 },
    { "W1", "ささやき声",              200, 1.00, 1.12, 1.30, 1.0, 0.0, 1, 0.619 },
    { "R1", "ロボット",                140, 0.40, 1.05, 0.50, 0.0, 0.0, 0, 1.495 },
    { "K1", "高く明るいキャラクター声", 300, 1.40, 1.32, 1.10, 0.8, 0.8, 0, 1.054 },
};
#define NVOICE (int)(sizeof VOICES / sizeof VOICES[0])

static void gen_voices(FILE* f)
{
    double ref = slope_energy(1.3);
    int i, k;
    fprintf(f, "/* 生成ファイル（tools/fxgen.c）。手で編集しないこと。  SPDX-License-Identifier: MIT */\n");
    fprintf(f, "#include \"ct1_voices.h\"\n\n");
    for (i = 0; i < NVOICE; i++) {
        const VoiceDef* d = &VOICES[i];
        double p[PULSE_N], te, tilt_db, tilt_a, gain, breath_db;
        lf_pulse(d->rd, p, PULSE_N, &te);
        tilt_db = d->rd > 0.6 ? 6.0 * (d->rd - 0.6) : 0.0;
        tilt_a = tilt_db >= 25 ? 0.85 : tilt_db / 25.0 * 0.85;
        gain = pow(210.0 / d->f0, 1.1) * pow(ref / slope_energy(d->rd), 0.8) * pow(d->rd / 1.3, 0.4);
        if (d->fscale > 1.15) gain *= pow(1.15 / d->fscale, 1.5);
        if (d->whisper) gain = 1.0;
        gain *= d->level;
        breath_db = d->breath > 0 ? 40.0 + 20.0 * log10(d->breath) : 0.0;
        fprintf(f, "#if CT1_VOICE_ALL || CT1_VOICE_ID == %d\n", i);
        fprintf(f, "/* %s: %s  (F0 %.0fHz, 声道 %.2f, Rd %.2f) */\n", d->id, d->desc, d->f0, d->fscale, d->rd);
        fprintf(f, "const CtFxVoice CT1_VOICE_%s = {\n", d->id);
        fprintf(f, "    \"%s\", %ld, %ld, %d, %ld, %ld, %ld, %ld, %ld, %ld, %ld, 0,\n    {",
                d->id, rnd(d->f0), rnd(64 * d->range), d->whisper, rnd(4096 * d->fscale),
                rnd(d->fscale > 1.0 ? 130.0 * (d->fscale - 1.0) : 0.0), rnd(256 * tilt_a),
                rnd(breath_db), rnd(10.24 * d->jitter), rnd(256 * gain), rnd(64 * te));
        for (k = 0; k < PULSE_N; k++) fprintf(f, "%s%ld%s", k % 12 ? " " : "\n        ", rnd(16384 * p[k]), k < PULSE_N - 1 ? "," : "");
        fprintf(f, "\n    }\n};\n#endif\n\n");
    }
}

int main(int argc, char** argv)
{
    FILE *a, *b;
    if (argc != 3) { fprintf(stderr, "usage: fxgen fx_tables.c ct1_voices.c\n"); return 2; }
    a = fopen(argv[1], "wb");
    b = fopen(argv[2], "wb");
    if (!a || !b) { fprintf(stderr, "fxgen: cannot write\n"); return 1; }
    gen_tables(a);
    gen_voices(b);
    fclose(a);
    fclose(b);
    return 0;
}
