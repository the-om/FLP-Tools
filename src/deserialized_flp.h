#pragma once

#include <cassert>      // assert
#include <cstring>      // memcpy
#include <memory>       // unique_ptr
#include <string>       // wstring
#include <vector>       // vector
#include "type_defs.h"
#include "flp.h"
#include "version.h"


// TODO: Defaults!!
class DeserializedFLP {
public:
	template<typename Iter>
	static DeserializedFLP from_events(Iter begin, Iter end);

private:
	template<typename Iter>
	DeserializedFLP(Iter, Iter);

	struct Plugin {
		template<typename Iter>
		explicit Plugin(Iter&);
		Plugin() = default;
		std::wstring name;
		std::wstring display_name;
		uint32 icon;
		uint32 color;
		size param_size;
		std::unique_ptr<unsigned char const[]> params;
	};

	struct ChannelRackState {
		struct Channel {
			template<typename Iter>
			void read_events(Iter& it);

			// common to all channel types
			bool enabled = true;
			ChannelType type = ChannelType::Sampler;
			Plugin plugin;
			size filter_index; // index into channel filters array
			int_fast8 fine_tune; // -100 to 100
			uint_fast8 middle_note; // 0(C0) to 131(B10)
			bool key_zone_active;
			uint_fast8 key_zone_start; // 0 to 131
			uint_fast8 key_zone_end; // 0 to 131

			// specific to all generators, audio clips, sampler channels and layer
			struct {
				uint_fast16 pan; // 0 to 12800
				uint_fast16 vol; // 0 to 12800
				int_fast16  pitch; // -4800 to 4800
				uint_fast16 mod_x; // 0 to 256
				uint_fast16 mod_y; // 0 to 256
				enum : uint_fast8 {
					FastLP,
					LP,
					BP,
					HP,
					BS,
					LPx2,
					SVF_LP,
					SVF_LPx2
				} filter_type;
			} levels;

			struct {
				int_fast16 pan; // -6400 to 6400
				uint_fast16 vol; // 0 to 25600
				int_fast16 mod_x; // -256 to 256
				int_fast16 mod_y; // -256 to 256
			} levels_adjustment;

			// specific to all generators, audio clips and sampler channels
			uint_fast8 cut; // 0 to 99
			uint_fast8 cut_by; // 0 to 99

			bool follow_main_pitch : 1;
			bool add_to_key : 1;

			// specific to all generators and sampler
			bool full_porta : 1;
			uint_fast16 gate; // 450 to 1447 (1447 is off)
			uint_fast16 shift_time; // 0 to 1024
			uint_fast8 swing_mix; // 0 to 128

			struct Tracking {
				int_fast16 pan; // -256 to 256
				int_fast16 mod_x; // -256 to 256
				int_fast16 mod_y; // -256 to 256
				uint_fast8 middle_note; // 0(C0) to 120(C10)
			};
			Tracking vol_tracking;
			Tracking key_tracking;

			struct {
				uint16 feed; // 0 to 25600
				int_fast16 pan; // -6400 is -100%, 6400 is 0%, 19200 is 100%
				int16 pitch; // -1200 to 1200
				uint16 time; // 0 to 384
				uint_fast16 mod_x; // 0 is -100%, 128 is 0%, 256 is 100%
				uint_fast16 mod_y; // 0 is -100%, 128 is 0%, 256 is 100%
				uint_fast8 num_echoes; // 1 to 10
				bool ping_pong : 1;
				bool fat_mode : 1;
			} delay;

			struct {
				uint_fast8 max; // 0 to 99
				uint_fast16 slide; // 0 to 1660
				bool porta : 1, mono : 1;
			} polyphony;

			uint_fast8 target_mixer_track;

			// specific to sampler and hybrid generators
			struct Envelope {
				bool enabled : 1, tempo_synced : 1, lfo_tempo_synced : 1, lfo_global : 1;
				uint_fast32 delay, attack, hold, decay; // 100 to 65536
				uint_fast8 sustain; // 0 to 128
				uint_fast32 release; // 100 to 65536
				int_fast16 amount; // -128 to 128
				int_fast16 attack_tension; // -128 to 128
				int_fast16 decay_tension; // -128 to 128
				int_fast16 release_tension; // -128 to 128
				enum : uint_fast8 {
					Sine = 0,
					Triangle = 1,
					Square = 2
				} lfo_shape;
				uint_fast32 lfo_delay; // 100 to 65536
				uint_fast32 lfo_attack; // 100 to 65536
				int_fast16 lfo_amount; // -128 to 128
				uint_fast32 lfo_speed; // 200 to 65536
			};
			Envelope pan_envelope;
			Envelope vol_envelope;
			Envelope mod_x_envelope;
			Envelope mod_y_envelope;
			Envelope pitch_envelope;

