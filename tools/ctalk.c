/* ctalk: ColorfulTalk V1 の試聴・WAV 出力ツール（開発用に 10 種の声をすべて含む）  SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "colorful_talk_v1.h"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>
#endif

static void usage(void)
{
    fputs(
        "usage: ctalk [options] \"koe string\"\n"
        "  -o FILE     write WAV to FILE (default out.wav unless --play)\n"
        "  -f FILE     read the koe string from a UTF-8 text file\n"
        "  -v VOICE    F1 F2 F3 M1 M2 C1 E1 W1 R1 K1   (default F1)\n"
        "  -s N        speed 50-300 (default 100)\n"
        "  --play      play the result (Windows)\n"
        "example: ctalk -o hello.wav \"こんにちわ、せか'い。\"\n",
        stderr);
}

static FILE* u8fopen(const char* path, const char* mode)
{
#ifdef _WIN32
    wchar_t wp[1024], wm[8];
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wp, 1024)) return NULL;
    if (!MultiByteToWideChar(CP_UTF8, 0, mode, -1, wm, 8)) return NULL;
    return _wfopen(wp, wm);
#else
    return fopen(path, mode);
#endif
}

static char* read_file(const char* path)
{
    FILE* f = u8fopen(path, "rb");
    char* buf;
    long len;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (char*)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    len = (long)fread(buf, 1, (size_t)len, f);
    buf[len] = '\0';
    fclose(f);
    if (len >= 3 && (unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB && (unsigned char)buf[2] == 0xBF)
        memmove(buf, buf + 3, (size_t)len - 2);
    return buf;
}

static int _stricmp_ascii(const char* a, const char* b)
{
    for (; *a && *b; a++, b++) {
        char x = (char)(*a >= 'a' && *a <= 'z' ? *a - 32 : *a), y = (char)(*b >= 'a' && *b <= 'z' ? *b - 32 : *b);
        if (x != y) return 1;
    }
    return *a != *b;
}

static const char* err_name(int e)
{
    static const char* names[] = { "ok", "null argument", "invalid UTF-8", "character not allowed",
        "misplaced accent mark", "misplaced long vowel mark", "misplaced devoicing mark",
        "nothing to speak", "parameter out of range", "koe string too long", "out of memory",
        "invalid voice data" };
    return e >= 0 && e < (int)(sizeof names / sizeof names[0]) ? names[e] : "unknown error";
}

static void print_error(const char* koe, int err, int pos)
{
    fprintf(stderr, "ctalk: error %d: %s", err, err_name(err));
    if (pos >= 0 && koe) {
        const unsigned char* p = (const unsigned char*)koe;
        int k = 0;
        while (*p && k < pos) {
            p++;
            while ((*p & 0xC0) == 0x80) p++;
            k++;
        }
        if (*p) {
            const unsigned char* q = p + 1;
            while ((*q & 0xC0) == 0x80) q++;
            fprintf(stderr, " at character %d '%.*s'", pos, (int)(q - p), (const char*)p);
        } else {
            fprintf(stderr, " at character %d", pos);
        }
    }
    fputc('\n', stderr);
}

void ct1_test_select_voice(int id);

static int run(int argc, char** argv)
{
    static const char* NAMES[10] = { "F1", "F2", "F3", "M1", "M2", "C1", "E1", "W1", "R1", "K1" };
    const char* out = NULL;
    char* koe = NULL;
    int owned = 0, play = 0, i, size = 0, err = 0, speed = 100;
    unsigned char* wav;

    for (i = 1; i < argc; i++) {
        const char* a = argv[i];
        const char* val = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
        if (!strcmp(a, "--play")) { play = 1; continue; }
        if (!strcmp(a, "-o") || !strcmp(a, "-f") || !strcmp(a, "-v") || !strcmp(a, "-s")) {
            if (!val) { usage(); return 2; }
            i++;
            if (a[1] == 'o') out = val;
            else if (a[1] == 's') speed = atoi(val);
            else if (a[1] == 'v') {
                int k, found = -1;
                for (k = 0; k < 10; k++) if (!_stricmp_ascii(val, NAMES[k])) found = k;
                if (found < 0) { fprintf(stderr, "ctalk: unknown voice %s\n", val); return 2; }
                ct1_test_select_voice(found);
            } else {
                koe = read_file(val);
                if (!koe) { fprintf(stderr, "ctalk: cannot read %s\n", val); return 1; }
                owned = 1;
            }
            continue;
        }
        if (koe) { usage(); return 2; }
        koe = argv[i];
    }
    if (!koe) { usage(); return 2; }

    wav = ct1_synthe_wav(koe, speed, &size, &err);
    if (!wav) {
        int pos = -1;
        if (err != CT_ERR_PARAM && err != CT_ERR_MEMORY) {
            void* w = malloc(ct1_work_size());
            if (w) { ct1_start(w, koe, speed); pos = ct1_error_pos(w); free(w); }
        }
        print_error(koe, err, pos);
        if (owned) free(koe);
        return 1;
    }
    if (owned) free(koe);
    if (out || !play) {
        FILE* f = u8fopen(out ? out : "out.wav", "wb");
        if (!f || fwrite(wav, 1, (size_t)size, f) != (size_t)size) {
            fprintf(stderr, "ctalk: cannot write %s\n", out ? out : "out.wav");
            if (f) fclose(f);
            ct1_free(wav);
            return 1;
        }
        fclose(f);
    }
#ifdef _WIN32
    if (play) PlaySoundA((LPCSTR)wav, NULL, SND_MEMORY | SND_SYNC);
#else
    if (play) fputs("ctalk: --play is only supported on Windows\n", stderr);
#endif
    ct1_free(wav);
    return 0;
}

#ifdef _WIN32
int wmain(int argc, wchar_t** wargv)
{
    char** argv = (char**)calloc((size_t)argc + 1, sizeof *argv);
    int i, r;
    SetConsoleOutputCP(CP_UTF8);
    for (i = 0; i < argc; i++) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, NULL, 0, NULL, NULL);
        argv[i] = (char*)malloc((size_t)len);
        WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], len, NULL, NULL);
    }
    r = run(argc, argv);
    for (i = 0; i < argc; i++) free(argv[i]);
    free(argv);
    return r;
}
#else
int main(int argc, char** argv)
{
    return run(argc, argv);
}
#endif
