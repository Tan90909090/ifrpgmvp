#pragma once

#include "spi_apis.h"

namespace utils {
	/// <summary>std::array&lt;std::byte, N&gt;���\�z���܂��B</summary>
	/// <seealso>https://stackoverflow.com/questions/45172052/correct-way-to-initialize-a-container-of-stdbyte</seealso>
	template<typename... Ts>
	constexpr inline std::array<std::byte, sizeof...(Ts)> make_bytes(Ts&& ... args)  {
		return{ std::byte(std::forward<Ts>(args))... };
	}

	// unique_ptr��deleter�p�֐��I�u�W�F�N�g
	struct close_handle_deleter {
		using pointer = HANDLE;
		void operator() (HANDLE handle) const noexcept {
			if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
			}
		}
	};

	// �v���O�C���pAPI�Ŕ���������O��\���܂��B
	class spi_error : public std::runtime_error {
		spi_result result_;
	public:
		// F12�Ō��Ă����ƁAstd::exception�ɂ�noexcept�w�肪���Ă���̂ɁAstd::runtime_error�ɂ�noexpect�w�肪�Ȃ��͉̂��́c�c
		explicit spi_error(spi_result result) noexcept : std::runtime_error(""), result_(result) {
		}

		spi_result result() const noexcept {
			return result_; 
		}
	};

	// LocalLock, LocalUnlock���J�v�Z�������܂��BLocalLock���s����spi_error(spi_result::memory_error)�𑗏o���܂��B
	template<class T>
	class hlocal_locker {
		HLOCAL h_mem_;
		void* buf_;
	public:
		explicit hlocal_locker(HLOCAL h_mem) : h_mem_(h_mem), buf_(LocalLock(h_mem)) {
			if (buf_ == nullptr) {
				throw spi_error(spi_result::memory_error);
			}
		}
		~hlocal_locker() {
			if (buf_ != nullptr) {
				LocalUnlock(h_mem_);
			}
		}
		hlocal_locker(const hlocal_locker&) = delete;
		hlocal_locker& operator =(const hlocal_locker&) = delete;

		T* get() noexcept { return static_cast<T*>(buf_); }
		T& operator *() noexcept { return *get(); }
		T* operator ->() noexcept { return get(); }
		T& operator [](std::size_t i) noexcept { return get()[i]; }
	};

	// LocalAlloc, LocalFree���J�v�Z�������܂��BLocalAlloc���s���ɂ�spi_error(spi_result::out_of_memory)��throw���܂��B
	template<class T>
	class hlocal_allocator {
		HLOCAL h_mem_;
	public:
		explicit hlocal_allocator(size_t size) : h_mem_(LocalAlloc(LMEM_FIXED, size))  {
			if (h_mem_ == nullptr) {
				throw spi_error(spi_result::out_of_memory);
			}
		}
		~hlocal_allocator() {
			LocalFree(h_mem_);
		}
		hlocal_allocator(const hlocal_allocator&) = delete;
		hlocal_allocator& operator =(const hlocal_allocator&) = delete;

		// release���nullptr�ł��B
		HLOCAL get() noexcept { return h_mem_; }
		HLOCAL release() noexcept {
			auto result = h_mem_;
			h_mem_ = nullptr;
			return result;
		}

		hlocal_locker<T> lock() { return hlocal_locker<T>{ h_mem_ }; }
	};

	// libpng�ň������\�[�X��������܂��B
	class read_struct_destroyer {
		png_structpp png_ptr_ptr_;
		png_infopp info_ptr_ptr_;
		png_infopp end_info_ptr_ptr_;
	public:
		read_struct_destroyer(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr, png_infopp end_info_ptr_ptr) noexcept
			: png_ptr_ptr_(png_ptr_ptr), info_ptr_ptr_(info_ptr_ptr), end_info_ptr_ptr_(end_info_ptr_ptr) {
		}

		~read_struct_destroyer() {
			if (png_ptr_ptr_  != nullptr && *png_ptr_ptr_ != nullptr) {
				png_destroy_read_struct(png_ptr_ptr_, info_ptr_ptr_, end_info_ptr_ptr_);
			}
		}

		read_struct_destroyer(const read_struct_destroyer&) = delete;
		read_struct_destroyer& operator =(const read_struct_destroyer&) = delete;
	};

	// png_image�̃��b�p�[�ł��B
	class png_image_wrapper {
		png_image image_;
	public:
		png_image_wrapper() {
			image_.version = PNG_IMAGE_VERSION;
			image_.opaque = nullptr; // "this is REQUIRED, your program may crash if you don't do it."
		}
		~png_image_wrapper() {
			png_image_free(&image_);
		}

		// ���s���ɂ�spi_error(spi_result::broken_data)��throw���܂��B
		void begin_read(const void* buf, size_t len) {
			if (png_image_begin_read_from_memory(&image_, buf, len) == 0) {
				throw spi_error(spi_result::broken_data);
			}
		}

		// ���s���ɂ�spi_error(spi_result::broken_data)��throw���܂��B
		void finish_read(png_uint_32 format, png_const_colorp background, void* buffer, bool bottom_to_up)  {
			// "A negative stride indicates that the bottom - most row is first in the buffer."
			// ��BMP���̏����ƍ����̂ŁA�����Ɋ��p�����Ă��炤
			image_.format = format;
			auto row_stride_abs = static_cast<png_int_32>(image_.width * PNG_IMAGE_PIXEL_CHANNELS(image_.format)); // 0�w�莞�̃f�t�H���g�l
			auto row_stride = (bottom_to_up ? -1 : 1) * row_stride_abs;

			if (png_image_finish_read(&image_, background, buffer, row_stride, nullptr) == 0) {
				throw spi_error(spi_result::broken_data);
			}
		}

		const png_image& image() const noexcept { return image_; }

		png_image_wrapper(const png_image_wrapper&) = delete;
		png_image_wrapper& operator =(const png_image_wrapper&) = delete;
	};

	constexpr static auto rpgmvp_header = make_bytes(
		0x52, 0x50, 0x47, 0x4d, 0x56, 0x00, 0x00, 0x00, // signature, "RPGMV\0\0\0"
		0x00, 0x03, 0x01, // version
		0x00, 0x00, 0x00, 0x00, 0x00 // remain
	);
	static_assert(rpgmvp_header.size() == 16);

	constexpr static auto png_first_bytes = make_bytes(
		137, 80, 78, 71, 13, 10, 26, 10, // signature
		0, 0, 0, 13, // IHDR LENGTH(network byte order)
		73, 72, 68, 82 // IHDR CHUNK TYPE
	);
	static_assert(png_first_bytes.size() == 16);

	template<class T>
	spi_result get_picture_helper_with_exception_handling(LPSTR buf, std::int32_t len, std::uint32_t flag, T image_processor) {
		try {
			switch (flag & 0b111)
			{
			case 0:
			{
				// �f�B�X�N�t�@�C��
				std::unique_ptr<HANDLE, utils::close_handle_deleter> handle;
				handle.reset(CreateFileA(buf, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
				if (handle.get() == INVALID_HANDLE_VALUE) {
					return spi_result::file_read_error;
				}

				if (SetFilePointer(handle.get(), len, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
					return spi_result::file_read_error;
				}

				LARGE_INTEGER size;
				if (!GetFileSizeEx(handle.get(), &size)) {
					return spi_result::file_read_error;
				}

				// 32bit�R���p�C���Ȃ̂�size_t��4byte�ł���int64_t�ւ̃L���X�g�͏�ɐ�������
				auto size_to_read_ll = size.QuadPart - len;
				if (size_to_read_ll > static_cast<LONGLONG>(std::numeric_limits<std::size_t>::max())) {
					return spi_result::out_of_memory; // 32bit�v���Z�X�ł͊m�ەs��
				}

				auto size_to_read = static_cast<std::size_t>(size_to_read_ll);
				auto buf = std::make_unique<std::byte[]>(size_to_read);
				DWORD read_bytes = 0;
				if (!ReadFile(handle.get(), buf.get(), size_to_read, &read_bytes, nullptr)) {
					return spi_result::file_read_error;
				}

				return image_processor(buf.get(), read_bytes);
			}
			case 1:
				// ��������̃C���[�W
				return image_processor(
					static_cast<std::byte*>(static_cast<void*>(buf)),
					static_cast<std::size_t>(len));
			default:
				return spi_result::internal_error;
			}
		}
		catch (const std::bad_alloc&) {
			return spi_result::out_of_memory;
		}
		catch (const utils::spi_error& e) {
			return e.result();
		}
	}

	inline bool has_rpgmvp_header(const std::byte* buf, std::size_t len) noexcept {
		if (len < rpgmvp_header.size()) {
			return false;
		}

		return std::memcmp(buf, rpgmvp_header.data(), rpgmvp_header.size()) == 0;
	}

	template<class T>
	spi_result rpvmvp_to_png_helper(std::byte* buf, std::size_t len, T png_processor) {
		if (!has_rpgmvp_header(buf, len)) {
			return spi_result::unknown_format;
		}
		if (len < rpgmvp_header.size() + png_first_bytes.size()) {
			return spi_result::broken_data;
		}

		// png�w�b�_�[�̕���
		auto png_buf = buf + rpgmvp_header.size();
		memcpy(png_buf, png_first_bytes.data(), png_first_bytes.size());
		auto png_len = len - rpgmvp_header.size();

		return png_processor(png_buf, png_len);
	}

	template<class T>
	spi_result libpng_helper(std::byte* png_address, std::size_t png_length, T png_lib_processor) {
		if (png_sig_cmp(static_cast<png_const_bytep>(static_cast<void*>(png_address)), 0, 8) != 0) {
			return spi_result::internal_error; // �����������ł��Ă��Ȃ��Ȃ�w�b�_�[��`���Ԉ���Ă���
		}

		png_structp png_ptr = nullptr;
		png_infop info_ptr = nullptr;
		read_struct_destroyer destroyer{ &png_ptr, &info_ptr, nullptr };

		// �f�t�H���g�ł�stderr�ɃG���[���e��x�����e���o�͂����̂Ŏ���R�[���o�b�N�ɂ��}��
		png_ptr = png_create_read_struct(
			PNG_LIBPNG_VER_STRING,
			nullptr, // png_get_error_ptr()�Ŏ擾�ł���error function�p�̃��[�U�[��`�̓��e
			[](png_structp png_ptr, png_const_charp error_msg) { longjmp(png_jmpbuf(png_ptr), 1); }, // error_fn, must be [noreturn]
			[](png_structp png_ptr, png_const_charp warning_msg) {} // warning_fn
		); 
		if (png_ptr == nullptr) {
			return spi_result::out_of_memory;
		}

		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == nullptr) {
			return spi_result::out_of_memory;
		}

		if (setjmp(png_jmpbuf(png_ptr))) {
			return spi_result::broken_data;
		}

		struct callback_argument {
			std::byte* buf;
			std::size_t len;
			std::size_t current_offset;
		};
		callback_argument arg{ png_address, png_length, 0 };

		// FILE*����łȂ�����������R�s�[������
		png_set_read_fn(
			png_ptr,
			&arg,
			[](png_structp png_ptr, png_bytep data, png_size_t length) {
				// The user_read_data() function is responsible for detecting and handling end - of - data errors.
				auto arg = static_cast<callback_argument*>(png_get_io_ptr(png_ptr));
				if (arg->current_offset + length <= arg->len) {
					memcpy(data, arg->buf + arg->current_offset, length);
					arg->current_offset += length;
				}
				else {
					png_error(png_ptr, "Data size is too short.");
				}
			});
		png_read_info(png_ptr, info_ptr);

		return png_lib_processor(png_ptr, info_ptr);
	}

	inline void set_bitmap_info_header(BITMAPINFOHEADER* header, DWORD size, const png_image& image, WORD bit_count, DWORD compression) {
		header->biSize = size;
		header->biWidth = image.width;
		header->biHeight = image.height; // If biHeight is positive, the bitmap is a bottom-up DIB and its origin is the lower-left corner.
		header->biPlanes = 1;
		header->biBitCount = bit_count;
		header->biCompression = compression;
		header->biSizeImage = 0;
		header->biXPelsPerMeter = 0;
		header->biYPelsPerMeter = 0;
		header->biClrUsed = 0;
		header->biClrImportant = 0;
	};
}
