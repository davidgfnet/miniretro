
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#include <cstdlib>
#include "loader.h"

#ifndef STATIC_CORE

core_functions_t *load_core(const char *filename) {
	LIBHANDLE libhandle = LOAD_LIBRARY(filename);
	if (!libhandle)
		return NULL;

	core_functions_t *fns = (core_functions_t*)malloc(sizeof(core_functions_t));
	
	fns->core_init = (core_action_fnt)LOAD_SYMBOL(libhandle, "retro_init");
	fns->core_deinit = (core_action_fnt)LOAD_SYMBOL(libhandle, "retro_deinit");
	fns->core_run = (core_action_fnt)LOAD_SYMBOL(libhandle, "retro_run");
	fns->core_reset = (core_action_fnt)LOAD_SYMBOL(libhandle, "retro_reset");
	fns->core_get_info = (core_info_fnt)LOAD_SYMBOL(libhandle, "retro_get_system_info");
	fns->core_load_game = (core_loadg_fnt)LOAD_SYMBOL(libhandle, "retro_load_game");
	fns->core_unload_game = (core_unloadg_fnt)LOAD_SYMBOL(libhandle, "retro_unload_game");

	fns->core_set_env_function = (core_set_environment_fnt)LOAD_SYMBOL(libhandle, "retro_set_environment");
	fns->core_set_video_refresh_function = (core_set_video_refresh_fnt)LOAD_SYMBOL(libhandle, "retro_set_video_refresh");
	fns->core_set_audio_sample_function = (core_set_audio_sample_fnt)LOAD_SYMBOL(libhandle, "retro_set_audio_sample");
	fns->core_set_audio_sample_batch_function = (core_set_audio_sample_batch_fnt)LOAD_SYMBOL(libhandle, "retro_set_audio_sample_batch");
	fns->core_set_input_poll_function = (core_set_input_poll_fnt)LOAD_SYMBOL(libhandle, "retro_set_input_poll");
	fns->core_set_input_state_function = (core_set_input_state_fnt)LOAD_SYMBOL(libhandle, "retro_set_input_state");

	fns->core_serialize = (core_serialize_fnt)LOAD_SYMBOL(libhandle, "retro_serialize");
	fns->core_serialize_size = (core_serialize_size_fnt)LOAD_SYMBOL(libhandle, "retro_serialize_size");
	fns->core_unserialize = (core_unserialize_fnt)LOAD_SYMBOL(libhandle, "retro_unserialize");
	fns->core_get_system_av_info = (core_get_system_av_info_fnt)LOAD_SYMBOL(libhandle, "retro_get_system_av_info");
	fns->handle = libhandle;

	return fns;
}

void unload_core(core_functions_t *core) {
	UNLOAD_LIBRARY(core->handle);
	free(core);
}

#else

// This is to be used when a core can only be built by statically linking it (no -shared).
// At this moment I'm only aware of a single core with such issue, but I'm adding support
// nevertheless, since we have a handful of platforms where static cores are used and it's
// likely that we could encounter bugs due to it (thinking on dynarecs).

core_functions_t *load_core(const char *filename) {
	core_functions_t *fns = (core_functions_t*)malloc(sizeof(core_functions_t));

	fns->core_init = &retro_init;
	fns->core_deinit = &retro_deinit;
	fns->core_run = &retro_run;
	fns->core_reset = &retro_reset;
	fns->core_get_info = &retro_get_system_info;
	fns->core_load_game = &retro_load_game;
	fns->core_unload_game = &retro_unload_game;

	fns->core_set_env_function = &retro_set_environment;
	fns->core_set_video_refresh_function = &retro_set_video_refresh;
	fns->core_set_audio_sample_function = &retro_set_audio_sample;
	fns->core_set_audio_sample_batch_function = &retro_set_audio_sample_batch;
	fns->core_set_input_poll_function = &retro_set_input_poll;
	fns->core_set_input_state_function = &retro_set_input_state;

	fns->core_serialize = &retro_serialize;
	fns->core_serialize_size = &retro_serialize_size;
	fns->core_unserialize = &retro_unserialize;
	fns->core_get_system_av_info = &retro_get_system_av_info;

	return fns;
}

void unload_core(core_functions_t *core) {
	free(core);
}

#endif


