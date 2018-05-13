#pragma once

#include "flp.h"

#include <cassert>       // assert
#include <vector>        // vector
#include <string_view>   // string_view, wstring_view, std::size
#include <system_error>  // system_error
#include <utility>       // forward

#include "win32.h"
#include <omlib/win32/helpers.h> // utf conversion, TODO: replace by platform independent code


template<typename StreamType>
struct FLPInStream {

	template<typename... Args>
	FLPInStream(Args&&... args) :
		_stream(std::forward<Args>(args)...) {

	}

	void read_header() {
		// read flp header
		if(!_stream.read(&_file_header))
			failed_read(_stream);
		if(!check_chunkID(_file_header.header.ChunkID, "FLhd"))
			throw std::runtime_error { "Not an FLP file!" };

		// read data header
		if(!_stream.read(&_data_header))
			failed_read(_stream);
		if(!check_chunkID(_data_header.ChunkID, "FLdt"))
			throw std::runtime_error { "Invalid data header!" };
	}

	FLPEvent const& operator*() const& {
		return _current_event;
	}

	FLPInStream& operator++() {
		uint8_t event_id = 0;
		if(!_stream.read(&event_id))
			failed_read(_stream);

		_data_bytes_read += 1;

		_current_event.type = static_cast<FLPEventType>(event_id);

		auto event_size = event_id / 64;

		switch(event_size) {
		case 0:
			if(!_stream.read(&_current_event.u8))
				failed_read(_stream);
			_data_bytes_read += sizeof(uint8);
			break;
		case 1:
			if(!_stream.read(&_current_event.i16))
				failed_read(_stream);
			_data_bytes_read += sizeof(int16);
			break;
		case 2:
			if(!_stream.read(&_current_event.i32))
				failed_read(_stream);
			_data_bytes_read += sizeof(int32);
			break;
		case 3:
		{ // TEXT event
			uint32 text_size = 0;
			uint8 current_byte;
			uint32 shift_by = 0;
			do { // extract size
				 // left shift by more is undefined behaviour
				assert(shift_by < sizeof(shift_by) * CHAR_BIT);
				if(!_stream.read(&current_byte))
					failed_read(_stream);
				_data_bytes_read += 1;
				text_size += ((current_byte & uint8(0x7FU)) << shift_by);
				shift_by += 7;
			} while(current_byte & 0x80U);

			_current_event.var_size = text_size;

			if(text_size == 0) {
				_current_event.text_data = nullptr;
			} else {
				auto up_buffer = std::make_unique<byte[]>(text_size);
				if(_stream.read(up_buffer.get(), text_size) != text_size)
					failed_read(_stream);
				_data_bytes_read += text_size;

				_current_event.text_data = std::move(up_buffer);
			}
			break;
		}
		}
		return *this;
	}

	bool has_event() {
		return _data_bytes_read < _data_header.Length;
	}

	FLPFileHeader const& file_header() const& {
		return _file_header;
	}

	FLPChunkHeader const& data_header() const& {
		return _data_header;
	}

private:
	[[noreturn]]
	static void failed_read(StreamType const& stream) {
		std::string errstr = stream.errmsg(stream.error());
		throw std::runtime_error(errstr);
	}

	static bool check_chunkID(uint32 id, char const (&p)[5]) noexcept {
		assert(strlen(p) == 4);
		uint32 composit = p[0];
		composit |= (p[1] << 8);
		composit |= (p[2] << 16);
		composit |= (p[3] << 24);
		return id == composit;
	};

	StreamType _stream {};
	FLPFileHeader _file_header {};
	FLPChunkHeader _data_header {};
	uint32 _data_bytes_read = 0;
	FLPEvent _current_event {};
};

template<typename StreamT>
void stream_flp_header(StreamT& stream, FLPFileHeader const& header) {
	stream.begin_object();
	stream.key("format");
	stream.value_str_noescape(header_format_name(header.Format));
	stream.key("n_channels");
	stream.value(header.nChannels);
	stream.key("ppq");
	stream.value(header.BeatDiv);
	stream.end_object();
}

