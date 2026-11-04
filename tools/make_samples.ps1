# 試聴用の WAV を samples/v1 に作る（先に Release ビルドしておくこと）
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$ctalk = Join-Path $root 'build\Release\ctalk.exe'
$out = Join-Path $root 'samples\v1'
New-Item -ItemType Directory -Force $out | Out-Null

$text = "わたしわ、からふるとーくです。むりょうで/つかえる/おんせいごうせいです。"
& $ctalk -o (Join-Path $out '01_hello.wav') "こんにちわ、せか'い。"
& $ctalk -o (Join-Path $out '02_question.wav') "ほんとうに/そ'うですか？"
foreach ($v in 'F1', 'F2', 'F3', 'M1', 'M2', 'C1', 'E1', 'W1', 'R1', 'K1') {
    & $ctalk -v $v -o (Join-Path $out "voice_$v.wav") $text
}
Write-Host "samples written to $out"
