#include <iostream>   // cerr, cout
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

struct {
	std::filesystem::path input_path {};
	std::filesystem::path output_path {};
	Mode mode = Mode::not_set;
} program_args;

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

	static std::string errmsg(int er) {
		return { std::strerror(er) };
	}
};

int wmain(int argc, wchar_t* argv[]) {
	std::cout << "FLP-Show\n\n";
	std::ios_base::sync_with_stdio(false);

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

	FILE* f = _wfopen(program_args.input_path.c_str(), L"rb");
	if(f == nullptr) {
		std::cerr << "Could not open input file! - Exiting\n";
		return EXIT_FAILURE;
	}
	FLPInStream<CFileInStream> flp(f);

	flp.read_header();

	if(program_args.output_path.empty()) {
		if(program_args.mode == Mode::flp_to_json) {
			program_args.output_path = program_args.input_path;
			program_args.output_path.replace_filename(program_args.input_path.filename().wstring() + L".json");
		}
	}
	Om::CFile outfile(_wfopen(program_args.output_path.c_str(), L"wb"));
	if(!outfile.is_open()) {
		std::cerr << "Could not open output file! - Exiting\n";
		return EXIT_FAILURE;
	}

	using namespace std::chrono;
	using clock = high_resolution_clock;

	auto begin_time = clock::now();

	Om::JSONOutStream<Om::CFile> json_stream(outfile);
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
	if(is_unicode) {
		for(; flp.has_event(); ++flp)
			stream_flp_event<true>(json_stream, *flp);
	} else {
		for(; flp.has_event(); ++flp)
			stream_flp_event<false>(json_stream, *flp);
	}
	json_stream.end_array();
	json_stream.end_object();
	json_stream.flush();

	auto end_time = clock::now();
	std::cout << "elapsed time: " << duration_cast<microseconds>(end_time - begin_time).count() << "us\n";

	return EXIT_SUCCESS;
}
