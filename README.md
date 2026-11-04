# ColorfulTalk V1

16bit CPU から動く、無料（MIT ライセンス）の日本語音声合成エンジンです。
[ColorfulTalk シリーズ](https://github.com/ukagaka-irono/ColorfulTalk) の中で、声を 1 種類だけ組み込んだ小さな構成です。
より自然な声やパラメータでの声質調整が必要なら、後継の [ColorfulTalk V3](https://github.com/ukagaka-irono/ColorfulTalk-V3) を使ってください。

- 録音した声を使わない **フォルマント合成**。声データのライセンスを気にせず組み込んで配布できる
- **整数演算だけ** で動く。`int` が 16bit の CPU でも PC と同じ出力になる（AVR のシミュレータでビット単位の一致を確認）
- ROM 約 24KB、RAM 約 5.4KB（作業領域 4.9KB＋スタック）、動的メモリ不使用
- 入力はカナ記号列（V2・V3 と共通）。書き方は [docs/koe-kana.md](docs/koe-kana.md)
- 出力は 16bit PCM、8kHz、モノラル
- 声はビルド時に 1 種類を組み込む。標準は女声 `F1`。ほかに 9 種類（高めの女声・落ち着いた女声・男声・低い男声・子ども・年配・ささやき・ロボット・キャラクター声）

## 使い方

```c
#include "colorful_talk_v1.h"

static uint8_t work[5120];   /* ct1_work_size() 以上、4 バイト境界 */
int16_t buf[64];
int n;

if (ct1_start(work, "こんにちわ、せか'い。", 100) == CT_OK) {   /* 100 = 標準の話速 */
    while ((n = ct1_read(work, buf, 64)) > 0) {
        /* buf[0..n) を DAC へ（8kHz） */
    }
}
```

## 文書

| 文書 | 内容 |
|---|---|
| [docs/spec.md](docs/spec.md) | 仕様書（API、声、寸法、処理時間） |
| [docs/koe-kana.md](docs/koe-kana.md) | カナ記号列の書き方 |
| [docs/design.md](docs/design.md) | 内部設計（整数化の決まり、表、8kHz に合わせた変更） |
| [docs/intelligibility.md](docs/intelligibility.md) | 明瞭性の評価（V3 との比較） |
| [docs/errors.md](docs/errors.md) | エラーコード |

## ビルド

CMake 3.20 以上と C コンパイラが必要です。声は `-DCT1_VOICE=` で選びます（既定は `F1`）。

```bash
cmake -S . -B build -DCT1_VOICE=F1
```

```bash
cmake --build build --config Release
```

```bash
ctest --test-dir build -C Release
```

| 成果物 | 内容 |
|---|---|
| `colorful_talk_v1_static` | 静的ライブラリ（`ct_fx`・`ct_common` と一緒にリンク） |
| `colorful_talk_v1` | DLL / 共有ライブラリ |
| `ctalk` | 試聴・WAV 出力のツール（開発用に 10 種の声をすべて含み、`-v` で選べる） |

マイコンへ組み込むときは、次をプロジェクトに加えてください。

- ソース: `src/ct1_api.c`、`src/ct1_voices.c`、`fx/src/*.c`、`common/src/ct_koe.c`
- インクルードパス: `include/`、`src/`、`fx/include/`、`fx/src/`、`common/include/`
- 定義: `CT_NO_WAV`（PC 向けの WAV 関数を外す）、`CT1_VOICE_ID=0`〜`9`（声: F1 F2 F3 M1 M2 C1 E1 W1 R1 K1 の順）

## 16bit CPU での確認

`tests/avr/check.sh` で、V1 を ATmega1284P（`int` は 16bit）向けにビルドし、AVR シミュレータ（avr8js）で動かして
PC 版と出力を比べます。avr-gcc と Node.js（avr8js）が必要です。

```bash
AVR_GCC=/path/to/avr-gcc/bin AVR8JS=/path/to/avr8js-folder tests/avr/check.sh "こんにちわ、せか'い。"
```

`tools/make_samples.ps1` で試聴用の WAV（10 種の声）を `samples/v1/` に作れます。

## ライセンス

MIT License。詳細は [LICENSE](LICENSE)。