			// specific to sampler, hybrid generators(with samples) and audio clip channels
			struct {
				// Note: I think these values might be different in FL pre v12
				enum : uint_fast8 {
					resampling,
					e3generic,
					e3mono,
					slice_stretch,
					slice_map,
					automatic,
					e2generic,
					e2transient,
					e2mono,
					e2speech,
				} mode;
				int_fast16 pitch_shift; // -1200 to 1200
				int_fast16 multiplicator; // -20000 to 20000
				unsigned time;
			} time_stretching;

			struct {
				uint_fast16 fade_in; // 0 to 1024
				uint_fast16 fade_out; // same
				uint_fast16 pogo; // 0 to 512, 256 is center(no effect)
				bool remove_dc : 1, flip : 1, normalize : 1;
				bool fade_stereo: 1, reverse : 1, swap_stereo : 1;
			} precomputed_fx;

			std::wstring sample_filename;

			struct {
				bool keep_on_disk : 1;
				bool load_regions : 1;
				bool load_acid_markers : 1;
				bool resample : 1;
			} sample_flags;

			// specific to sampler channels
			bool use_loop_points;
			uint_fast8 loop_type; // 1 means ping pong loop

			// specific to layer
			struct {
				bool crossfade : 1;
				bool random : 1;
			} layer_flags;
			uint_fast16 layer_crossfade_level; // 0 to 256
			std::vector<unsigned> layered_channels; // indices
		};

		std::vector<std::wstring> channel_filters;
		int_least32 current_channel_filter_index; // -1 means "All"
		std::vector<Channel> channels;
	} channel_rack_state;

	class Pattern {
	public:
		struct Note {
			uint_fast32 position;
			uint_fast16 rack_channel;
			uint_fast32 length; // in ticks
			uint_fast8 key; // C5 is 60
			uint_fast8 group_id; // note group identifier, 0 is no group
			uint_fast8 fine_pitch; // 0 - 240, 120 is default
			uint_fast8 release;
			uint_fast8 midi_channel;
			uint_fast8 pan; // 0 to 128, 64 is default
			uint_fast8 velocity; // 0 to 128
			uint_fast8 mod_x; // 0 to 255, 128 is default
			uint_fast8 mod_y; // 0 to 255, 128 is default
			bool slide : 1;
			bool porta : 1;
		};

		struct TimeMarker {
			bool pattern_length;
			uint_fast32 position;
			std::wstring name;
		};

		template<typename Iter>
		void read_events(Iter&);

	private:
		std::wstring name;
		uint32 color; // ABGR
		bool ss_loop_enabled;

		std::vector<TimeMarker> time_markers;
		std::vector<Note> notes;
	};
	std::vector<Pattern> patterns;

	struct MixerState {
		bool apdc_enabled;
		class MixerTrack {
		public:
			template<typename Iter>
			void read_events(Iter&);
		private:
			struct Routing {
				unsigned target;
				float amount;
			};
			bool allow_threaded_processing;
			std::wstring name;
			uint32 color; // ABGR
			uint_least16 icon;
			int_fast32 audio_input_source_index = -1; // -1 means none
			int_fast32 audio_output_target_index = -1; // -1 means none
			std::vector<Plugin> plugins;
			std::vector<Routing> routed_to;
		} tracks[105];
	} mixer_state;

	struct PlaylistLane {
		PlaylistLane() = default;
		template<typename Iter>
		void read_events(Iter&);
	private:
		std::wstring name;
	};

	struct PlaylistState {
		struct Clip {
			char data[32];
		};

		struct TimeMarker {
			uint_fast32 position;
			std::wstring name;
		};
		uint_fast32 selection_start; // in PPQ
		uint_fast32 selection_end; // in PPQ
		std::vector<TimeMarker> time_markers;
		std::vector<PlaylistLane> lanes;
		std::vector<Clip> clips;
	} playlist_state;

	uint_fast32 tempo; // 10000 to 522000
	uint_fast16 current_pattern_index; // 1 indexed
	bool loop_active;
	uint_least8 shuffle; // 0 to 128
	int_fast16 main_pitch; // -1200 to 1200

	struct {
		uint_least8 steps_per_beat;
		uint_least8 beats_per_bar;
	} time_signature;

	enum class PanLaw : uint8 {
		Circular = 0,
		Triangular = 2
	} pan_law;
	bool play_truncated_notes;
	bool show_project_info;

	struct {
		std::wstring title;
		std::wstring genre;
		std::wstring author;
		std::wstring project_data_path;
		std::wstring comment;
		std::wstring url;
		// TODO: change to c++ style time
		// NOTE: integral part is days since 30.12.1899, fractional part is time
		double start_time;
		double working_time;
	} project_info;
};

