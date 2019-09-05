#include "spi_apis.h"
#include "utils.h"

/// <summary>Plug-in�Ɋւ�����𓾂�B</summary>
/// <param name='infono'>
/// �擾������ԍ�
/// 0   : Plug - in API�o�[�W����
/// 1   : Plug - in���A�o�[�W�����y�� copyright(Susie��About..�ɕ\������܂�)
/// 2n + 2 : ��\�I�Ȋg���q("*.JPG" "*.RGB;*.Q0" �Ȃ�)
/// 2n + 3 : �t�@�C���`����(�t�@�C���^�C�v���Ƃ��Ďg���܂�)
/// </param>
/// <param name='buf'>����[�߂�o�b�t�@�B</param>
/// <param name='buflen'>�o�b�t�@��(byte)�B</param>
/// <returns>�o�b�t�@�ɏ������񂾕�������Ԃ��܂��B���ԍ��������̏ꍇ�A0��Ԃ��܂��B</returns>
/// <remarks>
/// ���ԍ�0��1�͂��ׂẴo�[�W�����ŋ��ʂƂ��܂��B
/// 2�ȍ~�͓�Âg�݂�Susie��OPEN�_�C�A���O�ŗp������ł��B
/// ���plug - in�ŕ����̉摜�t�H�[�}�b�g�ɑΉ����Ă���ꍇ�͂��̐������g���q�ƃt�@�C���`������p�ӂ��܂��B
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

	// NUL�����p��1�v�f�͏������߂�K�v������
	if (buflen <= 0) {
		return 0;
	}
	auto info = infos[infono];
	size_t write_length = std::min(static_cast<size_t>(buflen - 1), info.size());
	strncpy_s(buf, buflen, info.data(), write_length);
	buf[write_length] = '\0';
	return write_length;
}

/// <summary>�W�J�\��(�Ή����Ă���)�t�@�C���`�������ׂ�B</summary>
/// <param name='filename'>�t�@�C���l�[���B</param>
/// <param name='dw'>
/// ��ʃ��[�h��  0  �̂Ƃ�:
///     �t�@�C���n���h���B
/// ��ʃ��[�h�� ��0 �̂Ƃ�:
///     �t�@�C���擪��(2Kbyte�ȏ�)��ǂݍ��񂾃o�b�t�@�ւ̃|�C���^�B
///     �t�@�C���T�C�Y��2Kbyte�ȉ��̏ꍇ���o�b�t�@��2Kbyte�m�ۂ��A�]���� 0 �Ŗ��߂邱�ƁB
/// </param>
/// <returns>�Ή����Ă���摜�t�H�[�}�b�g�ł���Δ�0��Ԃ�</returns>
/// <remarks>
/// �ePlug - in�͊�{�I�ɓn���ꂽ�t�@�C���̃w�b�_�𒲂ׁA�����̑Ή������t�@�C���t�H�[�}�b�g�ł��邩�ǂ����𒲂ׂ�B
/// �܂�Ƀt�@�C����(�g���q)�𔻒f�ޗ��Ƃ��ĕK�v�Ƃ�����A�����̃t�@�C���ō\������Ă���ꍇ������̂ŁA�t�@�C����(�t���p�X)�������ɉ������B
/// ����z�z��Plug - in�ł�filename�͎g���Ă��Ȃ��B
/// (��:"����z�z��Plug - in"�Ƃ�"Susie 32bit plug-in library Ver0.08"�̂���)
/// </remarks>
SPI_API(std::int32_t) IsSupported(LPSTR filename, std::uint32_t dw) {
	if (filename == nullptr) {
		return 0;
	}

	constexpr static int buffer_size = 2000; // "2Kbyte"��2KB�Ȃ̂�2KiB�Ȃ̂��H���S�̂��ߏ�����2KB���Ƃ肠�����̗p
	if (HIWORD(dw) == 0) {
		// �t�@�C���n���h��
		auto h_file = reinterpret_cast<HANDLE>(dw);
		std::array<std::byte, buffer_size> buf{}; // "�o�b�t�@��2Kbyte�m�ۂ��A�]���� 0 �Ŗ��߂邱�ƁB"
		DWORD read_bytes = 0;
		if (!ReadFile(h_file, buf.data(), buf.size(), &read_bytes, nullptr)) {
			return 0;
		}
		return utils::has_rpgmvp_header(buf.data(), buf.size());
	}
	else {
		// �o�b�t�@�ւ̃|�C���^
		auto buf = reinterpret_cast<std::byte*>(dw);
		return utils::has_rpgmvp_header(buf, buffer_size);
	}
}

