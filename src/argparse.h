#pragma once

#include <functional> // function
#include <unordered_map>
#include <cassert>
#include <cstring>
#include <type_traits>


namespace Om {

namespace detail {

template<typename CharT>
struct ParseOptions;

template<>
struct ParseOptions<char> {
	static constexpr char const short_prefix[] = "-";
	static constexpr char const long_prefix[] = "--";
};

template<>
struct ParseOptions<wchar_t> {
	static constexpr wchar_t const short_prefix[] = L"-";
	static constexpr wchar_t const long_prefix[] = L"--";
};

template<typename CharT>
CharT nulchar();

template<>
constexpr char nulchar<char>() {
	return '\0';
}

template<>
constexpr wchar_t nulchar<wchar_t>() {
	return L'\0';
}

template<typename ArT>
using array_elem_t = std::remove_cv_t<std::remove_pointer_t<std::decay_t<ArT>>>;

template<typename CharT, std::size_t PrefLen>
CharT* matches_prefix(CharT* s, CharT const (&prefix)[PrefLen]) {
	std::size_t i = 0;
	for(; i < PrefLen - 1; ++i) {
		if(s[i] == nulchar<CharT>() || s[i] != prefix[i])
			return nullptr;
	}
	return s + i;
};

template<typename PrsOpt, typename CharT>
CharT* is_switch(CharT* s) {
	CharT* result = matches_prefix(s, PrsOpt::long_prefix);
	if(result == nullptr) {
		result = matches_prefix(s, PrsOpt::short_prefix);
	}
	return result;
}

} // namespace detail

template<typename CharT = char>
using HandlerType = std::function<void(CharT const*)>;
template<typename CharT = char>
using ArgHandlerMap = std::unordered_map<std::basic_string<CharT>, HandlerType<CharT>>;

template<typename CharT>
HandlerType<CharT> write_var(std::basic_string<CharT>& target) {
	return [&target] (CharT const* arg) {
		if(arg)
			target = arg;
		else
			throw std::runtime_error{"Missing Argument"};
	};
}

template<typename CharT = char, typename PrsOpt = detail::ParseOptions<CharT>>
void parse_args(int argc, CharT** argv, ArgHandlerMap<CharT> const& arg_handlers) {
	using namespace detail;

	static_assert(std::is_array<decltype(PrsOpt::short_prefix)>::value &&
	              std::is_array<decltype(PrsOpt::long_prefix)>::value,
	              "ParseOptions prefixes must be arrays");

	static_assert(std::is_same<
	                  std::remove_const_t<CharT>, array_elem_t<decltype(PrsOpt::short_prefix)>
	              >::value &&
	              std::is_same<
	                  std::remove_const_t<CharT>, array_elem_t<decltype(PrsOpt::long_prefix)>
	              >::value, "ParseOptions prefixes char types must be the same as CharT");

	assert(argc >= 1);
	assert(argv);

	auto end = arg_handlers.end();
	typename ArgHandlerMap<CharT>::const_iterator it;

	int i = 1;

was_not_switch:
	if(i < argc) {
		if(CharT const* s = is_switch<PrsOpt>(argv[i])) {
			it = arg_handlers.find(s);
			if(it == end) {
				throw std::runtime_error("unknown option");
			} else {
				++i;
				goto was_switch;
			}
		} else {
			goto handle_last;
		}
	} else {
		goto the_end;
	}

was_switch:
	if(i < argc) {
		if(CharT const* s = is_switch<PrsOpt>(argv[i])) {
			it->second(nullptr);
			it = arg_handlers.find(s);
			if(it == end) {
				throw std::runtime_error{"unknown option"};
			} else {
				++i;
				goto was_switch;
			}
		} else {
			it->second(argv[i]);
			++i;
			goto was_not_switch;
		}
	} else {
		it->second(nullptr);
		goto the_end;
	}

handle_last:
	it = arg_handlers.find(L"");
	if(it != end) {
		it->second(argv[i]);
	} else {
		throw std::runtime_error{"no handler for last argument"};
	}

the_end:
	return;
}

} // namespace Om
