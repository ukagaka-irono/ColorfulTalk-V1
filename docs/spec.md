# ColorfulTalk V1 仕様書

> 状態: 案（設計中）。シリーズ全体の仕様書: [ColorfulTalk](https://github.com/ukagaka-irono/ColorfulTalk)（`docs/spec/v1.md` と同じ内容）

| 項目 | 内容 |
|---|---|
| 位置づけ | 16bit CPU から動く、声が 1 種類の小さなエンジン。後継は V3 |
| 対象 | 16bit 以上の CPU（`int` が 16bit でも動く）、Windows / macOS / Linux |
| 方式 | 規則合成（フォルマント合成）。V3 の方式を整数演算に移したもの |
| 入力 | カナ記号列（UTF-8）。[koe-kana.md](koe-kana.md) |
| 出力 | 16bit 符号付き PCM、モノラル、8kHz |
| 演算 | 整数のみ（固定小数点）。幅の決まった型（`int16_t` / `int32_t`）だけを使う |
| RAM 目標 | 8KB 以下（作業領域＋スタック） |
| ROM 目標 | 100KB 以下（コード＋声 1 種類） |
| 声 | ライブラリに 1 種類を組み込む。標準は女声。ほかに 9 種類から選んでビルドできる |
| ライセンス | MIT |

## 声

声はビルド時に 1 つ選びます。声を変えるときはライブラリを差し替えます（実行中には切り替えません）。
OS 向けには、声ごとにビルド済みのライブラリを配布します。

| 名前 | 声 |
|---|---|
| `F1`（標準） | 女声 |
| `F2` | 高めの女声 |
| `F3` | 落ち着いた女声 |
| `M1` | 男声 |
| `M2` | 低い男声 |
| `C1` | 子ども |
| `E1` | 年配の声 |
| `W1` | ささやき声 |
| `R1` | ロボット |
| `K1` | 高く明るいキャラクター声 |

ビルド時の選び方（案）: CMake の設定時に `-DCT1_VOICE=F1` を付ける（既定は `F1`）。

## API（案）

```c
#define CT1_WORK_SIZE  /* 作業領域のバイト数（ヘッダで定義） */

int ct1_start(void* work, const char* koe_utf8, int speed);   /* speed: 50〜300（100 = 標準） */
int ct1_read(void* work, int16_t* buf, int n);                /* 書いたサンプル数。0 で終わり */
int ct1_error_pos(const void* work);
const char* ct1_voice_name(void);                             /* 組み込んだ声の名前 */

/* PC 向け */
unsigned char* ct1_synthe_wav(const char* koe_utf8, int speed, int* size, int* err);
void           ct1_free(void* buf);
```

## 実装方針

- 合成コア（固定小数点）は V2 と共有する
  - 共振器: Q14 の係数、32bit の積和
  - 声帯音源: LF パルスを前計算した表（声ごと）
  - 声の高さ: 藤崎モデルを区分線形で近似
  - 乱数: 線形帰還シフトレジスタ
- 声は「母音・子音の目標値、音源表、韻律の係数」をまとめた構造体として ROM に置く
- 正しく動くことの確認: PC 版と、`int` が 16bit の AVR（シミュレータ）で出力がビット単位で一致すること
