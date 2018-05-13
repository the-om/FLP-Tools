#pragma once

#include "result.h"
#include <string>
#include <string_view>


namespace Om {
	
Om::Result<std::string> utf16_to_utf8(std::wstring_view str16);

}
