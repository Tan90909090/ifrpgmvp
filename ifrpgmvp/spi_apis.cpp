#include "spi_apis.h"
#include "utils.h"

/// <summary>Plug-inに関する情報を得る。</summary>
/// <param name='infono'>
/// 取得する情報番号
/// 0   : Plug - in APIバージョン
/// 1   : Plug - in名、バージョン及び copyright(SusieのAbout..に表示されます)
/// 2n + 2 : 代表的な拡張子("*.JPG" "*.RGB;*.Q0" など)
/// 2n + 3 : ファイル形式名(ファイルタイプ名として使われます)
/// </param>
/// <param name='buf'>情報を納めるバッファ。</param>
/// <param name='buflen'>バッファ長(byte)。</param>
/// <returns>バッファに書き込んだ文字数を返します。情報番号が無効の場合、0を返します。</returns>
/// <remarks>
/// 情報番号0と1はすべてのバージョンで共通とします。
/// 2以降は二つづつ組みでSusieのOPENダイアログで用いる情報です。
/// 一つのplug - inで複数の画像フォーマットに対応している場合はその数だけ拡張子とファイル形式名を用意します。
/// </remarks>
SPI_API(std::int32_t) GetPluginInfo(std::int32_t infono, LPSTR buf, std::int32_t buflen) {
#ifdef IFRPGMVP_32BPP
#define BPP "32bpp"
#else
#define BPP "24bpp"
#endif
	using namespace std::literals::string_view_literals;
	constexpr static std::array infos = {
		"00IN"sv,
		"RPGMVP to " BPP " DIB filter ver 1.0.0 (compiled with libpng ver " PNG_LIBPNG_VER_STRING ")"sv,
		"*.rpgmvp"sv,
		"RPGMVP"sv,
	};
#undef BPP

	if (buf == nullptr || infono < 0 || infono >= static_cast<std::int32_t>(infos.size())) {
		return 0;
	}

	// NUL文字用に1要素は書き込める必要がある
	if (buflen <= 0) {
		return 0;
	}
	auto info = infos[infono];
	size_t write_length = std::min(static_cast<size_t>(buflen - 1), info.size());
	strncpy_s(buf, buflen, info.data(), write_length);
	buf[write_length] = '\0';
	return write_length;
}

/// <summary>展開可能な(対応している)ファイル形式か調べる。</summary>
/// <param name='filename'>ファイルネーム。</param>
/// <param name='dw'>
/// 上位ワードが  0  のとき:
///     ファイルハンドル。
/// 上位ワードが 非0 のとき:
///     ファイル先頭部(2Kbyte以上)を読み込んだバッファへのポインタ。
///     ファイルサイズが2Kbyte以下の場合もバッファは2Kbyte確保し、余分は 0 で埋めること。
/// </param>
/// <returns>対応している画像フォーマットであれば非0を返す</returns>
/// <remarks>
/// 各Plug - inは基本的に渡されたファイルのヘッダを調べ、自分の対応したファイルフォーマットであるかどうかを調べる。
/// まれにファイル名(拡張子)を判断材料として必要としたり、複数のファイルで構成されている場合があるので、ファイル名(フルパス)も引数に加えた。
/// 今回配布のPlug - inではfilenameは使われていない。
/// (注:"今回配布のPlug - in"とは"Susie 32bit plug-in library Ver0.08"のこと)
/// </remarks>
SPI_API(std::int32_t) IsSupported(LPSTR filename, std::uint32_t dw) {
	if (filename == nullptr) {
		return 0;
	}

	constexpr static int buffer_size = 2000; // "2Kbyte"は2KBなのか2KiBなのか？安全のため小さい2KBをとりあえず採用
	if (HIWORD(dw) == 0) {
		// ファイルハンドル
		auto h_file = reinterpret_cast<HANDLE>(dw);
		std::array<std::byte, buffer_size> buf{}; // "バッファは2Kbyte確保し、余分は 0 で埋めること。"
		DWORD read_bytes = 0;
		if (!ReadFile(h_file, buf.data(), buf.size(), &read_bytes, nullptr)) {
			return 0;
		}
		return utils::has_rpgmvp_header(buf.data(), buf.size());
	}
	else {
		// バッファへのポインタ
		auto buf = reinterpret_cast<std::byte*>(dw);
		return utils::has_rpgmvp_header(buf, buffer_size);
	}
}

