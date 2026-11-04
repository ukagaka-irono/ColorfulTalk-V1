# ColorfulTalk V1

16bit CPU から動く、無料（MIT ライセンス）の日本語音声合成エンジンです。
[ColorfulTalk シリーズ](https://github.com/ukagaka-irono/ColorfulTalk) の中で、声を 1 種類だけ組み込んだ小さな構成です。

> **状態: 設計中。** ソースコードはまだありません。仕様は [docs/spec.md](docs/spec.md) で検討しています。
> すぐに使いたい場合は後継の [ColorfulTalk V3](https://github.com/ukagaka-irono/ColorfulTalk-V3) を使ってください。

## 特長（予定）

- 整数演算だけで動く。`int` が 16bit の CPU でも動く
- 入力はカナ記号列（V2・V3 と共通）。書き方は [docs/koe-kana.md](docs/koe-kana.md)
- 出力は 16bit PCM、8kHz、モノラル
- RAM 8KB 以下、ROM 100KB 以下（声 1 種類込み）
- 声は 1 種類を組み込む。標準は女声で、ほかに 9 種類から選んでビルドできる。声を変えるときはライブラリを差し替える

## 配布の予定

| 対象 | 配布物 |
|---|---|
| Windows / macOS / Linux | 声ごとにビルド済みのライブラリとヘッダ |
| OS のない組み込み機器 | ソースコード一式（声を選んでビルド） |

MIT ライセンスなので、声や関数の形を変えたいときは自由に改変できます。

## 文書

| 文書 | 内容 |
|---|---|
| [docs/spec.md](docs/spec.md) | 仕様書（案） |
| [docs/koe-kana.md](docs/koe-kana.md) | カナ記号列 |
| [docs/errors.md](docs/errors.md) | エラーコード |

## ライセンス

MIT License。詳細は [LICENSE](LICENSE)。
