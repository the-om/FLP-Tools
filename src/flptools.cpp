﻿#include <cstdio>
#include <filesystem> // path
#include <chrono>     // high_resolution_clock

#include "argparse.h"
#include "flp_stream.h"
#include "version.h"
#include "json.h"
#include "cfile.h"


enum class Mode {
	not_set,
	flp_to_json,
	json_to_flp
};

struct ProgramArgs {
	std::filesystem::path input_path {};
	std::filesystem::path output_path {};
	Mode mode = Mode::not_set;
};

std::function<void(wchar_t const*)> write_path_arg(std::filesystem::path& p) {
	return [&p] (wchar_t const* arg) {
		if(arg)
			p = arg;
		else
			throw std::runtime_error("missing argument");
	};
}

struct CFileInStream : public Om::CFile {
	CFileInStream(FILE* fptr) : Om::CFile(fptr) {}

	static char const* errmsg(int er) {
		return { std::strerror(er) };
	}
};

bool flp_to_json(ProgramArgs const& program_args) {
	FILE* f = _wfopen(program_args.input_path.c_str(), L"rb");
	if(f == nullptr) {
		std::fputs("Could not open input file! - Exiting\n", stderr);
		return false;
	}
	FLPInStream<CFileInStream> flp(f);

	Om::CFile outfile(_wfopen(program_args.output_path.c_str(), L"wb"));
	if(!outfile.is_open()) {
		std::fputs("Could not open output file! - Exiting\n", stderr);
		return false;
	}
	Om::JSONOutStream<Om::CFile> json_stream(outfile);

	flp.read_header();

	json_stream.begin_object();
	json_stream.key("header");
	stream_flp_header(json_stream, flp.file_header());
	json_stream.key("events");
	json_stream.begin_array();
	bool is_unicode = false;
	while(flp.has_event()) {
		FLPEvent const& event = *flp;
		stream_flp_event<false>(json_stream, event);
		if(event.type == FLPEventType::FLP_Version) {
			Version version(reinterpret_cast<char const*>(event.text_data.get()));
			if(version >= "12.0.0") {
				is_unicode = true;
			}
			++flp;
			break;
		}
		++flp;
	}
	if(is_unicode)
		for(; flp.has_event(); ++flp)
			stream_flp_event<true>(json_stream, *flp);
	else
		for(; flp.has_event(); ++flp)
			stream_flp_event<false>(json_stream, *flp);

	json_stream.end_array();
	json_stream.end_object();
	json_stream.flush();

	return true;
}

int wmain(int argc, wchar_t* argv[]) {
	std::ios_base::sync_with_stdio(false);

	ProgramArgs program_args {};
	Om::ArgHandlerMap<wchar_t> const arg_handlers = {
		{L"o", write_path_arg(program_args.output_path)},
		{L"",  write_path_arg(program_args.input_path) }
	};

	Om::parse_args<wchar_t>(argc, argv, arg_handlers);
	if(program_args.mode == Mode::not_set) {
		auto input_ext = program_args.input_path.extension();
		if(input_ext == L"json") {
			program_args.mode = Mode::json_to_flp;
		} else {
			program_args.mode = Mode::flp_to_json;
		}
	}

	if(program_args.output_path.empty()) {
		if(program_args.mode == Mode::flp_to_json) {
			program_args.output_path = program_args.input_path;
			program_args.output_path.replace_filename(
				program_args.input_path.filename().wstring() + L".json"
			);
		}
	}

	std::fprintf(stderr, "Input file: %ls\n", program_args.input_path.c_str());
	std::fprintf(stderr, "Output file: %ls\n", program_args.output_path.c_str());

	using namespace std::chrono;
	using clock = high_resolution_clock;

	auto begin_time = clock::now();

	if(!flp_to_json(program_args))
		return EXIT_FAILURE;

	auto end_time = clock::now();
	std::fprintf(stderr, "elapsed time: %lldus\n", duration_cast<microseconds>(end_time - begin_time).count());

	return EXIT_SUCCESS;
}