namespace detail {
	inline char get_nibble_char(byte nibble) {
		auto nbval = static_cast<unsigned char>(nibble);
		if(nbval < 0xA) {
			return nbval + '0';
		} else {
			return nbval - 0xA + 'A';
		}
	}

	template<typename Stream>
	void stream_fxrouting(Stream& stream, FLPEvent const& e) {
		stream.key("data_type");
		stream.value_str_noescape("fx_routing[]");
		stream.key("data_size");
		stream.value(e.var_size);
		stream.key("data");
		stream.begin_array();

		auto const* data = reinterpret_cast<unsigned char const*>(e.text_data.get());

		assert(e.var_size != 0);
		for(size i = 0; i < e.var_size; ++i) {
			if(data[i] == 0) {
				continue;
			}
			stream.begin_object();
			stream.key("target");
			stream.value(i);
			stream.key("value");
			stream.value(data[i]);
			stream.end_object();
		}

		stream.end_array();
	}

	template<typename Stream>
	void stream_pattern_note(Stream& stream, FLPEvent const& e) {
		assert(e.var_size % sizeof(RawPatternNote) == 0);
		stream.key("data_type");
		stream.value_str_noescape("pattern_note[]");
		stream.key("data");
		auto* notes = reinterpret_cast<RawPatternNote const*>(e.text_data.get());
		int const n_notes = static_cast<int>(e.var_size / sizeof(RawPatternNote));
		stream.begin_array();
		for(int i = 0; i < n_notes; ++i) {
			RawPatternNote const& note = notes[i];
			stream.begin_object();
			stream.key("position");
			stream.value(note.position);
			stream.key("flags");
			stream.value(note.flags);
			stream.key("rack_channel");
			stream.value(note.rack_channel);
			stream.key("length");
			stream.value(note.length);
			// C5 is 60
			stream.key("key");
			stream.value(note.key);
			stream.key("data0");
			stream.value(note.data0);
			stream.key("group");
			stream.value(note.group_id);
			stream.key("data1");
			stream.value(note.data1);
			stream.key("fine_pitch");
			stream.value(note.fine_pitch);
			stream.key("data2");
			stream.value(note.data2);
			stream.key("release");
			stream.value(note.release);
			stream.key("flags2");
			stream.value((note.midi_channel & 0xF0) >> 4);
			stream.key("midi_channel");
			stream.value(note.midi_channel & 0x0F);
			stream.key("pan");
			stream.value(note.pan);
			stream.key("velocity");
			stream.value(note.velocity);
			stream.key("mod_x");
			stream.value(note.mod_x);
			stream.key("mod_y");
			stream.value(note.mod_y);
			stream.end_object();
		}
		stream.end_array();
	}

	template<typename Stream>
	void stream_bytes(Stream& stream, FLPEvent const& e) {
		// write bytes as hex
		stream.key("data_type");
		stream.value_str_noescape("bytes");
		stream.key("data_size");
		stream.value(e.var_size);

		stream.key("data");

		if(e.var_size == 0) {
			stream.value(nullptr);
			return;
		}

		char local_buf[4415];
		std::unique_ptr<char> large_buf;
		char* buf;
		size_t const required_bufsz = (e.var_size - 1) * 3 + 2;
		if(required_bufsz > std::size(local_buf)) {
			large_buf.reset(new char[required_bufsz]);
			buf = large_buf.get();
			//printf("used large buffer! size: %zu\n", required_bufsz);
		} else {
			buf = local_buf;
		}
		byte const* data = e.text_data.get();
		byte bt;
		char* p = buf;
		for(auto i = 0U; i < e.var_size - 1; i++) {
			bt = data[i];
			p[0] = detail::get_nibble_char((bt & byte { 0xF0 }) >> 4);
			p[1] = detail::get_nibble_char(bt & byte { 0x0F });
			p[2] = ' ';
			p += 3;
		}
		bt = data[e.var_size - 1];
		p[0] = detail::get_nibble_char((bt & byte { 0xF0 }) >> 4);
		p[1] = detail::get_nibble_char(bt & byte { 0x0F });

		stream.value_str_noescape(std::string_view(buf, required_bufsz));
	}

