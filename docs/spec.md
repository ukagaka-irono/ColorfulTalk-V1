# ColorfulTalk V1 仕様書

> シリーズ全体の仕様書: [ColorfulTalk](https://github.com/ukagaka-irono/ColorfulTalk)（`docs/spec/v1.md` と同じ内容）

| 項目 | 内容 |
|---|---|
| 版 | 1.0.0 |
| 位置づけ | 16bit CPU から動く、声が 1 種類の小さなエンジン。後継は V3 |
| 対象 | 16bit 以上の CPU（`int` が 16bit でも動く）、Windows / macOS / Linux |
| 方式 | 規則合成（フォルマント合成）。V3 の方式を整数演算に移したもの |
| 入力 | カナ記号列（UTF-8）。[koe-kana.md](koe-kana.md) |
| 出力 | 16bit 符号付き PCM、モノラル、8kHz |
| 演算 | 整数のみ。幅の決まった型（`int16_t` / `int32_t`）だけを使い、64bit 演算・浮動小数点は使わない |
| 作業領域 | 4,928 バイト（`ct1_work_size()`）。呼び出し側が用意する |
| 動的メモリ | ストリーミング API は使わない。`ct1_synthe_wav()` だけ `malloc` を使う |
| スレッド | 作業領域ごとに独立 |
| 声 | ライブラリに 1 種類を組み込む。標準は女声 `F1`。ほかに 9 種類から選んでビルドできる |
| ライセンス | MIT |

## 寸法

ATmega1284P（8bit AVR、`int` は 16bit）向けに avr-gcc 16.1 `-Os` でビルドし、シミュレータ（avr8js）で測定。

| 項目 | 値 | 目標 |
|---|---|---|
| ROM（コード＋表＋声 1 種） | 約 24KB | 100KB 以下 |
| RAM: 作業領域 | 4,928 バイト | |
| RAM: スタックの最大 | 約 490 バイト | |
| RAM 合計（定数を ROM に置ける CPU） | 約 5.4KB | 8KB 以下 |
| PC 版との出力の一致 | ビット単位で一致（4 文で確認） | |

AVR は定数を RAM にコピーするため、上記に加えて約 4.6KB を使います（`PROGMEM` 化は今後の課題）。
ARM・MSP430・RL78 など、定数を ROM のまま読める CPU では不要です。

### 処理時間

AVR（8bit、ハードウェアの乗算は 8×8 のみ）では 1 サンプルあたり平均約 4,400 サイクルで、
8kHz を実時間で出すには約 35MHz 相当が必要です。16×16 の乗算器を持つ 16bit / 32bit CPU では大幅に少なくなります。
フレームの始め（4ms ごと）に処理が集中するため、1 フレーム分（32 サンプル）以上の出力バッファを用意してください。

## 声

ビルド時に 1 つ選びます（CMake の設定時に `-DCT1_VOICE=F1` など。既定は `F1`）。
声を変えるときはライブラリを差し替えます。

| 名前 | 声 | F0 | 声道倍率 | Rd |
|---|---|---|---|---|
| `F1`（標準） | 女声 | 210Hz | 1.15 | 1.30 |
| `F2` | 高めの女声 | 260Hz | 1.22 | 1.20 |
| `F3` | 落ち着いた女声 | 180Hz | 1.10 | 1.50 |
| `M1` | 男声 | 115Hz | 1.00 | 1.00 |
| `M2` | 低い男声 | 95Hz | 0.93 | 1.10 |
| `C1` | 子ども | 290Hz | 1.28 | 1.40 |
| `E1` | 年配の声 | 150Hz | 1.08 | 1.80 |
| `W1` | ささやき声 | ― | 1.12 | ― |
| `R1` | ロボット | 140Hz | 1.05 | 0.50 |
| `K1` | 高く明るいキャラクター声 | 300Hz | 1.32 | 1.10 |

## API

ヘッダ: [include/colorful_talk_v1.h](../include/colorful_talk_v1.h)

```c
size_t      ct1_work_size(void);
int         ct1_start(void* work, const char* koe_utf8, int speed);   /* speed: 50〜300（100 = 標準） */
int         ct1_read(void* work, int16_t* buf, int n);                /* 書いたサンプル数。0 で終わり */
int         ct1_error_pos(const void* work);
const char* ct1_voice_name(void);                                     /* 組み込んだ声の名前 */

/* PC 向け（CT_NO_WAV を定義すると外れる） */
unsigned char* ct1_synthe_wav(const char* koe_utf8, int speed, int* size, int* err);
void           ct1_free(void* buf);
```

| 関数 | 説明 |
|---|---|
| `ct1_work_size` | 作業領域に必要なバイト数 |
| `ct1_start` | 記号列を解析して合成を始める。戻り値はエラーコード |
| `ct1_read` | 最大 `n` サンプルを書き、書いた数を返す。0 なら終わり。何サンプルずつ読んでも結果は同じ |
| `ct1_error_pos` | `ct1_start` が失敗したとき、原因の文字位置（0 始まり、文字単位）。不明なら -1 |
| `ct1_voice_name` | 組み込んだ声の名前（`"F1"` など） |
| `ct1_synthe_wav` | RIFF/WAVE（16bit, 8kHz, モノラル）のバイト列を返す |

作業領域は 4 バイト境界に置いてください。

```c
static uint8_t work[5120];
int16_t buf[64];
int n;

if (ct1_start(work, "こんにちわ、せか'い。", 100) == CT_OK) {
    while ((n = ct1_read(work, buf, 64)) > 0) {
        /* buf[0..n) を DAC へ */
    }
}
```

## エラーコード

[errors.md](errors.md) を参照してください。V1 が返すのは `CT_OK`、`CT_ERR_NULL`、記号列のエラー（2〜7, 9）、
`CT_ERR_PARAM`（話速が範囲外）、`CT_ERR_MEMORY`（`ct1_synthe_wav` のみ）です。
1 回に渡せる記号列は 256 モーラ・64 アクセント句までです。

## 仕組み

[design.md](design.md) を参照してください。
