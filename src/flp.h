#pragma once

#include <memory>        // unique_ptr

#include "flp_enums.h"


struct FLPEvent {
	FLPEventType type;
	union {
		uint8 u8;
		int16 i16;
		int32 i32;
		size var_size;
	};

	std::unique_ptr<byte const[]> text_data;
};

#pragma pack(push, 1)
struct FLPChunkHeader {
	uint32 ChunkID;
	uint32 Length;
};

struct FLPFileHeader {
	FLPChunkHeader header;
	FLPFormat Format;
	uint16 nChannels;
	uint16 BeatDiv; // PPQ
};

struct RawPatternNote {
	enum Flag : uint16 {
		// flags in flags
		slide = 0x8,

		// flags in midi_channel
		porta = 0x10
	};
	uint32 position;
	uint16 flags;
	uint16 rack_channel;
	uint32 length; // in ticks
	uint8 key; // C5 is 60
	byte data0;
	uint8 group_id; // note group identifier, 0 is no group
	byte data1;
	uint8 fine_pitch; // 0 - 240, 120 is default
	byte data2;
	uint8 release;
	uint8 midi_channel; // also has porta flag in bit 4
	uint8 pan; // 0 to 128, 64 is default
	uint8 velocity; // 0 to 128
	uint8 mod_x; // 0 to 255, 128 is default
	uint8 mod_y; // 0 to 255, 128 is default
};

static_assert(sizeof(RawPatternNote) == 24, "");
#pragma pack(pop)
