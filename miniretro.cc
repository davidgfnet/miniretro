
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#include <iostream>
#include "argparse.hpp"
#include "libretro.h"
#include "util.h"
#include "loader.h"

typedef RETRO_CALLCONV void (*core_info_function)(struct retro_system_info *info);
typedef RETRO_CALLCONV void (*core_action_function)(void);
typedef RETRO_CALLCONV void (*core_loadg_function)(struct retro_game_info *game);

typedef RETRO_CALLCONV void (*core_set_environment_fn)(retro_environment_t);
typedef RETRO_CALLCONV void (*core_set_video_refresh_fn)(retro_video_refresh_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_fn)(retro_audio_sample_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_batch_fn)(retro_audio_sample_batch_t);
typedef RETRO_CALLCONV void (*core_set_input_poll_fn)(retro_input_poll_t);
typedef RETRO_CALLCONV void (*core_set_input_state_fn)(retro_input_state_t);

std::string systemdir;
std::string outputdir = ".";
unsigned frame_counter = 0;
unsigned dump_every = 0;
enum retro_pixel_format videofmt = RETRO_PIXEL_FORMAT_0RGB1555;

bool RETRO_CALLCONV env_callback(unsigned cmd, void *data) {
	switch (cmd) {
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
		videofmt = *(enum retro_pixel_format*)data;
		return true;
	case RETRO_ENVIRONMENT_GET_VARIABLE:
		return false;
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
		*((const char**)data) = systemdir.c_str();
		return true;
	case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
		return true;
	case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
		return false;
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
		return false;   // TODO capture logs
	default:
		return false;
	}
}

void RETRO_CALLCONV video_update(const void *data, unsigned width, unsigned height, size_t pitch) {
	frame_counter++;
	if (dump_every && (frame_counter % dump_every) == 0) {
		char filename[PATH_MAX];
		sprintf(filename, "%s/screenshot%06u.png", outputdir.c_str(), frame_counter);
		dump_image(data, width, height, pitch, videofmt, filename);
	}
}

void RETRO_CALLCONV input_poll() {
	// Do nothing for now
}

void RETRO_CALLCONV single_sample(int16_t left, int16_t right) {
	// TODO: output audio
}

size_t RETRO_CALLCONV audio_buffer(const int16_t *data, size_t frames) {
	return frames;   // TODO: output audio
}


int16_t RETRO_CALLCONV input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	// No input for now
	return 0;
}

int main(int argc, char **argv) {
	argparse::ArgumentParser parser;

	#ifndef STATIC_CORE
	parser.addArgument("-c", "--core", 1, false);
	#endif
	parser.addArgument("-r", "--rom", 1, false);
	parser.addArgument("-o", "--output", 1);
	parser.addArgument("-s", "--system", 1);

	// Max number of frames to run
	parser.addArgument("-f", "--frames", 1);

	// Dumps the specific frames (ie. 10 20)
	parser.addArgument("--dump-frames", '*');
	// Dumps a frame every N frames
	parser.addArgument("--dump-frames-every", 1);

	// TODO: dump other stuff
	// TODO: simulate input

	parser.parse(argc, (const char **)argv);

	// Read the args
	std::string corefile;
	std::string rom_file = parser.retrieve<std::string>("r");
	#ifndef STATIC_CORE
	corefile = parser.retrieve<std::string>("c");
	#endif
	if (parser.exists("output"))
		outputdir = parser.retrieve<std::string>("output");
	if (parser.exists("dump-frames"))
		dump_every = parser.retrieve<unsigned>("dump-frames-every");
	unsigned maxframes = 300;
	if (parser.exists("frames"))
		maxframes = parser.retrieve<unsigned>("frames");
	if (parser.exists("system"))
		systemdir = parser.retrieve<std::string>("system");

	core_functions_t *retrofns = load_core(corefile.c_str());
	if (!retrofns) {
		std::cerr << "Could not load " << corefile << std::endl;
		return 1;
	}

	struct retro_system_info info;
	retrofns->core_get_info(&info);
	std::cout << "Loaded core " << info.library_name << " version " << info.library_version << std::endl;

	// Set the required callbacks
	retrofns->core_set_env_function(&env_callback);
	retrofns->core_set_video_refresh_function(&video_update);
	retrofns->core_set_audio_sample_function(&single_sample);
	retrofns->core_set_audio_sample_batch_function(&audio_buffer);
	retrofns->core_set_input_poll_function(&input_poll);
	retrofns->core_set_input_state_function(&input_state);

	// Call init now
	retrofns->core_init();

	// Init the core and load the ROM
	std::cout << "Loading ROM " << rom_file << std::endl;
	struct retro_game_info gameinfo = {.path = rom_file.c_str(), .data = NULL, .size = 0, .meta = NULL};
	retrofns->core_load_game(&gameinfo);

	for (unsigned i = 0; i < maxframes; i++) {
		retrofns->core_run();
	}

	retrofns->core_deinit();
	free(retrofns);
}


