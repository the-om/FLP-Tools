#pragma once

#include "type_defs.h"
#include <type_traits>


enum class FLPFormat : int16 {
	FLP_Format_None          = -1,                    // temporary
	FLP_Format_Song          = 0,                     // full project
	FLP_Format_Score         = 0x10,                  // score
	FLP_Format_Auto          = FLP_Format_Score + 8,  // automation
	FLP_Format_ChanState     = 0x20,                  // channel
	FLP_Format_PlugState     = 0x30,                  // plugin
	FLP_Format_PlugState_Gen = 0x31,
	FLP_Format_PlugState_FX  = 0x32,
	FLP_Format_MixerState    = 0x40,                  // mixer track
	FLP_Format_Patcher       = 0x50,                  // special: tells to Patcherize (internal)
};

enum class ChannelType {
	Sampler        = 0,
	Plugin         = 2,
	Layer          = 3,
	AudioClip      = 4,
	AutomationClip = 5
};

enum class PluginFlags {
	Plug_Visible      = 1,         // editor is visible or not
	Plug_Disabled     = 2,         // obsolete
	Plug_Detached     = 4,         // editor is detached
	Plug_Maximized    = 8,         // editor is maximized
	Plug_Generator    = 16,        // plugin is a generator (can be loaded into a channel)
	Plug_SD           = 32,        // smart disable option is on
	Plug_TP           = 64,        // threaded processing option is on
	Plug_Demo         = 128,       // saved with a demo version
	Plug_HideSettings = 1 << 8,    // editor is in compact mode
	Plug_Captionized  = 1 << 9,    // editor is captionized
	Plug_DX           = 1 << 16,   // indicates the plugin is a DirectX plugin (obsolete)
	Plug_EditorSize   = 2 << 16,   // editor size is specified (obsolete)
	Plug_EditorFlags  = Plug_Visible | Plug_Detached | Plug_Maximized | Plug_HideSettings | Plug_Captionized
};

enum class FLPEventType : uint8 {
	FLP_Byte              = 0,
	FLP_ChanEnabled       = 0,
	FLP_NoteOn            = 1,  // +pos
	FLP_ChanVol           = 2,  // obsolete
	FLP_ChanPan           = 3,  // obsolete
	FLP_MIDIChan          = 4,
	FLP_MIDINote          = 5,
	FLP_MIDIPatch         = 6,
	FLP_MIDIBank          = 7,
	FLP_LoopActive        = 9,
	FLP_ShowInfo          = 10,
	FLP_Shuffle           = 11,
	FLP_MainVol           = 12,  // obsolete
	FLP_FitToSteps        = 13,  // obsolete byte version
	FLP_Pitchable         = 14,  // obsolete
	FLP_Zipped            = 15,
	FLP_Delay_Flags       = 16,  // obsolete
	FLP_TimeSig_Num       = 17,
	FLP_TimeSig_Beat      = 18,
	FLP_UseLoopPoints     = 19,
	FLP_LoopType          = 20,
	FLP_ChanType          = 21,
	FLP_TargetFXTrack     = 22,
	FLP_PanVolTab         = 23,  // log vol & circular pan tables
	FLP_nStepsShown       = 24,  // obsolete
	FLP_SSLength          = 25,  // +length
	FLP_SSLoop            = 26,
	FLP_FXProps           = 27,  // FlipY, ReverseStereo, etc
	FLP_Registered        = 28,  // reg version
	FLP_APDC              = 29,
	FLP_TruncateClipNotes = 30,
	FLP_EEAutoMode        = 31,
	// TODO: event 32 is used in FL 12

