# ifrpgmvp
## 概要
`Susie 32bit Plug-in 仕様 rev5`に基づいたSusieプラグインです。
本ソフトウェアで.rpgmvp形式のファイルを読み込むことが出来ます。

`Susie 32bit Plug-in 仕様 rev5`は[Susieのだうんろーど](https://www.digitalpad.co.jp/~takechin/download.html#spi)からダウンロードできる`Plug-in package ver0.08`中の`Spi_api.txt`にて定義されています。

## 使い方
[releases](https://github.com/Tan90909090/ifrpgmvp/releases)から最新版のZIPをダウンロードしてください。
ZIP中に以下のプラグインがありますので、お好みの方をアプリケーションに登録してご使用ください。
- `ifrpgmvp_32bpp.spi`は、出力が透明度付き32bppであるプラグインです。アプリケーション側が32bpp画像に対応しており、透明度にも対応している場合におすすめです。
- `ifrpgmvp_24bpp.spi`は、出力が24bppであるプラグインです。元画像と特定色でアルファブレンドした結果を出力します。アプリケーション側が32bpp画像に対応していないか、透明度に対応していない場合におすすめです。

## 仕様
- `ifrpgmvp_24bpp.spi`において、アルファブレンドする色は固定(マゼンタ)です。
- 元画像中のテキスト情報は無視されます。

## 動作確認
本ソフトウェアは以下のアプリケーションで動作確認しています。
- [Skymaker V2.60 エントランス版](http://files.in.coocan.jp/finekit/)
- [TEST SPI ver0.0.9](http://www.asahi-net.or.jp/~kh4s-smz/)
- [MassiGra Version 0.45](http://www.massigra.net/)

## ビルド手順
+ 開発環境に[Vcpkg](https://github.com/microsoft/vcpkg)をインストールします。[Buildsystem Integration](https://github.com/microsoft/vcpkg/blob/master/docs/users/integration.md#integrate)も実施します。
+ `.\vcpkg.exe install libpng:x86-windows-static` を実行し、libpngをインストールします。
+ 本ソフトウェアをビルドします。ビルド確認はVisual Studio 2019で行っています。

## 使用ライブラリ
本ソフトウェアは以下のソフトウェアを使用しています。
- [libpng](http://www.libpng.org/pub/png/libpng.html) ([PNG Reference Library License version 2](http://www.libpng.org/pub/png/src/libpng-LICENSE.txt))

## 参考ソフトウェア
本ソフトウェアは以下のソフトウェアを参考に開発しています。
- [RPG Maker MV CoreScript](https://github.com/rpgtkoolmv/corescript) ([MIT License](https://github.com/rpgtkoolmv/corescript/blob/master/LICENSE))

## ライセンス
本ソフトウェアはMITライセンスのもとで公開されています。
