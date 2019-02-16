#pragma once

#include <system_error>
#include <string>
#include <string_view>


namespace Om {
	
std::error_code utf16_to_utf8(std::wstring_view str16, std::string* out_strutf8);

}