/// <summary>画像ファイルに関する情報を得る。</summary>
/// <param name='buf'>入力がファイルの場合 ファイル名、メモリーの場合 ファイルイメージへのポインタ。</param>
/// <param name='len'>入力がファイルの場合 読込み開始オフセット(MacBin対応のため)、メモリーの場合 データサイズ。</param>
/// <param name='flag'>
/// 追加情報 xxxx xxxx xxxx xSSS
///     SSS : 入力形式
///         000 : ディスクファイル
///         001 : メモリ上のイメージ
/// </param>
/// <param name='lpInfo'>画像情報。</param>
/// <returns>エラーコード。0なら正常終了。</returns>
SPI_API(spi_result) GetPictureInfo(LPSTR buf, std::int32_t len, std::uint32_t flag, PictureInfo* lpInfo) {
	if (buf == nullptr || len < 0 || lpInfo == nullptr) {
		return spi_result::internal_error;
	}

	return utils::get_picture_helper_with_exception_handling(
		buf,
		len,
		flag,
		[lpInfo](std::byte* image_buf, std::size_t image_len) {
			return utils::rpvmvp_to_png_helper(
				image_buf,
				image_len,
				[lpInfo](const std::byte* png_buf, std::size_t png_len) {
					utils::png_image_wrapper wrapper;
					wrapper.begin_read(png_buf, png_len);

					lpInfo->left = 0;
					lpInfo->top = 0;
					lpInfo->width = static_cast<std::int32_t>(wrapper.image().width); // pngの幅、高さは2^31未満と保証されているのでキャストは常に成功する
					lpInfo->height = static_cast<std::int32_t>(wrapper.image().height);
					lpInfo->x_density = 0;
					lpInfo->y_density = 0;
					lpInfo->colorDepth = 8 * PNG_IMAGE_SAMPLE_SIZE(wrapper.image().format);
					lpInfo->hInfo = nullptr; // テキスト情報はめったに無いだろうからひとまず非対応

					return spi_result::success;
				});
		});
}