/// <summary>�摜�t�@�C���Ɋւ�����𓾂�B</summary>
/// <param name='buf'>���͂��t�@�C���̏ꍇ �t�@�C�����A�������[�̏ꍇ �t�@�C���C���[�W�ւ̃|�C���^�B</param>
/// <param name='len'>���͂��t�@�C���̏ꍇ �Ǎ��݊J�n�I�t�Z�b�g(MacBin�Ή��̂���)�A�������[�̏ꍇ �f�[�^�T�C�Y�B</param>
/// <param name='flag'>
/// �ǉ���� xxxx xxxx xxxx xSSS
///     SSS : ���͌`��
///         000 : �f�B�X�N�t�@�C��
///         001 : ��������̃C���[�W
/// </param>
/// <param name='lpInfo'>�摜���B</param>
/// <returns>�G���[�R�[�h�B0�Ȃ琳��I���B</returns>
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
					lpInfo->width = static_cast<std::int32_t>(wrapper.image().width); // png�̕��A������2^31�����ƕۏ؂���Ă���̂ŃL���X�g�͏�ɐ�������
					lpInfo->height = static_cast<std::int32_t>(wrapper.image().height);
					lpInfo->x_density = 0;
					lpInfo->y_density = 0;
					lpInfo->colorDepth = 8 * PNG_IMAGE_SAMPLE_SIZE(wrapper.image().format);
					lpInfo->hInfo = nullptr; // �e�L�X�g���͂߂����ɖ������낤����ЂƂ܂���Ή�

					return spi_result::success;
				});
		});
}

