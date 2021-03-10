
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#ifndef _LOADER_HH_
#define _LOADER_HH_

#include "libretro.h"

// Function types
typedef RETRO_CALLCONV void (*core_info_fnt)(struct retro_system_info *info);
typedef RETRO_CALLCONV void (*core_action_fnt)(void);
typedef RETRO_CALLCONV bool (*core_loadg_fnt)(const struct retro_game_info *game);
typedef RETRO_CALLCONV void (*core_set_environment_fnt)(retro_environment_t);
typedef RETRO_CALLCONV void (*core_set_video_refresh_fnt)(retro_video_refresh_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_fnt)(retro_audio_sample_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_batch_fnt)(retro_audio_sample_batch_t);
typedef RETRO_CALLCONV void (*core_set_input_poll_fnt)(retro_input_poll_t);
typedef RETRO_CALLCONV void (*core_set_input_state_fnt)(retro_input_state_t);

typedef struct {
	core_action_fnt core_init;
	core_action_fnt core_deinit;
	core_action_fnt core_run;
	core_info_fnt   core_get_info;
	core_loadg_fnt  core_load_game;
	core_set_environment_fnt core_set_env_function;
	core_set_video_refresh_fnt core_set_video_refresh_function;
	core_set_audio_sample_fnt core_set_audio_sample_function;
	core_set_audio_sample_batch_fnt core_set_audio_sample_batch_function;
	core_set_input_poll_fnt core_set_input_poll_function;
	core_set_input_state_fnt core_set_input_state_function;

	void *handle;
} core_functions_t;

// Loads the core and gets a handler to its functions
core_functions_t *load_core(const char *filename);

// Unloads and frees the core
void unload_core(core_functions_t *core);

#endif

