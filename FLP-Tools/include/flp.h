#pragma once

#include "flp_enums.h"

#include <memory>        // unique_ptr


namespace Om {

struct FLPEvent {
	FLPEventType type;
	union {
		std::uint8_t u8;
		std::int16_t i16;
		std::int32_t i32;
		std::size_t var_size;
	};
	std::unique_ptr<std::byte const[]> text_data;
};

#pragma pack(push, 1)
struct FLPChunkHeader {
	std::uint32_t ChunkID;
	std::uint32_t Length;
};

struct FLPFileHeader {
	FLPChunkHeader header;
	FLPFormat Format;
	std::uint16_t nChannels;
	std::uint16_t BeatDiv; // PPQ
};

struct RawPatternNote {
	enum Flag : std::uint16_t {
		// flags in flags
		slide = 0x08,

		// flags in midi_channel
		porta = 0x10
	};
	std::uint32_t position;
	std::uint16_t flags;
	std::uint16_t rack_channel;
	std::uint32_t length; // in ticks
	std::uint8_t key; // C5 is 60
	std::byte data0;
	std::uint8_t group_id; // note group identifier, 0 is no group
	std::byte data1;
	std::uint8_t fine_pitch; // 0 - 240, 120 is default
	std::byte data2;
	std::uint8_t release;
	std::uint8_t midi_channel; // also has porta flag in bit 4
	std::uint8_t pan; // 0 to 128, 64 is default
	std::uint8_t velocity; // 0 to 128
	std::uint8_t mod_x; // 0 to 255, 128 is default
	std::uint8_t mod_y; // 0 to 255, 128 is default
};

static_assert(sizeof(RawPatternNote) == 24);


struct RawPlaylistClip {
	std::uint32_t position;
	std::uint32_t flags;
	std::uint32_t duration;
	std::uint16_t lane_index; // (499 - lane_index) for FL 20
	std::byte data[10];
	std::uint32_t window_start; // if window_start and window_end are -1 no window is applied to the pattern
	std::uint32_t window_end; // automation clips are different
};

#pragma pack(pop)

}