/// <summary>�摜��W�J����B</summary>
/// <summary>
/// </summary>
/// <param name='buf'>���͂��t�@�C���̏ꍇ �t�@�C�����A�������[�̏ꍇ �t�@�C���C���[�W�ւ̃|�C���^�B</param>
/// <param name='len'>���͂��t�@�C���̏ꍇ �Ǎ��݊J�n�I�t�Z�b�g(MacBin�Ή��̂���)�A�������[�̏ꍇ �f�[�^�T�C�Y�B</param>
/// <param name='flag'>
/// �ǉ���� xxxx xxxx xxxx xSSS
///     SSS : ���͌`��
///         000 : �f�B�X�N�t�@�C��
///         001 : ��������̃C���[�W
/// </param>
/// <param name='pHBInfo'>BITMAPINFO �\���̂��[�߂�ꂽ�������n���h�����Ԃ����B</param>
/// <param name='pHBm'>�r�b�g�}�b�v�f�[�^�{�̂̃������n���h�����Ԃ����B</param>
/// <param name='lpProgressCallback'>
/// �r���o�߂�\������R�[���o�b�N�֐��ւ̃|�C���^�B
/// NULL�̏ꍇ�Aplug-in�͏������I������܂Ńv���Z�X���L���A���f���o���܂���B
/// </param>
/// <param name='lData'>�R�[���o�b�N�֐��ɓn��long�f�[�^�B�|�C���^����K�v�ɉ����Ď󂯓n����B</param>
/// <returns>�G���[�R�[�h�B0�Ȃ琳��I���B</returns>
/// <remarks>
/// �v���O�C����LocalAlloc�ɂ���ĕK�v�ȃ������[���m�ۂ��A���̃n���h����Ԃ��B
/// �A�v���P�[�V������LocalFree�ɂ���ă������[���J������K�v������B
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
						? 0 // RGBA����1px��4byte�Ȃ̂łɉ摜���ɂ�炸padding��0�ɂȂ�
						: [&wrapper]() {
							size_t row_bytes = colors * wrapper.image().width;
							return row_bytes % 4 == 0 ? 0 : 4 - (row_bytes % 4);
						}();

					auto decoded_buf = std::make_unique<std::byte[]>(colors * wrapper.image().width * wrapper.image().height);
					constexpr png_color background{ 0xFF, 0x00, 0xFF }; // RGBA���ł͎g�p����Ȃ�
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
						// �����x�t BMP �̃t�H�[�}�b�g http://files.in.coocan.jp/finekit/transbmp.html
						// �����x�t BMP �̃t�H�[�}�b�g(2) http://files.in.coocan.jp/finekit/transbmp2.html
						// BMP�t�@�C���t�H�[�}�b�g�iWindows�g���j - �摜�t�@�C�����o�� - �ɐF�H�[ https://www.mm2d.net/main/prog/c/image_io-09.html 

						auto header = static_cast<BITMAPV4HEADER*>(static_cast<void*>(bmp_info_header.get()));
						utils::set_bitmap_info_header(bmp_info_header.get(), sizeof(*header), wrapper.image(), 32, BI_BITFIELDS);
						header->bV4RedMask   = 0x000000FF; // little-endian
						header->bV4GreenMask = 0x0000FF00;
						header->bV4BlueMask  = 0x00FF0000;
						header->bV4AlphaMask = 0xFF000000;
						header->bV4CSType = LCS_sRGB; // �ȍ~�̃t�B�[���h�́AbV4CSType��LCS_CALIBRATED_RGB�ȊO�̏ꍇ�͖��������
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

						// BITMAPV4�Ȃ��f��RGBA�̏��ɕ��ׂ��āA���l���܂ނ��Ƃ��ł���
						// �܂��e��f��4byte�Ȃ̂�DIB�̍s���Ƃ̃A���C�����g�������I�ɒB�������
						// �܂�libpng�̏o�͂����̂܂�susie�A�v���P�[�V�����ɓn����
						memcpy(bmp_buf.get(), decoded_buf.get(), bmp_length);
					}
					else {
						auto header = bmp_info_header.get();
						utils::set_bitmap_info_header(header, sizeof(*header), wrapper.image(), 24, BI_RGB);

						int dst_index = 0;
						int src_index = 0;
						for (size_t y = 0; y < wrapper.image().height; ++y) {
							for (size_t x = 0; x < wrapper.image().width; ++x) {
								// format��BGR�Ȃ̂œ������ԂɂȂ��Ă���
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

/// <summary>�v���r���[�E�J�^���O�\���p�摜�k���W�J���[�e�B���B</summary>
/// <param name='buf'>���͂��t�@�C���̏ꍇ �t�@�C�����A�������[�̏ꍇ �t�@�C���C���[�W�ւ̃|�C���^�B</param>
/// <param name='len'>���͂��t�@�C���̏ꍇ �Ǎ��݊J�n�I�t�Z�b�g(MacBin�Ή��̂���)�A�������[�̏ꍇ �f�[�^�T�C�Y�B</param>
/// <param name='flag'>
/// �ǉ���� xxxx xxxx xxxx xSSS
///     SSS : ���͌`��
///         000 : �f�B�X�N�t�@�C��
///         001 : ��������̃C���[�W
/// </param>
/// <param name='pHBInfo'>BITMAPINFO �\���̂��[�߂�ꂽ�������n���h�����Ԃ����B</param>
/// <param name='pHBm'>�r�b�g�}�b�v�f�[�^�{�̂̃������n���h�����Ԃ����B</param>
/// <param name='lpProgressCallback'>
/// �r���o�߂�\������R�[���o�b�N�֐��ւ̃|�C���^�B
/// NULL�̏ꍇ�Aplug-in�͏������I������܂Ńv���Z�X���L���A���f���o���܂���B
/// </param>
/// <param name='lData'>�R�[���o�b�N�֐��ɓn��long�f�[�^�B�|�C���^����K�v�ɉ����Ď󂯓n����B</param>
/// <returns>�G���[�R�[�h�B0�Ȃ琳��I���B���̊֐��̓I�v�V�����ł���A���Ή��̏ꍇ�� - 1 ��Ԃ��B</returns>
/// <remarks>
/// �v���r���[���ŗp����k�����ꂽ�摜���t�@�C������쐬����B
/// JPEG�̗l�ɁA�A���S���Y���̊֌W�ŏk�����ꂽ�T�C�Y�ł͍����ɓW�J�o����Ƃ��ɂ��̊֐����C���v�������g��/�� �B
/// ����z�z��Plug - in�ł� IFJPEG.PLG �̂ݑΉ�(1 / 4�T�C�Y�œW�J)���Ă���B
/// ���Ή��̏ꍇ�ASusie�͒ʏ�̓W�J���[�e�B����p���ēW�J������k���������s���B
/// �i�Ή����Ă��Ă��k�����[�h���ꂽ�摜���X�ɃT�C�Y�������Ă���j
/// �v���O�C����LocalAlloc�ɂ���ĕK�v�ȃ������[���m�ۂ��A���̃n���h����Ԃ��B
/// �A�v���P�[�V������LocalFree�ɂ���ă������[���J������K�v������B
/// </remarks>
SPI_API(spi_result) GetPreview(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData) {
	return spi_result::not_implemented; // �ʏ�̉摜�W�J�����Ƀt�H�[���o�b�N
}