	// WORD sized (63..127)
	FLP_Word           = 64,
	FLP_NewChan        = FLP_Word,
	FLP_NewPat         = FLP_Word + 1,   // +PatNum (word)
	FLP_Tempo          = FLP_Word + 2,   // obsolete, replaced by FLP_FineTempo
	FLP_CurrentPatNum  = FLP_Word + 3,
	FLP_PatData        = FLP_Word + 4,
	FLP_FX             = FLP_Word + 5,
	FLP_FXFlags        = FLP_Word + 6,
	FLP_FXCut          = FLP_Word + 7,
	FLP_DotVol         = FLP_Word + 8,
	FLP_DotPan         = FLP_Word + 9,
	FLP_FXPreamp       = FLP_Word + 10,
	FLP_FXDecay        = FLP_Word + 11,
	FLP_FXAttack       = FLP_Word + 12,
	FLP_DotNote        = FLP_Word + 13,
	FLP_DotPitch       = FLP_Word + 14,
	FLP_DotMix         = FLP_Word + 15,
	FLP_MainPitch      = FLP_Word + 16,
	FLP_RandChan       = FLP_Word + 17,  // obsolete
	FLP_MixChan        = FLP_Word + 18,  // obsolete
	FLP_FXRes          = FLP_Word + 19,
	FLP_OldSongLoopPos = FLP_Word + 20,  // obsolete
	FLP_FXStDel        = FLP_Word + 21,
	FLP_FX3            = FLP_Word + 22,
	FLP_DotFRes        = FLP_Word + 23,
	FLP_DotFCut        = FLP_Word + 24,
	FLP_ShiftTime      = FLP_Word + 25,
	FLP_LoopEndBar     = FLP_Word + 26,
	FLP_Dot            = FLP_Word + 27,
	FLP_DotShift       = FLP_Word + 28,
	FLP_Tempo_Fine     = FLP_Word + 29,  // obsolete, replaced by FLP_FineTempo
	FLP_LayerChan      = FLP_Word + 30,
	FLP_FXIcon         = FLP_Word + 31,
	FLP_DotRel         = FLP_Word + 32,
	FLP_SwingMix       = FLP_Word + 33,
	FLP_FXInsertIndex  = FLP_Word + 34,  // NOTE: undocumented, guessed name

	// DWORD sized (128..191)
	FLP_Int              = 128,
	FLP_PluginColor      = FLP_Int,
	FLP_PLItem           = FLP_Int + 1,   // Pos (word) +PatNum (word) (obsolete)
	FLP_Echo             = FLP_Int + 2,
	FLP_FXSine           = FLP_Int + 3,
	FLP_CutCutBy         = FLP_Int + 4,
	FLP_WindowH          = FLP_Int + 5,
	FLP_MiddleNote       = FLP_Int + 7,
	FLP_Reserved         = FLP_Int + 8,   // may contain an invalid version info
	FLP_MainResCut       = FLP_Int + 9,   // obsolete
	FLP_DelayFRes        = FLP_Int + 10,
	FLP_Reverb           = FLP_Int + 11,
	FLP_StretchTime      = FLP_Int + 12,
	FLP_SSNote           = FLP_Int + 13,  // SimSynth patch middle note (obsolete)
	FLP_FineTune         = FLP_Int + 14,
	FLP_SampleFlags      = FLP_Int + 15,
	FLP_LayerFlags       = FLP_Int + 16,
	FLP_ChanFilterNum    = FLP_Int + 17,
	FLP_CurrentFilterNum = FLP_Int + 18,
	FLP_FXOutChanNum     = FLP_Int + 19,  // FX track output channel
	FLP_NewTimeMarker    = FLP_Int + 20,  // + Time & Mode in higher bits
	FLP_FXColor          = FLP_Int + 21,
	FLP_PatColor         = FLP_Int + 22,
	FLP_PatAutoMode      = FLP_Int + 23,  // obsolete
	FLP_SongLoopPos      = FLP_Int + 24,
	FLP_AUSmpRate        = FLP_Int + 25,
	FLP_FXInChanNum      = FLP_Int + 26,  // FX track input channel
	FLP_PluginIcon       = FLP_Int + 27,
	FLP_FineTempo        = FLP_Int + 28,
	// TODO: event 157 is used in FL 12

