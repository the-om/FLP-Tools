#include "flp_enums.h"


char const* header_format_name(FLPFormat hf) noexcept {
	switch(hf) {
	case FLPFormat::FLP_Format_None:
		return "FLP_Format_None";
	case FLPFormat::FLP_Format_Song:
		return "FLP_Format_Song";
	case FLPFormat::FLP_Format_Score:
		return "FLP_Format_Score";
	case FLPFormat::FLP_Format_Auto:
		return "FLP_Format_Auto";
	case FLPFormat::FLP_Format_ChanState:
		return "FLP_Format_ChanState";
	case FLPFormat::FLP_Format_PlugState:
		return "FLP_Format_PlugState";
	case FLPFormat::FLP_Format_PlugState_Gen:
		return "FLP_Format_PlugState_Gen";
	case FLPFormat::FLP_Format_PlugState_FX:
		return "FLP_Format_PlugState_FX";
	case FLPFormat::FLP_Format_MixerState:
		return "FLP_Format_MixerState";
	case FLPFormat::FLP_Format_Patcher:
		return "FLP_Format_Patcher";
	default:
		return nullptr;
	}
}

static constexpr char const* const event_names[256] = {
	// 0 "FLP_Byte"
	"FLP_ChanEnabled",
	"FLP_NoteOn",
	"FLP_ChanVol",
	"FLP_ChanPan",
	"FLP_MIDIChan",
	"FLP_MIDINote",
	"FLP_MIDIPatch",
	"FLP_MIDIBank",
	// 8
	nullptr,
	"FLP_LoopActive",
	"FLP_ShowInfo",
	"FLP_Shuffle",
	"FLP_MainVol",
	"FLP_FitToSteps",
	"FLP_Pitchable",
	"FLP_Zipped",
	// 16
	"FLP_Delay_Flags",
	"FLP_TimeSig_Num",
	"FLP_TimeSig_Beat",
	"FLP_UseLoopPoints",
	"FLP_LoopType",
	"FLP_ChanType",
	"FLP_TargetFXTrack",
	"FLP_PanVolTab",
	// 24
	"FLP_nStepsShown",
	"FLP_SSLength",
	"FLP_SSLoop",
	"FLP_FXProps",
	"FLP_Registered",
	"FLP_APDC",
	"FLP_TruncateClipNotes",
	"FLP_EEAutoMode",
	// 32
	nullptr,  // TODO: event 32 is used since FL 12
	nullptr,
	nullptr,
	nullptr,  // TODO: event 35 is used since FL 20
	nullptr,  // TODO: event 36 is used since FL 20
	nullptr,  // TODO: event 37 is used since FL 20
	nullptr,
	nullptr,
	// 40
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	// 64 "FLP_Word"
	"FLP_NewChan",
	"FLP_NewPat",
	"FLP_Tempo",
	"FLP_CurrentPatNum",
	"FLP_PatData",
	"FLP_FX",
	"FLP_FXFlags",
	"FLP_FXCut",
	// 72
	"FLP_DotVol",
	"FLP_DotPan",
	"FLP_FXPreamp",
	"FLP_FXDecay",
	"FLP_FXAttack",
	"FLP_DotNote",
	"FLP_DotPitch",
	"FLP_DotMix",
	// 80
	"FLP_MainPitch",
	"FLP_RandChan",
	"FLP_MixChan",
	"FLP_FXRes",
	"FLP_OldSongLoopPos",
	"FLP_FXStDel",
	"FLP_FX3",
	"FLP_DotFRes",
	// 88
	"FLP_DotFCut",
	"FLP_ShiftTime",
	"FLP_LoopEndBar",
	"FLP_Dot",
	"FLP_DotShift",
	"FLP_Tempo_Fine",
	"FLP_LayerChan",
	"FLP_FXIcon",
	// 96
	"FLP_DotRel",
	"FLP_SwingMix",
	"FLP_FXInsertIndex",
	nullptr,  // TODO: event 99 is used since FL 20
	nullptr,  // TODO: event 100 is used since FL 20
	nullptr,
	nullptr,
	nullptr,
	// 104
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	// 128 "FLP_Int",
	"FLP_PluginColor",
	"FLP_PLItem",
	"FLP_Echo",
	"FLP_FXSine",
	"FLP_CutCutBy",
	"FLP_WindowH",
	nullptr,
	"FLP_MiddleNote",
	// 136
	"FLP_Reserved",
	"FLP_MainResCut",
	"FLP_DelayFRes",
	"FLP_Reverb",
	"FLP_StretchTime",
	"FLP_SSNote",
	"FLP_FineTune",
	"FLP_SampleFlags",
	// 144
	"FLP_LayerFlags",
	"FLP_ChanFilterNum",
	"FLP_CurrentFilterNum",
	"FLP_FXOutChanNum",
	"FLP_NewTimeMarker",
	"FLP_FXColor",
	"FLP_PatColor",
	"FLP_PatAutoMode",
	// 152
	"FLP_SongLoopPos",
	"FLP_AUSmpRate",
	"FLP_FXInChanNum",
	"FLP_PluginIcon",
	"FLP_FineTempo",
	nullptr, // TODO: event 157 is used since FL 12
	nullptr, // TODO: event 158 is used since FL 20
	nullptr, // TODO: event 159 is used since FL 20
	// 160
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	// 192 "FLP_Text", "FLP_Undef",
	"FLP_Text_ChanName",
	"FLP_Text_PatName",
	"FLP_Text_Title",
	"FLP_Text_Comment",
	"FLP_Text_SampleFileName",
	"FLP_Text_URL",
	"FLP_Text_CommentRTF",
	"FLP_Version",
	// 200
	"FLP_RegName",
	"FLP_Text_DefPluginName",
	"FLP_Text_ProjDataPath",
	"FLP_Text_PluginName",
	"FLP_Text_FXName",
	"FLP_Text_TimeMarker",
	"FLP_Text_Genre",
	"FLP_Text_Author",
	// 208
	"FLP_MIDICtrls",
	"FLP_Delay",
	"FLP_TS404Params",
	"FLP_DelayLine",
	"FLP_NewPlugin",
	"FLP_PluginParams",
	"FLP_Reserved2",
	"FLP_ChanParams",
	// 216
	"FLP_CtrlRecChan",
	"FLP_PLSel",
	"FLP_Envelope",
	"FLP_ChanLevels",
	"FLP_ChanFilter",
	"FLP_ChanPoly",
	"FLP_NoteRecChan",
	"FLP_PatCtrlRecChan",
	// 224
	"FLP_PatNoteRecChan",
	"FLP_InitCtrlRecChan",
	"FLP_RemoteCtrl_MIDI",
	"FLP_RemoteCtrl_Int",
	"FLP_Tracking",
	"FLP_ChanOfsLevels",
	"FLP_Text_RemoteCtrlFormula",
	"FLP_Text_ChanFilter",
	// 232
	"FLP_RegBlackList",
	"FLP_PLRecChan",
	"FLP_ChanAC",
	"FLP_FXRouting",
	"FLP_FXParams",
	"FLP_ProjectTime",
	"FLP_PLTrackInfo",
	"FLP_Text_PLTrackName",
	// 240
	nullptr,
	nullptr,  // TODO: event 241 is used since FL 20
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	// 248
};

char const* flp_event_name(std::underlying_type_t<FLPEventType> event_id) noexcept {
	return event_names[event_id];
};
