
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
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
std::string outputdir = ".";
unsigned frame_counter = 0;
unsigned dump_every = 0;
unsigned save_dump_every = 0;
enum retro_pixel_format videofmt = RETRO_PIXEL_FORMAT_0RGB1555;
struct retro_system_av_info avinfo;
pid_t ffpidv = 0;
int ffpipev[2] = {0};
pid_t ffpida = 0;
int ffpipea[2] = {0};

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
	frame_counter++;
	if (!data)
		return;

	if (dump_every && (frame_counter % dump_every) == 0) {
		char filename[PATH_MAX];
		sprintf(filename, "%s/screenshot%06u.png", outputdir.c_str(), frame_counter);
		dump_image(data, width, height, pitch, videofmt, filename);
	}
	if (ffpidv)
		dump_image(data, width, height, pitch, videofmt, ffpipev[1]);
}

void RETRO_CALLCONV input_poll() {
	// Do nothing for now
}

void RETRO_CALLCONV single_sample(int16_t left, int16_t right) {
	if (ffpida) {
		int16_t buf[2] = {left, right};
		write(ffpipea[1], buf, sizeof(buf));
	}
}

size_t RETRO_CALLCONV audio_buffer(const int16_t *data, size_t frames) {
	if (ffpida)
		write(ffpipea[1], data, frames*2*sizeof(int16_t));
	return frames;
}

int16_t RETRO_CALLCONV input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
	if (icmds.count(frame_counter) && (icmds.at(frame_counter) & (1 << id)))
		return 1;
	// No input for now
	return 0;
}

void alarmhandler(int signal) {
	std::cerr << "Alarm triggered" << std::endl;
	exit(-1);
}

int main(int argc, char **argv) {
	// Set up alarm handler to ensure we can abort
	signal(SIGALRM, alarmhandler);

	argparse::ArgumentParser parser;

	#ifndef STATIC_CORE
	parser.addArgument("-c", "--core", 1, false);
	#endif
	parser.addArgument("-r", "--rom", 1, false);
	parser.addArgument("-o", "--output", 1, false);
	parser.addArgument("-s", "--system", 1, false);

	// Disables alarm (for debugging)
	parser.addArgument("--no-alarm", '*');

	// Max number of frames to run
	parser.addArgument("-f", "--frames", 1);

	// Frame timeout (if the core gets stuck for too long)
	parser.addArgument("-t", "--timeout", 1);

	// Dumps the specific frames (ie. 10 20)
	parser.addArgument("--dump-frames", '*');
	// Dumps a frame every N frames
	parser.addArgument("--dump-frames-every", 1);
	// Generates a video/audio from the video/audio streams
	parser.addArgument("--dump-video", 1);
	parser.addArgument("--dump-audio", 1);

	// Dumps state saves every N frames
	parser.addArgument("--dump-savestates-every", 1);

	// Read input commands
	parser.addArgument("-i", "--input", 1);

	// TODO: dump other stuff

	parser.parse(argc, (const char **)argv);

	// Read the args
	std::string corefile;
	std::string rom_file = parser.retrieve<std::string>("r");
	#ifndef STATIC_CORE
	corefile = parser.retrieve<std::string>("c");
	#endif
	if (parser.gotArgument("output"))
		outputdir = parser.retrieve<std::string>("output");
	if (parser.gotArgument("dump-frames-every"))
		dump_every = parser.retrieve<unsigned>("dump-frames-every");
	if (parser.gotArgument("dump-savestates-every"))
		save_dump_every = parser.retrieve<unsigned>("dump-savestates-every");
	unsigned maxframes = 300;
	if (parser.gotArgument("frames"))
		maxframes = parser.retrieve<unsigned>("frames");
	unsigned frametimeout = 5;
	if (parser.gotArgument("timeout"))
		frametimeout = parser.retrieve<unsigned>("timeout");
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
	bool use_alarm = !parser.gotArgument("no-alarm");

	core_functions_t *retrofns = load_core(corefile.c_str());
	if (!retrofns) {
		std::cerr << "Could not load " << corefile << std::endl;
		return 1;
	}

	struct retro_system_info info;
	retrofns->core_get_info(&info);
	std::cout << "Loaded core " << info.library_name << " version " << info.library_version << std::endl;
	std::cout << "Core needs fullpath " << info.need_fullpath << std::endl;
	std::cout << "Running for " << maxframes << " frames with frame timeout of ";
	std::cout << frametimeout << " seconds" << std::endl;

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
	void *dptr = NULL;
	if (!info.need_fullpath) {
		FILE *fd = fopen(rom_file.c_str(), "rb");
		fseek(fd, 0, SEEK_END);
		gameinfo.size = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		dptr = malloc(gameinfo.size);
		fread(dptr, 1, gameinfo.size, fd);
		fclose(fd);
	}
	gameinfo.data = dptr;

	if (!retrofns->core_load_game(&gameinfo)) {
		std::cout << "Failed to load the game, retro_load_game returned false!" << std::endl;
		return -1;
	}
	retrofns->core_reset();
	retrofns->core_get_system_av_info(&avinfo);

	if (parser.gotArgument("dump-video")) {
		std::string videop = parser.retrieve<std::string>("dump-video");
		pipe(ffpipev);
		ffpidv = fork();
		if (ffpidv) {
			close(ffpipev[0]);
		}
		else {
			close(ffpipev[1]);
			dup2(ffpipev[0], 0);

			execlp("ffmpeg", "ffmpeg",
				"-f", "image2pipe",
				"-framerate", std::to_string(avinfo.timing.fps).c_str(),
				"-i", "-", "-vf", "format=yuv444p",
				"-c:v", "libx264",
				videop.c_str(), NULL);
		}
	}

	if (parser.gotArgument("dump-audio")) {
		std::string audiop = parser.retrieve<std::string>("dump-audio");
		pipe(ffpipea);
		ffpida = fork();
		if (ffpida) {
			close(ffpipea[0]);
		}
		else {
			close(ffpipea[1]);
			dup2(ffpipea[0], 0);

			execlp("ffmpeg", "ffmpeg",
				"-f", "s16le", "-ac", "2",
				"-ar", std::to_string(avinfo.timing.sample_rate).c_str(),
				"-i", "-",
				"-ar", "44.1k",
				"-c:a", "libvorbis",
				audiop.c_str(), NULL);
		}
	}

	for (unsigned i = 0; i < maxframes; i++) {
		if (use_alarm)
			alarm(frametimeout);
		retrofns->core_run();

		if (save_dump_every && (frame_counter % save_dump_every) == 0) {
			char filename[PATH_MAX];
			sprintf(filename, "%s/state%06u.bin", outputdir.c_str(), frame_counter);
			size_t sersz = retrofns->core_serialize_size();
			void *serstate = malloc(sersz);
			retrofns->core_serialize(serstate, sersz);
			FILE *fd = fopen(filename, "wb");
			if (fd) {
				fwrite(serstate, 1, sersz, fd);
				fclose(fd);
			}
			free(serstate);
		}
	}

	alarm(0);
	retrofns->core_deinit();
	if (dptr)
		free(dptr);
	free(retrofns);

	if (ffpida) {
		close(ffpipea[1]);
		waitpid(ffpida, NULL, 0);
	}
	if (ffpidv) {
		close(ffpipev[1]);
		waitpid(ffpidv, NULL, 0);
	}
}