	template<bool useWideStr, typename Stream>
	void stream_string(Stream& stream, FLPEvent const& e) {
		stream.key("data_type");
		stream.value_str_noescape("string");
		stream.key("string_length");
		assert(e.var_size >= 1);
		if constexpr (useWideStr) {
			assert(e.var_size % 2 == 0);
			stream.value(e.var_size / 2 - 1);
			stream.key("data");
			// FLP_Text_* is a UTF16 string from FL12 on
			auto wstr = reinterpret_cast<wchar_t const*>(e.text_data.get());
			std::size_t const len = e.var_size / 2 - 1;
			if(auto sutf8 = Om::win32::utf16_to_utf8(std::wstring_view(wstr, len)))
				stream.value(sutf8.get());
			else
				throw std::system_error(sutf8.get_error());
		} else {
			stream.value(e.var_size - 1);
			stream.key("data");
			char const* str = reinterpret_cast<char const*>(e.text_data.get());
			std::size_t const len = e.var_size - 1;
			stream.value(std::string_view(str, len));
		}
	}
}

template<bool useWideStr, typename StreamT>
void stream_flp_event(StreamT& stream, FLPEvent const& e) {
	auto const event_id =
		static_cast<std::underlying_type_t<FLPEventType>>(e.type);
	auto const event_size = event_id / 64;
	{
		stream.begin_object();
		stream.key("id");
		char id_buffer[48];
		char const* event_name = flp_event_name(e.type);
		int len = std::snprintf(id_buffer, sizeof(id_buffer), "%u/%s",
			static_cast<unsigned>(event_id),
			(event_name ? event_name : "Unknown"));
		stream.value_str_noescape(std::string_view(id_buffer, len));
	}

	switch(event_size) {
	case 0:
		stream.key("data_type");
		stream.value_str_noescape("uint8");
		stream.key("data");
		stream.value(static_cast<unsigned>(e.u8));
		break;
	case 1:
		stream.key("data_type");
		stream.value_str_noescape("int16");
		stream.key("data");
		stream.value(e.i16);
		break;
	case 2:
		stream.key("data_type");
		stream.value_str_noescape("int32");
		stream.key("data");
		stream.value(e.i32);
		break;
	case 3:
		switch(e.type) {
		case FLPEventType::FLP_Version:
			detail::stream_string<false>(stream, e);
			break;
		case FLPEventType::FLP_Text_ChanName:
		case FLPEventType::FLP_Text_PatName:
		case FLPEventType::FLP_Text_Title:
		case FLPEventType::FLP_Text_Comment:
		case FLPEventType::FLP_Text_SampleFileName:
		case FLPEventType::FLP_Text_URL:
		case FLPEventType::FLP_Text_CommentRTF:
		case FLPEventType::FLP_RegName:
		case FLPEventType::FLP_Text_DefPluginName:
		case FLPEventType::FLP_Text_ProjDataPath:
		case FLPEventType::FLP_Text_PluginName:
		case FLPEventType::FLP_Text_FXName:
		case FLPEventType::FLP_Text_TimeMarker:
		case FLPEventType::FLP_Text_Genre:
		case FLPEventType::FLP_Text_Author:
		case FLPEventType::FLP_Text_RemoteCtrlFormula:
		case FLPEventType::FLP_Text_ChanFilter:
		case FLPEventType::FLP_Text_PLTrackName:
			detail::stream_string<useWideStr>(stream, e);
			break;
		case FLPEventType::FLP_PatNoteRecChan:
			detail::stream_pattern_note(stream, e);
			break;
		case FLPEventType::FLP_FXRouting:
			detail::stream_fxrouting(stream, e);
			break;
		default:
			detail::stream_bytes(stream, e);
			break;
		}
		break;
	}

	stream.end_object();
}
