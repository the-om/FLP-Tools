#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <cstring>
#include <cassert>
#include <stack>
#include <vector>
#include <string>
#include <type_traits>
#include <charconv>
#include <new>


namespace Om {

inline std::string json_escape_string(std::string_view strv) {
	static char escape_chars[] = {
		'"',
		'\\',
		'/',
		'\b',
		'\f',
		'\n',
		'\r',
		'\t',
		'\0'
	};
	char const* str = strv.data();
	char const* endptr = str + strv.size();
	std::string ret;
	for(;;) {
		char const* where = nullptr;
		for(char const* p = str; p < endptr; ++p) {
			for(char const* pesc = &escape_chars[0]; *pesc != '\0'; ++pesc) {
				if(*p == *pesc) {
					where = p;
					break;
				}
			}
			if(where != nullptr)
				break;
		}
		if(where == nullptr) {
			break;
		}
		ret.append(str, where - str);
		switch(*where) {
		case '"':
			ret.append("\\\"", 2);
			break;
		case '\\':
			ret.append("\\\\", 2);
			break;
		case '/':
			ret.append("\\/", 2);
			break;
		case '\b':
			ret.append("\\b", 2);
			break;
		case '\f':
			ret.append("\\f", 2);
			break;
		case '\n':
			ret.append("\\n", 2);
			break;
		case '\r':
			ret.append("\\r", 2);
			break;
		case '\t':
			ret.append("\\t", 2);
			break;
		}
		str = where + 1;
	}
	ret.append(str, endptr - str);
	return ret;
}

template<typename StreamT>
class JSONOutStream {
public:
	JSONOutStream(StreamT& underlying) :
		m_stream { underlying },
		m_position { &m_buffer[0] } {
	}

	JSONOutStream(JSONOutStream const&) = delete;
	JSONOutStream& operator=(JSONOutStream const&) = delete;
	JSONOutStream(JSONOutStream&&) = delete;
	JSONOutStream& operator=(JSONOutStream&&) = delete;

	~JSONOutStream() {
		assert(m_agg_stack.empty());
		flush();
	}

	void begin_object() {
		int size_required = 1;
		prepare_write_value(size_required);
		*m_position++ = '{';
		m_agg_stack.push(StackEntry(AggregateType::Object));
		m_indent_level += 1;
	}

	void end_object() {
		assert(!m_agg_stack.empty());
		m_indent_level -= 1;
		auto size_required = 1;
		bool const nonempty = m_agg_stack.top().nonempty();
		if(nonempty) {
			// newline and indent
			size_required += 1 + m_indent_level;
		}
		flush_if_necessary(size_required);
		if(nonempty) {
			*m_position++ = '\n';
			indent();
		}
		*m_position++ = '}';

		m_agg_stack.pop();
		if(!m_agg_stack.empty())
			m_agg_stack.top().set_nonempty();
	}

	void begin_array() {
		prepare_write_value(1);
		*m_position++ = '[';
		m_agg_stack.push(StackEntry(AggregateType::Array));
		m_indent_level += 1;
	}

	void end_array() {
		assert(!m_agg_stack.empty());
		m_indent_level -= 1;
		int size_required = 1;

		if(m_agg_stack.top().nonempty()) {
			// newline and indent
			size_required += 1 + m_indent_level;
		}

		flush_if_necessary(size_required);

		if(m_agg_stack.top().nonempty()) {
			*m_position++ = '\n';
			indent();
		}
		*m_position++ = ']';

		m_agg_stack.pop();
		if(!m_agg_stack.empty())
			m_agg_stack.top().set_nonempty();
	}