// this assumes little endian
template<typename SourceT, size AdvanceBytes, typename TargetT>
void read_into(TargetT& target, unsigned char const*& source) {
	static_assert(sizeof(TargetT) >= sizeof(SourceT), "Target type should be at least as big as source type");
	static_assert((std::is_unsigned<SourceT>::value && std::is_unsigned<TargetT>::value) ||
	              (std::is_signed<SourceT>::value && std::is_signed<TargetT>::value)     ||
	              std::is_enum<TargetT>::value, // TODO: check enum signedness
	              "Source type and Target type must have same signedness");
	static_assert(AdvanceBytes >= sizeof(SourceT) ||
	              AdvanceBytes == 0,
	              "must skip more bytes than were read");

	SourceT source_temp;
	std::memcpy(&source_temp, source, sizeof(SourceT));
	target = static_cast<TargetT>(source_temp);
	source += AdvanceBytes;
}

template<typename Iter>
DeserializedFLP DeserializedFLP::from_events(Iter begin, Iter end) {
	return DeserializedFLP(begin, end);
}

template<typename Iter>
DeserializedFLP::DeserializedFLP(Iter events_begin, Iter events_end) {
	auto  it = events_begin;

	// read general project stuff
	assert(it->type == FLPEventType::FLP_Version);
	assert(!(Version(reinterpret_cast<char const*>(it->text_data.get())) < "12"));
	++it;
	assert(it->type == FLPEventType::FLP_Registered);
	++it;
	assert(it->type == FLPEventType::FLP_RegName);
	++it;
	assert(it->type == FLPEventType::FLP_FineTempo);
	tempo = static_cast<uint_fast32>(it->i32);
	++it;
	assert(it->type == FLPEventType::FLP_CurrentPatNum);
	current_pattern_index = static_cast<uint_fast16>(it->i16);
	++it;
	assert(it->type == FLPEventType::FLP_LoopActive);
	loop_active = !!it->u8;
	++it;
	assert(it->type == FLPEventType::FLP_Shuffle);
	shuffle = it->u8;
	++it;
	assert(it->type == FLPEventType::FLP_MainPitch);
	main_pitch = it->i16;
	++it;
	assert(it->type == FLPEventType::FLP_TimeSig_Num);
	time_signature.steps_per_beat = it->u8;
	++it;
	assert(it->type == FLPEventType::FLP_TimeSig_Beat);
	time_signature.beats_per_bar = it->u8;
	++it;
	if(it->type == FLPEventType::FLP_PLSel) {
		assert(it->var_size == 8);
		unsigned char const* ptr = it->text_data.get();
		read_into<uint32, 4>(playlist_state.selection_start, ptr);
		read_into<uint32, 0>(playlist_state.selection_end, ptr);
		++it;
	}
	assert(it->type == FLPEventType::FLP_PanVolTab);
	pan_law = static_cast<decltype(pan_law)>(it->u8);
	++it;
	if(it->type == FLPEventType::FLP_TruncateClipNotes) {
		play_truncated_notes = !!it->u8;
		++it;
	}
	assert(it->type == FLPEventType::FLP_ShowInfo);
	show_project_info = !!it->u8;
	++it;
	assert(it->type == FLPEventType::FLP_Text_Title);
	project_info.title =
		reinterpret_cast<wchar_t const*>(it->text_data.get());
	++it;
	assert(it->type == FLPEventType::FLP_Text_Genre);
	project_info.genre =
		reinterpret_cast<wchar_t const*>(it->text_data.get());
	++it;
	assert(it->type == FLPEventType::FLP_Text_Author);
	project_info.author =
		reinterpret_cast<wchar_t const*>(it->text_data.get());
	++it;
	assert(it->type == FLPEventType::FLP_Text_ProjDataPath);
	project_info.project_data_path =
		reinterpret_cast<wchar_t const*>(it->text_data.get());
	++it;
	assert(it->type == FLPEventType::FLP_Text_Comment);
	project_info.comment =
		reinterpret_cast<wchar_t const*>(it->text_data.get());
	++it;
	if(it->type == FLPEventType::FLP_Text_URL) {
		project_info.url =
			reinterpret_cast<wchar_t const*>(it->text_data.get());
		++it;
	}
	assert(it->type == FLPEventType::FLP_ProjectTime);
	assert(it->var_size == 16);
	unsigned char const* ptr = it->text_data.get();
	// NOTE: Assumes 64bit IEEE 754 double
	read_into<double, 8>(project_info.start_time, ptr);
	read_into<double, 0>(project_info.working_time, ptr);
	++it;

	while(it->type == FLPEventType::FLP_Text_ChanFilter) {
		channel_rack_state.channel_filters.emplace_back(
			reinterpret_cast<wchar_t const*>(it->text_data.get())
		);
		++it;
	}
	assert(it->type == FLPEventType::FLP_CurrentFilterNum);
	channel_rack_state.current_channel_filter_index =
		(it->i32 == -1) ? -1 : it->i32;
	++it;
	if(it->type == FLPEventType::FLP_CtrlRecChan) {
		// TODO:
		++it;
	}

	while(it != events_end) {
		switch(it->type) {
		case FLPEventType::FLP_NewPat:
			do {
				auto const pat_index = static_cast<size>(it->i16);
				if(pat_index >= patterns.size()) {
					patterns.resize(pat_index + 1);
				}
				patterns[pat_index].read_events(it);
			} while(it->type == FLPEventType::FLP_NewPat);
			continue;
		case FLPEventType::FLP_NewChan: {
			auto const index = static_cast<size>(it->i16);
			if(index >= channel_rack_state.channels.size()) {
				channel_rack_state.channels.resize(index + 1);
			}
			channel_rack_state.channels[index].read_events(it);
			continue;
		}
		//read mixer
		case FLPEventType::FLP_APDC:
			mixer_state.apdc_enabled = !!it->u8;
			++it;
			assert(it->type == FLPEventType::FLP_EEAutoMode);
			// fallthrough
		// TODO:
		case FLPEventType::FLP_EEAutoMode: {
			auto b = it->u8;
			(void)b;
			++it;
			assert(it->type == FLPEventType::FLP_FXParams);
			// fallthrough
		}
		case FLPEventType::FLP_FXParams: {
			size mixer_track_counter = 0;
			do {
				mixer_state.tracks[mixer_track_counter++].read_events(it);
			} while(it->type == FLPEventType::FLP_FXParams);
			assert(mixer_track_counter == 105);
			continue;
		}
		// TODO: playlist
		case FLPEventType::FLP_PLRecChan: {
			// read playlist clips
			#pragma pack(push, 1)
			struct PLClip {
				uint32 position;
				unsigned char flags[4];
				uint32 duration;
				unsigned char data[12];
				uint32 window_start;
				uint32 window_end;
			};
			#pragma pack(pop)
			//static_assert(sizeof(PLClip) == 32, "");

			std::vector<PLClip> clips(it->var_size / 32);
			std::memcpy(clips.data(), it->text_data.get(), it->var_size);

			++it;
			while(it->type == FLPEventType::FLP_NewTimeMarker) {
				PlaylistState::TimeMarker tm {};
				tm.position = static_cast<uint32>(it->i32);
				++it;
				if(it->type == FLPEventType::FLP_Text_TimeMarker) {
					tm.name = reinterpret_cast<wchar_t const*>(it->text_data.get());
					++it;
				}
				playlist_state.time_markers.push_back(tm);
			}
			continue;
		}
		case FLPEventType::FLP_PLTrackInfo:
			do {
				playlist_state.lanes.emplace_back();
				playlist_state.lanes.back().read_events(it);
			} while(it->type == FLPEventType::FLP_PLTrackInfo);
			continue;
		// TODO:
		case FLPEventType::FLP_RemoteCtrl_MIDI:
			assert(it->var_size == 20);
			++it;
			continue;
		// TODO:
		case FLPEventType::FLP_RemoteCtrl_Int:
			++it;
			// TODO: check for FLP_Text_RemoteCtrlFormula
			continue;
		// TODO:
		case FLPEventType::FLP_InitCtrlRecChan:
			++it;
			continue;
		// TODO:
		case FLPEventType::FLP_WindowH:
			++it;
			continue;
		default:
			std::cerr << "DeserializedFLP::constructor: unexpected event type : \""
			          << get_event_name_or_number(it->type)
			          << "\"\n";
			++it;
			continue;
		}
	}
}