/// <summary>画像を展開する。</summary>
/// <summary>
/// </summary>
/// <param name='buf'>入力がファイルの場合 ファイル名、メモリーの場合 ファイルイメージへのポインタ。</param>
/// <param name='len'>入力がファイルの場合 読込み開始オフセット(MacBin対応のため)、メモリーの場合 データサイズ。</param>
/// <param name='flag'>
/// 追加情報 xxxx xxxx xxxx xSSS
///     SSS : 入力形式
///         000 : ディスクファイル
///         001 : メモリ上のイメージ
/// </param>
/// <param name='pHBInfo'>BITMAPINFO 構造体が納められたメモリハンドルが返される。</param>
/// <param name='pHBm'>ビットマップデータ本体のメモリハンドルが返される。</param>
/// <param name='lpProgressCallback'>
/// 途中経過を表示するコールバック関数へのポインタ。
/// NULLの場合、plug-inは処理が終了するまでプロセスを占有し、中断も出来ません。
/// </param>
/// <param name='lData'>コールバック関数に渡すlongデータ。ポインタ等を必要に応じて受け渡せる。</param>
/// <returns>エラーコード。0なら正常終了。</returns>
/// <remarks>
/// プラグインはLocalAllocによって必要なメモリーを確保し、そのハンドルを返す。
/// アプリケーションはLocalFreeによってメモリーを開放する必要がある。
/// </remarks>
SPI_API(spi_result) GetPicture(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData) {
	if (buf == nullptr || len < 0 || pHBInfo == nullptr || pHBm == nullptr) {
		return spi_result::internal_error;
	}

	constexpr static int expected_notify_progress_count = 3;
	return utils::get_picture_helper_with_exception_handling(
		buf,
		len,
		flag,
		[pHBInfo, pHBm, lpProgressCallback, lData](std::byte* image_buf, std::size_t image_len) {
			return utils::rpvmvp_to_png_helper(
				image_buf,
				image_len,
				[pHBInfo, pHBm, lpProgressCallback, lData](const std::byte* png_buf, std::size_t png_len) {
					constexpr static int expected_notify_progress_count = 3;
					int current_notify_progress_count = 0;
					auto notify_progress_and_throw_if_necessary = [&current_notify_progress_count, lpProgressCallback, lData]() {
						if (lpProgressCallback != nullptr) {
							if (lpProgressCallback(current_notify_progress_count, expected_notify_progress_count - 1, lData) != 0) {
								throw utils::spi_error(spi_result::aborted);
							}
						}
						current_notify_progress_count++;
						assert(current_notify_progress_count <= expected_notify_progress_count);
					};

					notify_progress_and_throw_if_necessary();

					utils::png_image_wrapper wrapper;
					wrapper.begin_read(png_buf, png_len);

					constexpr static png_uint_32 color_format =
#ifdef IFRPGMVP_32BPP
						PNG_FORMAT_RGBA;
#else
						PNG_FORMAT_BGR;
#endif
					constexpr static png_uint_32 colors = PNG_IMAGE_SAMPLE_SIZE(color_format);
					const size_t padding = colors == 4
						? 0 // RGBA時は1pxが4byteなのでに画像幅によらずpaddingが0になる
						: [&wrapper]() {
							size_t row_bytes = colors * wrapper.image().width;
							return row_bytes % 4 == 0 ? 0 : 4 - (row_bytes % 4);
						}();

					auto decoded_buf = std::make_unique<std::byte[]>(colors * wrapper.image().width * wrapper.image().height);
					constexpr png_color background{ 0xFF, 0x00, 0xFF }; // RGBA時では使用されない
					wrapper.finish_read(color_format, &background, decoded_buf.get(), true);

					notify_progress_and_throw_if_necessary();

					constexpr static int bmp_header_size =
						colors == 4
						? sizeof(BITMAPV4HEADER) + (3 * sizeof(DWORD))
						: sizeof(BITMAPINFOHEADER);
					utils::hlocal_allocator<BITMAPINFOHEADER> h_bmp_info_header{ bmp_header_size };
					auto bmp_info_header = h_bmp_info_header.lock();

					const size_t bmp_length = wrapper.image().height * ((colors * wrapper.image().width) + padding);
					utils::hlocal_allocator<std::byte> h_bmp_buf{ bmp_length };
					auto bmp_buf = h_bmp_buf.lock();

					if (colors == 4) {
						// https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-header-types
						// 透明度付 BMP のフォーマット http://files.in.coocan.jp/finekit/transbmp.html
						// 透明度付 BMP のフォーマット(2) http://files.in.coocan.jp/finekit/transbmp2.html
						// BMPファイルフォーマット（Windows拡張） - 画像ファイル入出力 - 碧色工房 https://www.mm2d.net/main/prog/c/image_io-09.html 

						auto header = static_cast<BITMAPV4HEADER*>(static_cast<void*>(bmp_info_header.get()));
						utils::set_bitmap_info_header(bmp_info_header.get(), sizeof(*header), wrapper.image(), 32, BI_BITFIELDS);
						header->bV4RedMask   = 0x000000FF; // little-endian
						header->bV4GreenMask = 0x0000FF00;
						header->bV4BlueMask  = 0x00FF0000;
						header->bV4AlphaMask = 0xFF000000;
						header->bV4CSType = LCS_sRGB; // 以降のフィールドは、bV4CSTypeがLCS_CALIBRATED_RGB以外の場合は無視される
						header->bV4Endpoints = CIEXYZTRIPLE{};
						header->bV4GammaRed = 1;
						header->bV4GammaGreen = 1;
						header->bV4GammaBlue = 1;

						auto dsBitfield = static_cast<DWORD*>(static_cast<void*>(
							static_cast<std::byte*>(static_cast<void*>(bmp_info_header.get())) + sizeof(*header)
							));
						dsBitfield[0] = 0x000000FF;
						dsBitfield[1] = 0x0000FF00;
						dsBitfield[2] = 0x00FF0000;

						// BITMAPV4なら画素をRGBAの順に並べられて、α値も含むことができる
						// また各画素が4byteなのでDIBの行ごとのアライメントも自動的に達成される
						// つまりlibpngの出力をそのままsusieアプリケーションに渡せる
						memcpy(bmp_buf.get(), decoded_buf.get(), bmp_length);
					}
					else {
						auto header = bmp_info_header.get();
						utils::set_bitmap_info_header(header, sizeof(*header), wrapper.image(), 24, BI_RGB);

						int dst_index = 0;
						int src_index = 0;
						for (size_t y = 0; y < wrapper.image().height; ++y) {
							for (size_t x = 0; x < wrapper.image().width; ++x) {
								// formatがBGRなので同じ順番になっている
								bmp_buf[dst_index + 0] = decoded_buf[src_index + 0];
								bmp_buf[dst_index + 1] = decoded_buf[src_index + 1];
								bmp_buf[dst_index + 2] = decoded_buf[src_index + 2];
								dst_index += 3;
								src_index += 3;
							}
							for (size_t i = 0; i < padding; ++i) {
								bmp_buf[dst_index++] = static_cast<std::byte>(0x00);
							}
						}
					}

					notify_progress_and_throw_if_necessary();
					assert(current_notify_progress_count == expected_notify_progress_count);

					*pHBInfo = h_bmp_info_header.release();
					*pHBm = h_bmp_buf.release();
					return spi_result::success;
				});
		});
}

