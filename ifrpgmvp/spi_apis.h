#pragma once

static_assert(sizeof(void*) == 4, ".spi plug-in is intended for x86 environments.");

#include <pshpack1.h>
// �摜���
struct PictureInfo {
	std::int32_t left; // �摜��W�J����ʒu
	std::int32_t top; // �摜��W�J����ʒu
	std::int32_t width; // �摜�̕�(pixel)
	std::int32_t height; // �摜�̍���(pixel)
	std::uint16_t x_density; // ��f�̐����������x
	std::uint16_t y_density; // ��f�̐����������x
	std::int16_t colorDepth; // ��f�������bit��

	/// <summary>�摜���̃e�L�X�g���</summary>
	/// <remarks>
	/// http://www.asahi-net.or.jp/~kh4s-smz/spi/make_spi.html
	/// PictureInfo �\���̂� hInfo �́A�d�l�ɂ́uGlobal�������[�̃n���h�����i�[�����v�Ə�����Ă��܂����AHLOCAL hInfo�ŊԈႢ�Ȃ��悤�ł��B
	/// �������񎁂̍쐬���������h�L�������g���g�p����v���O�C���iifjpeg.spi ver0.24�j�� Global * API ���g�p���Ă��܂���B
	/// </remarks>
	HLOCAL hInfo;
};
#include <poppack.h>

// �܂� nNum==0 �ŃR�[������AnNum==nDenom �ɂȂ�܂Œ���I�ɌĂ΂��B
// �ߒl�� ��0 �̎��APlug - in�͏����𒆒f����B
using ProgressCallback = std::int32_t (PASCAL *)(std::int32_t nNum, std::int32_t nDenom, std::int32_t lData);

// �G���[�R�[�h
enum class spi_result : std::int32_t {
	not_implemented = -1, // ���̋@�\�̓C���v�������g����Ă��Ȃ�
	success = 0, // ����I��
	aborted = 1, // �R�[���o�b�N�֐�����0��Ԃ����̂œW�J�𒆎~����
	unknown_format = 2, // ���m�̃t�H�[�}�b�g
	broken_data = 3, // �f�[�^�����Ă���
	out_of_memory = 4, // �������[���m�ۏo���Ȃ�
	memory_error = 5, // �������[�G���[(Lock�o���Ȃ��A���̏ꍇ�Ƃ̂��� http://www2f.biglobe.ne.jp/%7Ekana/spi_api/index.html )
	file_read_error = 6, // �t�@�C�����[�h�G���[
	reserved = 7, // �i�\��j
	internal_error = 8, // �����G���[
};

// PASCAL�Ăяo���K����__stdcall�Ɠ��`�ł��邪�ASpi_api.txt��PASCAL�Ǝg���Ă���̂ł��̂܂܎g��
// https://stackoverflow.com/questions/4550294/stdcall-name-mangling-using-extern-c-and-dllexport-vs-module-definitions-msvc
// `__declspec(dllexport)`���g�����@����export�����}���O�����O����Ă����̂�.def���g���ăG�N�X�|�[�g����
#define SPI_API(result) extern "C" result PASCAL

// ���ʊ֐�
SPI_API(std::int32_t) GetPluginInfo(std::int32_t infono, LPSTR buf, std::int32_t buflen);

// '00IN'�̊֐�
SPI_API(std::int32_t) IsSupported(LPSTR filename, std::uint32_t dw);
SPI_API(spi_result) GetPictureInfo(LPSTR buf, std::int32_t len, std::uint32_t flag, PictureInfo* lpInfo);
SPI_API(spi_result) GetPicture(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData);
SPI_API(spi_result) GetPreview(LPSTR buf, std::int32_t len, std::uint32_t flag, HLOCAL* pHBInfo, HLOCAL* pHBm, ProgressCallback lpProgressCallback, std::int32_t lData);