template<typename Iter>
DeserializedFLP::Plugin::Plugin(Iter& event_it) {
	auto it = event_it;

	assert(it->type == FLPEventType::FLP_Text_DefPluginName);
	name = reinterpret_cast<wchar_t const*>(it->text_data.get());
	++it;

	// TODO: figure out what's in FLP_NewPlugin
	assert(it->type == FLPEventType::FLP_NewPlugin);
	assert(it->var_size == 52);
	++it;

	if(it->type == FLPEventType::FLP_Text_PluginName) {
		display_name =
			reinterpret_cast<wchar_t const*>(it->text_data.get());
		++it;
	}

	assert(it->type == FLPEventType::FLP_PluginIcon);
	icon = static_cast<decltype(icon)>(it->i32);
	++it;

	assert(it->type == FLPEventType::FLP_PluginColor);
	color = static_cast<uint32>(it->i32);
	++it;

	if(it->type == FLPEventType::FLP_PluginParams) {
		param_size = it->var_size;
		params = std::move(it->text_data);
		++it;
	}

	event_it = it;
}

template<typename Iter>
void DeserializedFLP::ChannelRackState::Channel::read_events(Iter& event_it) {
	using std::memcpy;

	auto read_tracking = [] (DeserializedFLP::ChannelRackState::Channel::Tracking& tracking,
	                         unsigned char const* ptr) {
		read_into<uint8, 4>(tracking.middle_note, ptr);
		read_into<int16, 4>(tracking.pan, ptr);
		read_into<int16, 4>(tracking.mod_x, ptr);
		read_into<int16, 4>(tracking.mod_y, ptr);
	};
	
	auto read_envelope = [] (DeserializedFLP::ChannelRackState::Channel::Envelope& envelope,
	                         unsigned char const* ptr) {
		uint8 flags = 0;
		memcpy(&flags, ptr, sizeof(flags));
		ptr += sizeof(int32);
		envelope.tempo_synced     = !!(flags & 0x01);
		envelope.lfo_tempo_synced = !!(flags & 0x02);
		envelope.lfo_global       = !!(flags & 0x20);
		uint32 enabled;
		read_into<uint32, 4>(enabled, ptr);
		envelope.enabled = !!enabled;
		read_into<uint32, 4>(envelope.delay, ptr);
		read_into<uint32, 4>(envelope.attack, ptr);
		read_into<uint32, 4>(envelope.hold, ptr);
		read_into<uint32, 4>(envelope.decay, ptr);
		read_into<uint8,  4>(envelope.sustain, ptr);
		read_into<uint32, 4>(envelope.release, ptr);
		read_into<int16,  4>(envelope.amount, ptr);
		read_into<uint32, 4>(envelope.lfo_delay, ptr);
		read_into<uint32, 4>(envelope.lfo_attack, ptr);
		read_into<int16,  4>(envelope.lfo_amount, ptr);
		read_into<uint32, 4>(envelope.lfo_speed, ptr);
		read_into<uint8,  4>(envelope.lfo_shape, ptr);
		read_into<int16,  4>(envelope.attack_tension, ptr);
		read_into<int16,  4>(envelope.decay_tension, ptr);
		read_into<int16,  0>(envelope.release_tension, ptr);
	};

	auto it = event_it;

	assert(it->type == FLPEventType::FLP_NewChan);
	++it;

	assert(it->type == FLPEventType::FLP_ChanType);
	type = static_cast<ChannelType>(it->u8);

	switch(type) {
	case ChannelType::Sampler:
	case ChannelType::Plugin:
	case ChannelType::Layer:
	case ChannelType::AudioClip:
	case ChannelType::AutomationClip:
		break;
	default:
		throw std::runtime_error {"Unknown channel type!"};
	}
	++it;

	plugin = Plugin(it);

	// set defaults for these values in case the events are missing
	middle_note = 60;
	fine_tune = 0;

	unsigned char const* text_ptr = nullptr;

	for(;;) {
		// TODO: check for ALL possible termination conditions...
		if(it->type == FLPEventType::FLP_NewChan ||
		   it->type == FLPEventType::FLP_NewTimeMarker ||
		   it->type == FLPEventType::FLP_NewPat ||
		   it->type == FLPEventType::FLP_PLRecChan ||
		   it->type == FLPEventType::FLP_PLTrackInfo ||
		   it->type == FLPEventType::FLP_RemoteCtrl_MIDI ||
		   it->type == FLPEventType::FLP_APDC) {
			break;
		}

		switch(it->type) {
		case FLPEventType::FLP_ChanEnabled:
			enabled = !!it->u8;
			break;
		case FLPEventType::FLP_Delay:
			assert(it->var_size == 20);
			text_ptr = it->text_data.get();
			read_into<uint16, sizeof(uint32)>(delay.feed, text_ptr);
			read_into<int16, sizeof(uint32)>(delay.pan, text_ptr);
			read_into<int16, sizeof(uint32)>(delay.pitch, text_ptr);
			read_into<uint8, sizeof(uint32)>(delay.num_echoes, text_ptr);
			read_into<uint16, 0>(delay.time, text_ptr);
			break;
		case FLPEventType::FLP_DelayFRes:
			delay.mod_x = static_cast<decltype(delay.mod_x)>(it->i32 >> 0x10);
			delay.mod_y = static_cast<decltype(delay.mod_y)>(it->i32 & 0x1FF);
			break;
		case FLPEventType::FLP_Reverb:
			// ignore legacy fx
			break;
		case FLPEventType::FLP_ShiftTime:
			shift_time = static_cast<decltype(shift_time)>(it->i16);
			break;
		case FLPEventType::FLP_SwingMix:
			swing_mix = static_cast<decltype(swing_mix)>(it->i16);
			break;
		case FLPEventType::FLP_FX3: // Pogo
			precomputed_fx.pogo = static_cast<decltype(precomputed_fx.pogo)>(it->i16);
			break;
		case FLPEventType::FLP_FXDecay:
			precomputed_fx.fade_out = static_cast<uint_fast16>(it->i16);
			break;
		case FLPEventType::FLP_FXAttack:
			precomputed_fx.fade_in = static_cast<uint_fast16>(it->i16);
			break;
		case FLPEventType::FLP_FX:
		case FLPEventType::FLP_FXCut:
		case FLPEventType::FLP_FXRes:
		case FLPEventType::FLP_FXPreamp:
		case FLPEventType::FLP_FXStDel:
		case FLPEventType::FLP_FXSine:
			// legacy fx
			break;
		case FLPEventType::FLP_FXFlags:
			precomputed_fx.reverse     = !!(it->i16 & 0x002);
			precomputed_fx.fade_stereo = !!(it->i16 & 0x001);
			precomputed_fx.swap_stereo = !!(it->i16 & 0x100);
			break;
		case FLPEventType::FLP_TargetFXTrack:
			target_mixer_track = it->u8;
			break;
		case FLPEventType::FLP_LayerChan: {
			assert(type == ChannelType::Layer);
			auto layered_chan_index = static_cast<typename decltype(layered_channels)::value_type>(it->i16);
			layered_channels.push_back(layered_chan_index);
			break;
		}
		case FLPEventType::FLP_ChanLevels:
			assert(it->var_size == 24);
			text_ptr = it->text_data.get();
			read_into<uint16, 4>(levels.pan,   text_ptr);
			read_into<uint16, 4>(levels.vol,   text_ptr);
			read_into<int16,  4>(levels.pitch, text_ptr);
			read_into<uint16, 4>(levels.mod_x, text_ptr);
			read_into<uint16, 4>(levels.mod_y, text_ptr);
			read_into<uint8,  4>(levels.filter_type, text_ptr);
			assert(type == ChannelType::Sampler ||
				   type == ChannelType::Plugin  || // ideally should only check for hybrid plugins, but don't know how
				   levels.filter_type == decltype(levels.filter_type)::FastLP);
			break;
		case FLPEventType::FLP_ChanOfsLevels:
			text_ptr = it->text_data.get();
			read_into<int32,  4>(levels_adjustment.pan, text_ptr);
			read_into<uint16, 4>(levels_adjustment.vol, text_ptr);
			// TODO: what's here?
			text_ptr += sizeof(int32);
			read_into<int32, 4>(levels_adjustment.mod_x, text_ptr);
			read_into<int32, 4>(levels_adjustment.mod_y, text_ptr);
			break;
		case FLPEventType::FLP_ChanPoly: {
			text_ptr = it->text_data.get();
			read_into<uint8,  4>(polyphony.max, text_ptr);
			read_into<uint16, 4>(polyphony.slide, text_ptr);
			auto bits = static_cast<unsigned char>(*text_ptr);
			polyphony.porta = !!(bits & 2);
			polyphony.mono  = !!(bits & 1);
			break;
		}
		// TODO:
		case FLPEventType::FLP_ChanParams:
			// includes: Crossfade loop, trim, declicking mode
			text_ptr = it->text_data.get();
			delay.ping_pong   = !!(text_ptr[10] & 2);
			delay.fat_mode    = !!(text_ptr[10] & 4);
			follow_main_pitch = !!(text_ptr[11] & 1);
			full_porta        = !!(text_ptr[62] & 1);
			add_to_key        = !!(text_ptr[63] & 1);
			gate = 0;
			memcpy(&gate, text_ptr + 64, sizeof(uint16));
			key_zone_start = 0;
			memcpy(&key_zone_start, text_ptr + 68, sizeof(uint8));
			key_zone_end = 0;
			memcpy(&key_zone_end, text_ptr + 72, sizeof(uint8));
			key_zone_active = !!(text_ptr[73] & 1);
			layer_crossfade_level = 0;
			memcpy(&layer_crossfade_level, text_ptr + 76, sizeof(uint16));
			memcpy(&time_stretching.pitch_shift, text_ptr + 100, sizeof(int16));
			memcpy(&time_stretching.multiplicator, text_ptr + 104, sizeof(int16));
			time_stretching.mode = static_cast<decltype(time_stretching.mode)>(0);
			memcpy(&time_stretching.mode, text_ptr + 108, sizeof(uint8));
			break;
		case FLPEventType::FLP_CutCutBy: {
			assert(type != ChannelType::Layer);
			auto d = static_cast<uint32>(it->i32);
			cut    = static_cast<decltype(cut)>(d & 0xFFFF);
			cut_by = static_cast<decltype(cut_by)>(d >> 0x10);
			break;
		}
		// TODO:
		case FLPEventType::FLP_LayerFlags: {
			auto flags = static_cast<uint32>(it->i32);
			layer_flags.random    = !!(flags & 1);
			layer_flags.crossfade = !!(flags & 2);
			break;
		}
		case FLPEventType::FLP_ChanFilterNum:
			filter_index = static_cast<decltype(filter_index)>(it->i32);
			break;
		// TODO: channel articulator
		case FLPEventType::FLP_ChanAC:
			break;
		case FLPEventType::FLP_Tracking: {
			text_ptr = it->text_data.get();
			read_tracking(vol_tracking, text_ptr);
			++it;
			assert(it->type == FLPEventType::FLP_Tracking);
			text_ptr = it->text_data.get();
			read_tracking(key_tracking, text_ptr);
			break;
		}
		case FLPEventType::FLP_Envelope:
			read_envelope(pan_envelope, it->text_data.get());
			++it;
			assert(it->type == FLPEventType::FLP_Envelope);
			read_envelope(vol_envelope, it->text_data.get());
			++it;
			assert(it->type == FLPEventType::FLP_Envelope);
			read_envelope(mod_x_envelope, it->text_data.get());
			++it;
			assert(it->type == FLPEventType::FLP_Envelope);
			read_envelope(mod_y_envelope, it->text_data.get());
			++it;
			assert(it->type == FLPEventType::FLP_Envelope);
			read_envelope(pitch_envelope, it->text_data.get());
			break;
		case FLPEventType::FLP_SampleFlags: {
			auto dw = it->i32;
			sample_flags.resample          = !!(dw &     1);
			sample_flags.load_regions      = !!(dw &     2);
			sample_flags.load_acid_markers = !!(dw &     4);
			use_loop_points                = !!(dw &     8);
			sample_flags.keep_on_disk      = !!(dw & 0x100);
			break;
		}
		case FLPEventType::FLP_LoopType:
			loop_type = it->u8;
			break;
		case FLPEventType::FLP_Text_SampleFileName:
			assert(type == ChannelType::Sampler ||
				   type == ChannelType::Plugin  ||
				   type == ChannelType::AudioClip);
			sample_filename = reinterpret_cast<wchar_t const*>(it->text_data.get());
			break;
		case FLPEventType::FLP_FineTune:
			fine_tune = static_cast<decltype(fine_tune)>(it->i32);
			break;
		case FLPEventType::FLP_MiddleNote:
			middle_note = static_cast<decltype(middle_note)>(it->i32);
			break;
		default:
			std::cerr << "Channel::read_events: unexpected event \"" << get_event_name_or_number(it->type) << "\"\n";
			break;
		}

		++it;
	}
	event_it = it;
}

