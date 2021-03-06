#pragma once

#include "flp.h"
#include "flp_utf_conversions.h"

#include <cassert>       // assert
#include <vector>        // vector
#include <string_view>   // string_view, wstring_view, std::size
#include <stdexcept>     // runtime_error
#include <utility>       // forward


namespace Om {

template<typename StreamType>
class FLPInStream {
public:
	template<typename... Args>
	FLPInStream(Args&&... args) :
		_stream(std::forward<Args>(args)...) {
		read_headers();
		++(*this);
	}

	FLPInStream(FLPInStream const&) = delete;
	FLPInStream& operator=(FLPInStream const&) = delete;

	bool has_event() {
		return _data_bytes_read < _data_header.Length;
	}

	FLPEvent* operator->() & {
		return &_current_event;
	}

	FLPEvent& operator*() & {
		return _current_event;
	}

	FLPInStream& operator++() {
		std::uint8_t event_id = 0;
		if(!_stream.read(&event_id))
			failed_read(_stream);

		_data_bytes_read += 1;

		_current_event.type = static_cast<FLPEventType>(event_id);

		auto event_size = event_id / 64;

		switch(event_size) {
		case 0:
			if(!_stream.read(&_current_event.u8))
				failed_read(_stream);
			_data_bytes_read += sizeof(std::uint8_t);
			break;
		case 1:
			if(!_stream.read(&_current_event.i16))
				failed_read(_stream);
			_data_bytes_read += sizeof(std::int16_t);
			break;
		case 2:
			if(!_stream.read(&_current_event.i32))
				failed_read(_stream);
			_data_bytes_read += sizeof(std::int32_t);
			break;
		case 3:
		{ // TEXT event
			std::uint32_t text_size = 0;
			std::uint8_t current_byte;
			std::uint32_t shift_by = 0;
			do { // extract size
				 // left shift by more is undefined behaviour
				assert(shift_by < sizeof(shift_by) * CHAR_BIT);
				if(!_stream.read(&current_byte))
					failed_read(_stream);
				_data_bytes_read += 1;
				text_size += ((current_byte & std::uint8_t(0x7FU)) << shift_by);
				shift_by += 7;
			} while(current_byte & 0x80U);

			_current_event.var_size = text_size;

			if(text_size == 0) {
				_current_event.text_data = nullptr;
			} else {
				auto up_buffer = std::make_unique<std::byte[]>(text_size);
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

	FLPFileHeader const& file_header() const& {
		return _file_header;
	}

	FLPChunkHeader const& data_header() const& {
		return _data_header;
	}

private:
	void read_headers() {
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

	[[noreturn]]
	static void failed_read(StreamType const& stream) {
		std::string errstr = stream.errmsg(stream.error());
		throw std::runtime_error(errstr);
	}

	static bool check_chunkID(std::uint32_t id, char const (&p)[5]) noexcept {
		assert(strlen(p) == 4);
		return id == std::uint32_t(p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24);
	};

	FLPEvent _current_event {};
	std::uint32_t _data_bytes_read = 0;
	FLPFileHeader _file_header {};
	FLPChunkHeader _data_header {};
	StreamType _stream {};
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

	template<typename Stream>
	void stream_fxrouting(Stream& stream, FLPEvent const& e) {
		stream.key("data_type");
		stream.value_str_noescape("fx_routing[]");
		stream.key("data");
		stream.begin_array();

		auto const* data = reinterpret_cast<unsigned char const*>(e.text_data.get());

		assert(e.var_size != 0);
		for(std::size_t i = 0; i < e.var_size; ++i) {
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
	void stream_pattern_notes(Stream& stream, FLPEvent const& e) {
		assert(e.var_size % sizeof(FLPPatternNoteRecord) == 0);
		stream.key("data_type");
		stream.value_str_noescape("pattern_note[]");
		stream.key("data");
		auto* notes = reinterpret_cast<FLPPatternNoteRecord const*>(e.text_data.get());
		int const n_notes = static_cast<int>(e.var_size / sizeof(FLPPatternNoteRecord));
		stream.begin_array();
		for(int i = 0; i < n_notes; ++i) {
			FLPPatternNoteRecord const& note = notes[i];
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
	void stream_playlist_clips(Stream& stream, FLPEvent const& e) {
		assert(e.var_size % sizeof(FLPPlaylistClipRecord) == 0);
		stream.key("data_type");
		stream.value_str_noescape("playlist_clip[]");
		stream.key("data");
		auto* clips = reinterpret_cast<FLPPlaylistClipRecord const*>(e.text_data.get());
		int const n_clips = static_cast<int>(e.var_size / sizeof(FLPPlaylistClipRecord));
		stream.begin_array();
		for(int i = 0; i < n_clips; ++i) {
			FLPPlaylistClipRecord const& clip = clips[i];
			stream.begin_object();
			stream.key("position");
			stream.value(clip.position);
			stream.key("data0");
			stream.value(clip.data0);
			stream.key("source_index");
			stream.value(clip.source_index);
			stream.key("duration");
			stream.value(clip.duration);
			stream.key("lane_index");
			stream.value(clip.lane_index);
			stream.key("group");
			stream.value(clip.group);
			stream.key("data1");
			stream.begin_array();
			for(int data_i = 0; data_i < std::size(clip.data1); ++data_i) {
				stream.value(clip.data1[data_i]);
			}
			stream.end_array();
			stream.key("flags");
			stream.value(clip.flags);
			stream.key("data2");
			stream.begin_array();
			for(int data_i = 0; data_i < std::size(clip.data2); ++data_i) {
				stream.value(clip.data2[data_i]);
			}
			stream.end_array();
			stream.key("window_start");
			stream.value(clip.window_start);
			stream.key("window_end");
			stream.value(clip.window_end);
			stream.end_object();
		}
		stream.end_array();
	}

	constexpr inline char get_nibble_char(std::byte nibble) noexcept {
		auto nbval = static_cast<unsigned char>(nibble);
		return (nbval < 0xA) ? nbval + '0' : nbval - 0xA + 'A';
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

		char local_buf[4457];
		//char local_buf[4016];
		std::unique_ptr<char> large_buf;
		char* buf;
		std::size_t const required_bufsz = (e.var_size - 1) * 3 + 2;
		if(required_bufsz > std::size(local_buf)) {
			large_buf.reset(new char[required_bufsz]);
			buf = large_buf.get();
			//std::printf("used large buffer: %s, %zu bytes\n", flp_event_name(e.type), required_bufsz);
		} else {
			buf = local_buf;
		}
		std::byte const* data = e.text_data.get();
		std::byte b;
		char* p = buf;
		for(auto i = 0U; i < e.var_size - 1; i++) {
			b = data[i];
			p[0] = detail::get_nibble_char(b >> 4);
			p[1] = detail::get_nibble_char(b & std::byte { 0x0F });
			p[2] = ' ';
			p += 3;
		}
		b = data[e.var_size - 1];
		p[0] = detail::get_nibble_char(b >> 4);
		p[1] = detail::get_nibble_char(b & std::byte { 0x0F });

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
			std::string sutf8;
			if(std::error_code err = Om::utf16_to_utf8(std::wstring_view(wstr, len), &sutf8))
				throw std::system_error(err);
			else
				stream.value(sutf8);
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
			detail::stream_pattern_notes(stream, e);
			break;
		case FLPEventType::FLP_PLRecChan:
			detail::stream_playlist_clips(stream, e);
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

}