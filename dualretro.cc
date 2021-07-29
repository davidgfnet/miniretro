
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license
// Runs two cores alongside and compares them (using their
// serialized state).

#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
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

const std::unordered_map<std::string, unsigned> buttons = {
	{"start",  RETRO_DEVICE_ID_JOYPAD_START},
	{"select", RETRO_DEVICE_ID_JOYPAD_SELECT},
	{"a",      RETRO_DEVICE_ID_JOYPAD_A},
	{"b",      RETRO_DEVICE_ID_JOYPAD_B},
	{"x",      RETRO_DEVICE_ID_JOYPAD_X},
	{"y",      RETRO_DEVICE_ID_JOYPAD_Y},
	{"up",     RETRO_DEVICE_ID_JOYPAD_UP},
	{"down",   RETRO_DEVICE_ID_JOYPAD_DOWN},
	{"left",   RETRO_DEVICE_ID_JOYPAD_LEFT},
	{"right",  RETRO_DEVICE_ID_JOYPAD_RIGHT},
};
std::unordered_map<unsigned, unsigned> icmds;
std::string systemdir;
unsigned curr_core = 0;
unsigned frame_counter[2] = {0, 0};
enum retro_pixel_format videofmt = RETRO_PIXEL_FORMAT_0RGB1555;

void RETRO_CALLCONV logging_callback(enum retro_log_level level, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
    va_end(args);
}

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
		((struct retro_log_callback*)data)->log = &logging_callback;
		return true;
	default:
		return false;
	}
}

void RETRO_CALLCONV video_update(const void *data, unsigned width, unsigned height, size_t pitch) {
	frame_counter[curr_core]++;
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
	if (icmds.count(frame_counter[curr_core]) && (icmds.at(frame_counter[curr_core]) & (1 << id)))
		return 1;
	// No input for now
	return 0;
}

int main(int argc, char **argv) {
	argparse::ArgumentParser parser;

	parser.addArgument("-c", "--core1", 1, false);
	parser.addArgument("-d", "--core2", 1, false);
	parser.addArgument("-r", "--rom", 1, false);
	parser.addArgument("-s", "--system", 1, false);

	// Max number of frames to run
	parser.addArgument("-f", "--frames", 1);

	// Read input commands
	parser.addArgument("-i", "--input", 1);

	parser.parse(argc, (const char **)argv);

	// Read the args
	std::string corefile1, corefile2;
	std::string rom_file = parser.retrieve<std::string>("r");
	corefile1 = parser.retrieve<std::string>("c");
	corefile2 = parser.retrieve<std::string>("d");
	unsigned maxframes = 300;
	if (parser.gotArgument("frames"))
		maxframes = parser.retrieve<unsigned>("frames");
	if (parser.gotArgument("system"))
		systemdir = parser.retrieve<std::string>("system");
	if (parser.gotArgument("input")) {
		std::istringstream spr(parser.retrieve<std::string>("input"));
		std::string entry;
		while (spr >> entry) {
			auto p = entry.find(':');
			if (p != std::string::npos) {
				std::string b = entry.substr(p+1);
				if (buttons.count(b))
					icmds[atoi(entry.c_str())] |= (1 << buttons.at(b));
			}
		}
	}

	core_functions_t *retrofns[2];
	retrofns[0] = load_core(corefile1.c_str());
	retrofns[1] = load_core(corefile2.c_str());
	if (!retrofns[0] || !retrofns[1]) {
		std::cerr << "Could not load cores" << std::endl;
		return 1;
	}

	// Set the required callbacks
	for (int i = 0; i < 2; i++) {
		retrofns[i]->core_set_env_function(&env_callback);
		retrofns[i]->core_set_video_refresh_function(&video_update);
		retrofns[i]->core_set_audio_sample_function(&single_sample);
		retrofns[i]->core_set_audio_sample_batch_function(&audio_buffer);
		retrofns[i]->core_set_input_poll_function(&input_poll);
		retrofns[i]->core_set_input_state_function(&input_state);

		// Call init now
		retrofns[i]->core_init();
	}

	// Init the core and load the ROM
	std::cout << "Loading ROM " << rom_file << std::endl;
	struct retro_game_info gameinfo = {.path = rom_file.c_str(), .data = NULL, .size = 0, .meta = NULL};
	void *dptr = NULL;
	{
		FILE *fd = fopen(rom_file.c_str(), "rb");
		fseek(fd, 0, SEEK_END);
		gameinfo.size = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		dptr = malloc(gameinfo.size);
		fread(dptr, 1, gameinfo.size, fd);
		fclose(fd);
	}
	gameinfo.data = dptr;

	for (int i = 0; i < 2; i++) {
		if (!retrofns[i]->core_load_game(&gameinfo)) {
			std::cout << "Failed to load the game, retro_load_game returned false!" << std::endl;
			return -1;
		}
		retrofns[i]->core_reset();
	}

	void *serstate[2];
	size_t sersz = retrofns[0]->core_serialize_size();
	for (int j = 0; j < 2; j++)
		serstate[j] = malloc(sersz);

	for (unsigned i = 0; i < maxframes; i++) {
		for (int j = 0; j < 2; j++) {
			curr_core = j;
			retrofns[j]->core_run();
			retrofns[j]->core_serialize(serstate[j], sersz);
		}

		// Compare states
		if (memcmp(serstate[0], serstate[1], sersz)) {
			printf("Mismatch in frame %d\n", i);
			break;
		}
	}

	for (int j = 0; j < 2; j++) {
		retrofns[j]->core_deinit();
		free(retrofns[j]);
	}
	if (dptr)
		free(dptr);
}