template<typename Iter>
void DeserializedFLP::Pattern::read_events(Iter& event_it) {
	auto it = event_it;
	assert(it->type == FLPEventType::FLP_NewPat);
	++it;

	for(;;) {
		// TODO: handle ALL stop cases
		if(it->type == FLPEventType::FLP_NewChan ||
		   it->type == FLPEventType::FLP_NewPat ||
		   it->type == FLPEventType::FLP_PLRecChan ||
		   it->type == FLPEventType::FLP_PLTrackInfo ||
		   it->type == FLPEventType::FLP_RemoteCtrl_MIDI ||
		   it->type == FLPEventType::FLP_APDC) {
			break;
		}
		switch(it->type) {
		// TODO: read note data
		case FLPEventType::FLP_PatNoteRecChan: {
			unsigned char const* note_data = it->text_data.get();
			constexpr uint32 note_size = 24;
			assert(it->var_size % note_size == 0);
			RawPatternNote raw_note;
			for(uint32 i = 0; i < it->var_size; i += note_size) {
				std::memcpy(&raw_note, note_data + i, note_size);
				Note note {};
				note.position = raw_note.position;
				note.rack_channel = raw_note.rack_channel;
				note.length = raw_note.length;
				note.key = raw_note.key;
				note.group_id = raw_note.group_id;
				note.fine_pitch = raw_note.fine_pitch;
				note.release = raw_note.release;
				note.midi_channel = (raw_note.midi_channel & 0xF);
				note.pan = raw_note.pan;
				note.velocity = raw_note.velocity;
				note.mod_x = raw_note.mod_x;
				note.mod_y = raw_note.mod_y;
				note.slide = !!(raw_note.flags & RawPatternNote::Flag::slide);
				note.porta = !!(raw_note.midi_channel & RawPatternNote::Flag::porta);
				notes.push_back(note);
			}

			++it;
			break;
		}
		// TODO:
		case FLPEventType::FLP_PatCtrlRecChan:
			++it;
			break;
		case FLPEventType::FLP_Text_PatName:
			name = reinterpret_cast<wchar_t const*>(it->text_data.get());
			++it;
			break;
		case FLPEventType::FLP_PatColor:
			color = static_cast<uint32>(it->i32);
			++it;
			break;
		case FLPEventType::FLP_SSLoop:
			ss_loop_enabled = !!it->u8;
			++it;
			break;
		case FLPEventType::FLP_NewTimeMarker: {
			enum TimeMarkerFlags : uint8 {
				pattern_length = 6
			};
			uint32 data = static_cast<uint32>(it->i32);
			// NOTE: taking upper 8 bit as flags and lower 24 bit as position
			// this is just guessing
			uint8 flag_part = (data & 0xFF000000) >> 24;
			TimeMarker tm = {};
			tm.position = data & 0x00FFFFFF;
			tm.pattern_length = ((flag_part & pattern_length) == pattern_length);
			++it;
			if(it->type == FLPEventType::FLP_Text_TimeMarker) {
				tm.name = reinterpret_cast<wchar_t const*>(it->text_data.get());
				++it;
			}
			time_markers.push_back(std::move(tm));
			break;
		}
		default:
			std::cerr << "Pattern::read_events: unexpected event \"" << get_event_name_or_number(it->type) << "\"\n";
			++it;
			break;
		}
	}
	event_it = it;
}