/// <summary>プレビュー・カタログ表示用画像縮小展開ルーティン。</summary>
/// <param name='buf'>入力がファイルの場合 ファイル名、メモリーの場合 ファイルイメージへのポインタ。</param>
/// <param name='len'>入力がファイルの場合 読込み開始オフセット(MacBin対応のため)、メモリーの場合 データサイズ。</param>
/// <param name='flag'>
/// 追加情報 xxxx xxxx xxxx xSSS
///     SSS : 入力形式
///         000 : ディスクファイル
///         001 : メモリ上のイメージ
/// </param>
/// <param name='pHBInfo'>BITMAPINFO 構造体が納められたメモリハンドルが返される。</param>
/// <param name='pHBm'>ビットマップデータ本体のメモリハンドルが返される。</param>
/// <param name='lpProgressCallback'>
/// 途中経過を表示するコールバック関数へのポインタ。
/// NULLの場合、plug-inは処理が終了するまでプロセスを占有し、中断も出来ません。
/// </param>
/// <param name='lData'>コールバック関数に渡すlongデータ。ポインタ等を必要に応じて受け渡せる。</param>
/// <returns>エラーコード。0なら正常終了。この関数はオプションであり、未対応の場合は - 1 を返す。</returns>
/// <remarks>
/// プレビュー等で用いる縮小された画像をファイルから作成する。
/// JPEGの様に、アルゴリズムの関係で縮小されたサイズでは高速に展開出来るときにこの関数をインプリメントす/る 。
/// 今回配布のPlug - inでは IFJPEG.PLG のみ対応(1 / 4サイズで展開)している。
/// 未対応の場合、Susieは通常の展開ルーティンを用いて展開した後縮小処理を行う。
/// （対応していても縮小ロードされた画像を更にサイズ調整している）
/// プラグインはLocalAllocによって必要なメモリーを確保し、そのハンドルを返す。
/// アプリケーションはLocalFreeによってメモリーを開放する必要がある。
/// </remarks>
SPI_API(spi_result) GetPreview(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData) {
	return spi_result::not_implemented; // 通常の画像展開処理にフォールバック
}
