#include "utf_conversions.h"
#include "win32.h"


namespace Om {

Om::Result<std::string> utf16_to_utf8(std::wstring_view strv) {
	if(strv.size() >= std::size_t(INT_MAX)) {
		return { std::make_error_code(std::errc::invalid_argument) };
	}
	wchar_t const* const wstr = strv.data();
	int const isize = static_cast<int>(strv.size());
	if(isize == 0) {
		return {};
	}
	int const chars_needed = ::WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		wstr, isize,
		nullptr, 0,
		nullptr, nullptr
	);
	if(chars_needed == 0) {
		return { std::error_code(::GetLastError(), std::system_category()) };
	}

	std::string utf8_str(std::size_t(chars_needed), '\0');

	int const chars_written = ::WideCharToMultiByte(
		CP_UTF8,
		WC_ERR_INVALID_CHARS,
		wstr, isize,
		utf8_str.data(), chars_needed,
		nullptr, nullptr
	);
	if(chars_written == 0 || chars_written != chars_needed) {
		return { std::error_code(::GetLastError(), std::system_category()) };
	}

	utf8_str.resize(chars_written);

	return { std::move(utf8_str) };
}

}