template<typename Iter>
void DeserializedFLP::MixerState::MixerTrack::read_events(Iter& event_it) {

	auto it = event_it;

	// TODO: figure out what's in FLP_FXParams
	assert(it->type == FLPEventType::FLP_FXParams);
	assert(it->var_size == 8);
	unsigned char const* text_ptr = it->text_data.get();
	allow_threaded_processing = !(text_ptr[4] & 0x10);
	++it;

	for(;;) {
		switch(it->type) {
		// TODO: check for ALL possible termination conditions...
		case FLPEventType::FLP_NewChan:
		case FLPEventType::FLP_NewPat:
		case FLPEventType::FLP_PLTrackInfo:
		case FLPEventType::FLP_FXParams:
		case FLPEventType::FLP_PLRecChan:
		case FLPEventType::FLP_InitCtrlRecChan:
		case FLPEventType::FLP_WindowH:
			break;
		case FLPEventType::FLP_Text_DefPluginName: {
			Plugin plugin(it);
			if(it->type == FLPEventType::FLP_FXInsertIndex) {
				auto fxinsert_index = it->i16;
				if(fxinsert_index >= plugins.size()) {
					plugins.resize(fxinsert_index + 1);
				}
				plugins[fxinsert_index] = std::move(plugin);
				++it;
			} else {
				plugins.push_back(std::move(plugin));
			}
			continue;
		}
		case FLPEventType::FLP_FXInsertIndex:
			// if this event arrives without a plugin before it, the mixer track insert has no plugin
			if(it->i16 >= plugins.size()) {
				plugins.resize(it->i16 + 1);
			}
			++it;
			continue;
		case FLPEventType::FLP_FXRouting: {
			assert(it->var_size == 105);
			text_ptr = it->text_data.get();
			auto num_tracks = static_cast<unsigned>(it->var_size);
			for(auto i = 0U; i < num_tracks; i++) {
				if(text_ptr[i] != 0) {
					// TODO: routing level is somewhere inside InitCtrlRecChan... FML
					routed_to.push_back(Routing {i, 0.f});
				}
			}
			++it;
			continue;
		}
		case FLPEventType::FLP_FXInChanNum:
			audio_input_source_index = it->i32;
			++it;
			continue;
		case FLPEventType::FLP_FXOutChanNum:
			audio_output_target_index = it->i32;
			++it;
			continue;
		case FLPEventType::FLP_FXColor:
			color = static_cast<uint32>(it->i32);
			++it;
			continue;
		case FLPEventType::FLP_FXIcon:
			icon = static_cast<uint_least16>(it->i16);
			++it;
			continue;
		case FLPEventType::FLP_Text_FXName:
			name = reinterpret_cast<wchar_t const*>(it->text_data.get());
			++it;
			continue;
		default:
			std::cerr << "MixerTrack::read_events: unexpected event \"" << get_event_name_or_number(it->type) << "\"\n";
			++it;
			continue;
		}
		break;
	}
	event_it = it;
}

template<typename Iter>
void DeserializedFLP::PlaylistLane::read_events(Iter& it) {
	assert(it->type == FLPEventType::FLP_PLTrackInfo);
	// TODO: parse FLP_PLTrackInfo
	++it;
	if(it->type == FLPEventType::FLP_Text_PLTrackName) {
		name = reinterpret_cast<wchar_t const*>(it->text_data.get());
		++it;
	}
}
