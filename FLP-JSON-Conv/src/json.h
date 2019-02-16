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
		bool add_comma = false;
		bool add_newline = false;
		if(!m_agg_stack.empty()) {
			StackEntry e = m_agg_stack.top();
			if(e.type == AggregateType::Array) {
				add_newline = true;
				size_required += 1 + m_indent_level;
				if(e.n_elems > 0) {
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
		*m_position++ = '{';
		m_agg_stack.push(StackEntry { AggregateType::Object, 0 });
		m_indent_level += 1;
	}

	void end_object() {
		m_indent_level -= 1;

		auto size_required = 1;
		if(m_agg_stack.top().n_elems > 0) {
			// newline and indent
			size_required += 1 + m_indent_level;
		}
		flush_if_necessary(size_required);
		if(m_agg_stack.top().n_elems > 0) {
			*m_position++ = '\n';
			indent();
		}
		*m_position++ = '}';

		m_agg_stack.pop();
		if(!m_agg_stack.empty()) {
			StackEntry& e = m_agg_stack.top();
			++e.n_elems;
		}
	}

	void begin_array() {
		m_indent_level += 1;

		flush_if_necessary(1);

		*m_position++ = '[';
		m_agg_stack.push(StackEntry { AggregateType::Array, 0 });
	}

	void end_array() {
		m_indent_level -= 1;
		int size_required = 1;
		if(m_agg_stack.top().n_elems > 0) {
			// newline and indent
			size_required += 1 + m_indent_level;
		}

		flush_if_necessary(size_required);

		if(m_agg_stack.top().n_elems > 0) {
			*m_position++ = '\n';
			indent();
		}
		*m_position++ = ']';

		m_agg_stack.pop();
	}

	void key(std::string_view key) {
		int const keylen = (int)key.length();
		// space for newline, indentation, key, quotes, colon and space
		auto required_size = 1 + m_indent_level + 1 + keylen + 1 + 1 + 1;
		if(m_agg_stack.top().n_elems > 0) {
			required_size += 1; // comma
		}
		flush_if_necessary(required_size);
		if(m_agg_stack.top().n_elems > 0) {
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
			flush();
			char dquote = '"';
			m_stream.write(&dquote, 1);
			m_stream.write(value.data(), vlen);
			m_stream.write(&dquote, 1);
		} else {
			flush_if_necessary(size_required);
			*m_position++ = '"';
			std::memcpy(m_position, value.data(), vlen);
			m_position += vlen;
			*m_position++ = '"';
		}
		++m_agg_stack.top().n_elems;
	}

	void value(std::byte bt) {
		flush_if_necessary(2);
		auto value = std::uint8_t(bt);
		auto result = std::to_chars(m_position, m_position + 2, value, 16);
		m_position = result.ptr;
		++m_agg_stack.top().n_elems;
	}

	void value(std::nullptr_t) {
		flush_if_necessary(sizeof("null") - 1);
		std::memcpy(m_position, "null", sizeof("null") - 1);
		m_position += sizeof("null") - 1;
		++m_agg_stack.top().n_elems;
	}

	template<typename T>
	std::enable_if_t<std::is_arithmetic_v<T>> value(T value) {
		// TODO: charconv
		auto s = std::to_string(value);
		flush_if_necessary((int)s.size());
		std::memcpy(m_position, s.data(), s.size());
		m_position += s.size();
		++m_agg_stack.top().n_elems;
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
		if(space_available() < required_size) {
			flush();
		}
	}

	void indent() {
		for(int i = 0; i < m_indent_level; ++i) {
			*m_position++ = '\t';
		}
	}

	enum class AggregateType : std::uint8_t {
		Array,
		Object
	};

	struct StackEntry {
		AggregateType type;
		int n_elems;
	};

	static constexpr int buffer_size = 128;

	char m_buffer[buffer_size];
	char* m_position;
	std::stack<StackEntry, std::vector<StackEntry>> m_agg_stack;
	int m_indent_level = 0;
	StreamT& m_stream;
};


enum class JSONtype : std::uint8_t {
	null,
	boolean,
	number,
	string,
	array,
	object,
	variant = 0xFF
};

using JSONbool = bool;
using JSONnumber = double;
using JSONstring = std::string;
class JSONobject;
class JSONarray;

class JSONvariant {
public:
	JSONvariant() : mtype{ JSONtype::null }  {
		std::memset(obj_buffer, 0, sizeof(obj_buffer));
	}

	~JSONvariant() {
		destroy();
	}

	JSONvariant& operator=(JSONbool b) {
		destroy();
		get_bool() = b;
		mtype = JSONtype::boolean;
		return *this;
	}

	JSONvariant& operator=(JSONnumber n) {
		destroy();
		get_number() = n;
		mtype = JSONtype::number;
		return *this;
	}

	JSONvariant& operator=(JSONstring const& s) {
		destroy();
		get_string() = s;
		mtype = JSONtype::boolean;
		return *this;
	}

	JSONvariant& operator=(JSONarray n);
	JSONvariant& operator=(JSONobject o);

	JSONtype type() const noexcept {
		return mtype;
	}

	JSONbool& get_bool() noexcept {
		return *std::launder(reinterpret_cast<JSONbool*>(obj_buffer));
	}
	JSONbool const& get_bool() const noexcept {
		return *std::launder(reinterpret_cast<JSONbool const*>(obj_buffer));
	}
	JSONnumber& get_number() noexcept {
		return *std::launder(reinterpret_cast<JSONnumber*>(obj_buffer));
	}
	JSONnumber const& get_number() const noexcept {
		return *std::launder(reinterpret_cast<JSONnumber const*>(obj_buffer));
	}
	JSONstring& get_string() noexcept {
		return *std::launder(reinterpret_cast<JSONstring*>(obj_buffer));
	}
	JSONstring const& get_string() const noexcept {
		return *std::launder(reinterpret_cast<JSONstring const*>(obj_buffer));
	}
	JSONarray& get_array() noexcept {
		return *std::launder(reinterpret_cast<JSONarray*>(obj_buffer));
	}
	JSONarray const& get_array() const noexcept {
		return *std::launder(reinterpret_cast<JSONarray const*>(obj_buffer));
	}
	JSONobject& get_object() noexcept {
		return *std::launder(reinterpret_cast<JSONobject*>(obj_buffer));
	}
	JSONobject const& get_object() const noexcept {
		return *std::launder(reinterpret_cast<JSONobject const*>(obj_buffer));
	}

	bool is_null() const noexcept {
		return mtype == JSONtype::null;
	}

	explicit operator bool() const noexcept {
		return !is_null();
	}

private:
	void destroy();

	alignas(alignof(JSONstring)) std::byte obj_buffer[40];
	JSONtype mtype;
};

class JSONarray {
public:
	using value_type = JSONvariant;
	using iterator = JSONvariant*;
	using const_iterator = JSONvariant const*;

	JSONarray() noexcept = default;
	JSONarray(JSONarray const&);

	JSONarray(JSONarray&& other) noexcept :
		_data(other._data),
		_count(other._count) {
		other._data = nullptr;
		other._count = 0;
	}

	~JSONarray() noexcept;

	JSONarray& operator=(JSONarray&& other) noexcept {
		_data = other._data;
		_count = other._count;
		other._data = nullptr;
		other._count = 0;
		return *this;
	}

	JSONarray& operator=(JSONarray&& other) {
		// TODO
		return *this;
	}

	JSONvariant& operator[](std::uint32_t i) noexcept {
		return _data[i];
	}

	JSONvariant const& operator[](std::uint32_t i) const noexcept {
		return _data[i];
	}

	JSONvariant* data() noexcept {
		return _data;
	}

	JSONvariant const* data() const noexcept {
		return _data;
	}

	std::size_t size() const {
		return _count;
	}

private:
	JSONvariant* _data = nullptr;
	std::uint32_t _count = 0;

};

static_assert(sizeof(JSONarray) <= sizeof(JSONvariant) - sizeof(JSONtype));
static_assert(sizeof(JSONstring) <= sizeof(JSONvariant) - sizeof(JSONtype));

class JSONobject {
public:

private:
	//std::unordered_map<std::string_view, JSONvariant> _map;
};

static_assert(sizeof(JSONobject) <= sizeof(JSONvariant) - sizeof(JSONtype));

inline void JSONvariant::destroy() {
	switch(mtype) {
	case Om::JSONtype::null:
		break;
	case Om::JSONtype::boolean:
		break;
	case Om::JSONtype::number:
		break;
	case Om::JSONtype::string: {
		auto& str = reinterpret_cast<JSONstring&>(obj_buffer);
		str.~JSONstring();
		break;
	}
	case Om::JSONtype::array: {
		auto& str = reinterpret_cast<JSONarray&>(obj_buffer);
		str.~JSONarray();
		break;
	}
	case Om::JSONtype::object: {
		auto& str = reinterpret_cast<JSONobject&>(obj_buffer);
		str.~JSONobject();
		break;
	}
	}
}


template<typename StreamT>
class JSONInStream {
public:
	enum class NextT {

	};

	JSONInStream(StreamT underlying) :
		m_stream { underlying },
		pos { &buffer[0] },
		max_pos { pos } {

	}

	JSONInStream(JSONInStream const&) = delete;
	JSONInStream& operator=(JSONInStream const&) = delete;
	JSONInStream(JSONInStream&&) = delete;
	JSONInStream& operator=(JSONInStream&&) = delete;

	NextT next_type() {
		skip_whitespace();

	}


private:
	void skip_whitespace() {
		do {
			while(pos < max_pos) {
				switch(*pos) {
				case '\t': [[fallthrough]];
				case ' ':  [[fallthrough]];
				case '\r': [[fallthrough]];
				case '\n': [[fallthrough]];
					break;
				default:
					return;
				}
			}
			auto actually_read = m_stream.read(buffer, buffer_size);
			pos = &buffer[0];
			max_pos = pos + actually_read;
		} while(true);
	}

	static constexpr int buffer_size = 128;
	char buffer[buffer_size] {};
	char* pos;
	char* max_pos;
	StreamT& m_stream;

};



} // namespace Om
