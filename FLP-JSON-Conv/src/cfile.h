#pragma once

#include <cassert>
#include <cstdio>
#include <vector>
#include <type_traits>


namespace Om {

class CFile {
	FILE* file_ptr;

public:
	static CFile open(char const* filename, char const* mode) noexcept {
		CFile ret { std::fopen(filename, mode) };
		return ret;
	}

	CFile() noexcept = default;

	CFile(FILE* fp) noexcept : file_ptr { fp } { }

	CFile(CFile const&) = delete;

	CFile(CFile&& other) noexcept :
		file_ptr { other.file_ptr } {
		other.file_ptr = nullptr;
	}

	CFile& operator=(CFile const&) = delete;

	CFile& operator=(CFile&& other) noexcept {
		if(this != &other) {
			close();
			file_ptr = other.file_ptr;
			other.file_ptr = nullptr;
		}
		return *this;
	}

	template<typename OutT>
	bool read(OutT* target) noexcept {
		static_assert(std::is_trivially_copyable<OutT>::value, "OutT must be trivially copyable!");
		assert(file_ptr != nullptr);
		std::size_t ret_fread = std::fread(target, sizeof(OutT), 1, file_ptr);
		return ret_fread != 0;
	}

	template<typename OutT>
	std::size_t read(OutT target[], std::size_t num_elems) noexcept {
		static_assert(std::is_trivially_copyable<OutT>::value, "OutT must be trivially copyable!");
		assert(file_ptr != nullptr);
		std::size_t ret_fread = std::fread(target, sizeof(OutT), num_elems, file_ptr);
		return ret_fread;
	}

	template<typename OutT, std::size_t N>
	std::size_t read(OutT(&target)[N]) noexcept {
		static_assert(std::is_trivially_copyable<OutT>::value, "OutT must be trivially copyable!");
		assert(file_ptr != nullptr);
		std::size_t ret_fread = std::fread(target, sizeof(OutT), N, file_ptr);
		return ret_fread;
	}

	template<typename InT>
	std::size_t write(InT data[], std::size_t num_elems) noexcept {
		static_assert(std::is_trivially_copyable<InT>::value, "InT must be trivially copyable!");
		assert(file_ptr != nullptr);
		std::size_t written = std::fwrite(data, sizeof(InT), num_elems, file_ptr);
		return written;
	}

	void close() noexcept {
		if(file_ptr) {
			int retval = std::fclose(file_ptr);
			static_cast<void>(retval);
			assert(retval == 0);
			file_ptr = nullptr;
		}
	}

	FILE* fptr() const noexcept {
		return file_ptr;
	}

	FILE* transfer_fptr() noexcept {
		auto ret = file_ptr;
		file_ptr = nullptr;
		return ret;
	}

	bool is_open() const noexcept {
		return file_ptr != nullptr;
	}

	bool eof() const noexcept {
		assert(file_ptr);
		return !!std::feof(file_ptr);
	}

	bool error() const noexcept {
		assert(file_ptr);
		return static_cast<bool>(std::ferror(file_ptr));
	}

	~CFile() {
		close();
	}
};

} // namespace Om