	// Variable size (192..255)
	FLP_Undef                  = 192,           // +Length (VarLengthInt)
	FLP_Text                   = FLP_Undef,     // +Length (VarLengthInt) +Text (Null Term. AnsiString)
	FLP_Text_ChanName          = FLP_Text,      // obsolete
	FLP_Text_PatName           = FLP_Text + 1,
	FLP_Text_Title             = FLP_Text + 2,
	FLP_Text_Comment           = FLP_Text + 3,
	FLP_Text_SampleFileName    = FLP_Text + 4,
	FLP_Text_URL               = FLP_Text + 5,
	FLP_Text_CommentRTF        = FLP_Text + 6,  // comments in Rich Text format
	FLP_Version                = FLP_Text + 7,
	FLP_RegName                = FLP_Text + 8,  // since 1.3.9 the (scrambled) reg name is stored in the FLP
	FLP_Text_DefPluginName     = FLP_Text + 9,
	//FLP_Text_CommentRTF_SC     = FLP_Text + 10,  // new comments in Rich Text format (obsolete)
	FLP_Text_ProjDataPath      = FLP_Text + 10,
	FLP_Text_PluginName        = FLP_Text + 11,  // plugin's name
	FLP_Text_FXName            = FLP_Text + 12,  // FX track name
	FLP_Text_TimeMarker        = FLP_Text + 13,  // time marker name
	FLP_Text_Genre             = FLP_Text + 14,
	FLP_Text_Author            = FLP_Text + 15,
	FLP_MIDICtrls              = FLP_Text + 16,
	FLP_Delay                  = FLP_Text + 17,
	FLP_TS404Params            = FLP_Text + 18,
	FLP_DelayLine              = FLP_Text + 19,  // obsolete
	FLP_NewPlugin              = FLP_Text + 20,  // new VST or DirectX plugin
	FLP_PluginParams           = FLP_Text + 21,
	FLP_Reserved2              = FLP_Text + 22,  // used once for testing
	FLP_ChanParams             = FLP_Text + 23,  // block of various channel params (can grow)
	FLP_CtrlRecChan            = FLP_Text + 24,  // automated controller events
	FLP_PLSel                  = FLP_Text + 25,  // selection in playlist
	FLP_Envelope               = FLP_Text + 26,
	FLP_ChanLevels             = FLP_Text + 27,  // pan, vol, pitch, filter, filter type
	FLP_ChanFilter             = FLP_Text + 28,  // cut, res, type (obsolete)
	FLP_ChanPoly               = FLP_Text + 29,  // max poly, poly slide, monophonic
	FLP_NoteRecChan            = FLP_Text + 30,  // automated note events
	FLP_PatCtrlRecChan         = FLP_Text + 31,  // automated ctrl events per pattern
	FLP_PatNoteRecChan         = FLP_Text + 32,  // automated note events per pattern
	FLP_InitCtrlRecChan        = FLP_Text + 33,  // init values for automated events
	FLP_RemoteCtrl_MIDI        = FLP_Text + 34,  // remote control entry (MIDI)
	FLP_RemoteCtrl_Int         = FLP_Text + 35,  // remote control entry (internal)
	FLP_Tracking               = FLP_Text + 36,  // vol/kb tracking
	FLP_ChanOfsLevels          = FLP_Text + 37,  // levels offset
	FLP_Text_RemoteCtrlFormula = FLP_Text + 38,  // remote control entry formula
	FLP_Text_ChanFilter        = FLP_Text + 39,
	FLP_RegBlackList           = FLP_Text + 40,  // black list of reg codes
	FLP_PLRecChan              = FLP_Text + 41,  // playlist
	FLP_ChanAC                 = FLP_Text + 42,  // channel articulator
	FLP_FXRouting              = FLP_Text + 43,
	FLP_FXParams               = FLP_Text + 44,
	FLP_ProjectTime            = FLP_Text + 45,
	FLP_PLTrackInfo            = FLP_Text + 46,
	FLP_Text_PLTrackName       = FLP_Text + 47,
};

char const* header_format_name(FLPFormat hf) noexcept;
char const* flp_event_name(std::underlying_type_t<FLPEventType> event_id) noexcept;

inline char const* flp_event_name(FLPEventType event_type) noexcept {
	return flp_event_name(static_cast<std::underlying_type_t<FLPEventType>>(event_type));
}
