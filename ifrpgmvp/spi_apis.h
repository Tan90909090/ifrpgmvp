#pragma once

static_assert(sizeof(void*) == 4, ".spi plug-in is intended for x86 environments.");

#include <pshpack1.h>
// 画像情報
struct PictureInfo {
	std::int32_t left; // 画像を展開する位置
	std::int32_t top; // 画像を展開する位置
	std::int32_t width; // 画像の幅(pixel)
	std::int32_t height; // 画像の高さ(pixel)
	std::uint16_t x_density; // 画素の水平方向密度
	std::uint16_t y_density; // 画素の垂直方向密度
	std::int16_t colorDepth; // 画素当たりのbit数

	/// <summary>画像内のテキスト情報</summary>
	/// <remarks>
	/// http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html
	/// PictureInfo 構造体の hInfo は、仕様には「Globalメモリーのハンドルが格納される」と書かれていますが、HLOCAL hInfoで間違いないようです。
	/// たけちん氏の作成した内蔵ドキュメントを使用するプラグイン（ifjpeg.spi ver0.24）は Global * API を使用していません。
	/// </remarks>
	HLOCAL hInfo;
};
#include <poppack.h>

// まず nNum==0 でコールされ、nNum==nDenom になるまで定期的に呼ばれる。
// 戻値が 非0 の時、Plug - inは処理を中断する。
using ProgressCallback = std::int32_t (PASCAL *)(std::int32_t nNum, std::int32_t nDenom, std::int32_t lData);

// エラーコード
enum class spi_result : std::int32_t {
	not_implemented = -1, // その機能はインプリメントされていない
	success = 0, // 正常終了
	aborted = 1, // コールバック関数が非0を返したので展開を中止した
	unknown_format = 2, // 未知のフォーマット
	broken_data = 3, // データが壊れている
	out_of_memory = 4, // メモリーが確保出来ない
	memory_error = 5, // メモリーエラー(Lock出来ない、等の場合とのこと http://www2f.biglobe.ne.jp/%7Ekana/spi_api/index.html )
	file_read_error = 6, // ファイルリードエラー
	reserved = 7, // （予約）
	internal_error = 8, // 内部エラー
};

// PASCAL呼び出し規則は__stdcallと同義であるが、Spi_api.txtでPASCALと使われているのでそのまま使う
// https://stackoverflow.com/questions/4550294/stdcall-name-mangling-using-extern-c-and-dllexport-vs-module-definitions-msvc
// `__declspec(dllexport)`を使う方法だとexport名がマングリングされていたので.defを使ってエクスポートする
#define SPI_API(result) extern "C" result PASCAL

// 共通関数
SPI_API(std::int32_t) GetPluginInfo(std::int32_t infono, LPSTR buf, std::int32_t buflen);

// '00IN'の関数
SPI_API(std::int32_t) IsSupported(LPSTR filename, std::uint32_t dw);
SPI_API(spi_result) GetPictureInfo(LPSTR buf, std::int32_t len, std::uint32_t flag, PictureInfo* lpInfo);
SPI_API(spi_result) GetPicture(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData);
SPI_API(spi_result) GetPreview(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData);
