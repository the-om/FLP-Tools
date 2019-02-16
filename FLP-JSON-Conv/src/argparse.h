#pragma once

#include <functional>
#include <unordered_map>
#include <string>
#include <string_view>
#include <cassert>
#include <cstring>
#include <type_traits>


namespace Om {

namespace detail {

template<typename CharT>
struct DefaultParseOptions;

template<>
struct DefaultParseOptions<char> {
	static constexpr char const short_prefix[] = "-";
	static constexpr char const long_prefix[] = "--";
};

template<>
struct DefaultParseOptions<wchar_t> {
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
using array_elem_t = std::remove_pointer_t<std::decay_t<ArT>>;

template<typename CharT, std::size_t PrefLen>
CharT* matches_prefix(CharT* s, CharT const (&prefix)[PrefLen]) {
	auto const nul = nulchar<CharT>();
	std::size_t i = 0;
	for(; i < PrefLen - 1; ++i) {
		if(s[i] == nul || s[i] != prefix[i])
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
using ArgHandlerMap = std::unordered_map<std::basic_string_view<CharT>, HandlerType<CharT>>;


template<typename CharT = char, typename PrsOpt = detail::DefaultParseOptions<CharT>>
void parse_args(int argc, CharT** argv, ArgHandlerMap<CharT> const& arg_handlers) {
	using namespace detail;
	using namespace std::string_literals;

	static_assert(
		std::is_array<decltype(PrsOpt::short_prefix)>::value
		&&
		std::is_array<decltype(PrsOpt::long_prefix)>::value,
		"ParseOptions prefixes must be arrays"
		);

	static_assert(
		std::is_same<
		std::remove_const_t<CharT>,
		std::remove_cv_t<array_elem_t<decltype(PrsOpt::short_prefix)>>
		>::value
		&&
		std::is_same<
		std::remove_const_t<CharT>,
		std::remove_cv_t<array_elem_t<decltype(PrsOpt::long_prefix)>>
		>::value,
		"ParseOptions prefixes char types must be the same as character type of argv"
		);

	assert(argc >= 1);
	assert(argv);

	enum class State {
		end,
		was_not_switch,
		was_switch,
		last_arg,
	};

	auto const nul = nulchar<CharT>();

	auto handler_end_it = arg_handlers.end();
	typename ArgHandlerMap<CharT>::const_iterator handler_it;

	int arg_index = 1;
	State state = State::was_not_switch;
	while(state != State::end) {
		switch(state) {
		case State::was_not_switch:
			if(arg_index >= argc) {
				state = State::end;
				break;
			}
			if(CharT const* s = is_switch<PrsOpt>(argv[arg_index])) {
				handler_it = arg_handlers.find(s);
				if(handler_it == handler_end_it) {
					throw std::runtime_error { "unknown option" };
				} else {
					++arg_index;
					state = State::was_switch;
				}
			} else {
				state = State::last_arg;
			}
			break;
		case State::was_switch:
			if(arg_index >= argc) {
				handler_it->second(nullptr);
				state = State::end;
			}
			if(CharT const* s = is_switch<PrsOpt>(argv[arg_index])) {
				handler_it->second(nullptr);
				handler_it = arg_handlers.find(s);
				if(handler_it == handler_end_it) {
					throw std::runtime_error { "unknown option" };
				} else {
					++arg_index;
					state = State::was_switch;
				}
			} else {
				handler_it->second(argv[arg_index]);
				++arg_index;
				state = State::was_not_switch;
			}
			break;
		case State::last_arg:
			handler_it = arg_handlers.find(std::basic_string_view<CharT>(&nul));
			if(handler_it != handler_end_it) {
				handler_it->second(argv[arg_index]);
				state = State::end;
			} else {
				throw std::runtime_error { "no handler for last argument" };
			}
			break;
		default:
			state = State::end;
			break;
		}
	}
}

} // namespace Om