	void key(std::string_view key) {
		int const keylen = (int)key.length();
		// space for newline, indentation, key, quotes, colon and space
		auto required_size = 1 + m_indent_level + 1 + keylen + 1 + 1 + 1;
		assert(!m_agg_stack.empty() && m_agg_stack.top().type() == AggregateType::Object);
		bool const is_not_first_key = m_agg_stack.top().nonempty();
		if(is_not_first_key) {
			required_size += 1; // comma
		}
		flush_if_necessary(required_size);
		if(is_not_first_key) {
			*m_position++ = ',';
		}
		*m_position++ = '\n';
		indent();
		*m_position++ = '"';
		std::memcpy(m_position, key.data(), keylen);
		m_position += keylen;
		*m_position++ = '"';
		*m_position++ = ':';
		*m_position++ = ' ';
	}

	void value(std::string_view value) {
		value_str_noescape(json_escape_string(value));
	}

	void value_str_noescape(std::string_view value) {
		int const vlen = (int)value.size();
		int const size_required = vlen + 2;

		if(size_required > buffer_size) {
			prepare_write_value(1);
			*m_position++ ='"';
			flush();
			m_stream.write(value.data(), vlen);
			*m_position++ = '"';
		} else {
			prepare_write_value(size_required);
			*m_position++ = '"';
			std::memcpy(m_position, value.data(), vlen);
			m_position += vlen;
			*m_position++ = '"';
		}
		if(!m_agg_stack.empty())
			m_agg_stack.top().set_nonempty();
	}

	void value(std::byte bt) {
		prepare_write_value(2);
		auto value = std::uint8_t(bt);
		auto result = std::to_chars(m_position, m_position + 2, value, 16);
		m_position = result.ptr;
		if(!m_agg_stack.empty())
			m_agg_stack.top().set_nonempty();
	}

	void value(std::nullptr_t) {
		prepare_write_value(sizeof("null") - 1);
		std::memcpy(m_position, "null", sizeof("null") - 1);
		m_position += sizeof("null") - 1;
		if(!m_agg_stack.empty())
			m_agg_stack.top().set_nonempty();
	}

	template<typename T>
	std::enable_if_t<std::is_arithmetic_v<T>> value(T value) {
		// TODO: charconv
		auto s = std::to_string(value);
		prepare_write_value((int)s.size());
		std::memcpy(m_position, s.data(), s.size());
		m_position += s.size();
		if(!m_agg_stack.empty())
			m_agg_stack.top().set_nonempty();
	}

	void flush() {
		m_stream.write(&m_buffer[0], m_position - &m_buffer[0]);
		m_position = &m_buffer[0];
	}

private:
	int space_available() {
		return (int)(m_buffer + buffer_size - m_position);
	}

	void flush_if_necessary(int required_size) {
		assert(required_size <= buffer_size);
		if(space_available() < required_size) {
			flush();
		}
	}

	void indent() {
		for(int i = 0; i < m_indent_level; ++i) {
			*m_position++ = '\t';
		}
	}

	void prepare_write_value(int size_required) {
		bool add_comma = false;
		bool add_newline = false;
		if(!m_agg_stack.empty()) {
			StackEntry e = m_agg_stack.top();
			if(e.type() == AggregateType::Array) {
				add_newline = true;
				size_required += 1 + m_indent_level;
				if(e.nonempty()) {
					size_required += 1;
					add_comma = true;
				}
			}
		}
		flush_if_necessary(size_required);
		if(add_newline) {
			if(add_comma) {
				*m_position++ = ',';
			}
			*m_position++ = '\n';
			indent();
		}
	}

	enum class AggregateType : std::uint8_t {
		Array,
		Object
	};

	struct StackEntry {
		StackEntry(AggregateType type) : 
			data { static_cast<std::uint8_t>(type) } {
		}

		AggregateType type() const {
			return static_cast<AggregateType>(data & 0x7F);
		}

		void set_nonempty() {
			data |= 0x80;
		}

		bool nonempty() const {
			return (data & 0x80) != 0;
		}

	private:
		std::uint8_t data;
	};

	static constexpr int buffer_size = 128;

	char m_buffer[buffer_size];
	char* m_position;
	std::stack<StackEntry, std::vector<StackEntry>> m_agg_stack;
	int m_indent_level = 0;
	StreamT& m_stream;
};

} // namespace Om
