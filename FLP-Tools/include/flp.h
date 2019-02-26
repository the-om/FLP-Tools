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
		std::size_t var_size; // size of text_data in bytes
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

struct FLPPatternNoteRecord {
	enum Flag : std::uint16_t {
		// flags in flags
		slide = 0x08,

		// flags in midi_channel
		porta = 0x10
	};
	std::uint32_t position;    // in ticks
	std::uint16_t flags;
	std::uint16_t rack_channel;
	std::uint32_t length;      // in ticks
	std::uint8_t key;          // C5 is 60
	std::byte data0;
	std::uint8_t group_id;     // note group identifier, 0 is no group
	std::byte data1;
	std::uint8_t fine_pitch;   // 0 - 240, 120 is default
	std::byte data2;
	std::uint8_t release;
	std::uint8_t midi_channel; // also has porta flag in bit 4
	std::uint8_t pan;          // 0 to 128, 64 is default
	std::uint8_t velocity;     // 0 to 128
	std::uint8_t mod_x;        // 0 to 255, 128 is default
	std::uint8_t mod_y;        // 0 to 255, 128 is default
};

static_assert(sizeof(FLPPatternNoteRecord) == 24);


struct FLPPlaylistClipRecord {
	std::uint32_t position;
	std::uint16_t data0;
	std::uint16_t source_index; // upper 4 bits are have 0x4 set if it's a pattern,
	                            // if upper 4 bits are 0x5 it' a pattern clip and the lower 24 bit are the 1-based pattern index
	                            // if upper 4 bits are 0x6 it's a pattern block (no source index)
	                            // if upper 4 bits are clear it's an audio clip or auto clip and the lower 24 bit are the 0-based channel-rack index
	std::uint32_t duration;
	std::uint16_t lane_index;   // (499 - actual_lane_index) for FL 20
	                            // in FL 11 this was (998 - actual_lane_index) for pattern blocks and (98 - actual_lane_index) for clips
	std::uint8_t group;         // 0 is no group
	std::byte data1[4];         // the "Main Automation" clip has data1[3]=0 otherwise these bytes are always(?) 0x00, 0x78, 0x00, 0x40
	std::byte flags;            // bit 6 is mute
	std::byte data2[4];         // looks like pan, vol, modx, mody but seems to be unused
	std::int32_t window_start;  // if window_start and window_end are -1 no window is applied to the pattern
	std::int32_t window_end;    // the "Main Automation" clip has window_start=-1 and window_end=0x7FFFFFFF
	                            // for automation clips and audio clips these are floats (-1.f means no window)
	                            // these floats are in units of 4 steps (no matter what steps per beat is)
};
static_assert(sizeof(FLPPlaylistClipRecord) == 32);
#pragma pack(pop)

}
