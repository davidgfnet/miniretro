
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#ifndef _LOADER_HH_
#define _LOADER_HH_

#include "libretro.h"

#ifdef WIN32
  #include <windows.h>
  #define LIBHANDLE HMODULE
  #define LOAD_LIBRARY(path) LoadLibraryA(path)
  #define UNLOAD_LIBRARY(lib) FreeLibrary(lib)
  #define LOAD_SYMBOL(lib, name) GetProcAddress(lib, name)
#else
  #include <dlfcn.h>
  #define LIBHANDLE void*
  #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY | RTLD_LOCAL)
  #define UNLOAD_LIBRARY(lib) dlclose(lib)
  #define LOAD_SYMBOL(lib, name) dlsym(lib, name)
#endif

// Function types
typedef RETRO_CALLCONV void (*core_info_fnt)(struct retro_system_info *info);
typedef RETRO_CALLCONV void (*core_action_fnt)(void);
typedef RETRO_CALLCONV bool (*core_loadg_fnt)(const struct retro_game_info *game);
typedef RETRO_CALLCONV void (*core_unloadg_fnt)(void);
typedef RETRO_CALLCONV void (*core_set_environment_fnt)(retro_environment_t);
typedef RETRO_CALLCONV void (*core_set_video_refresh_fnt)(retro_video_refresh_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_fnt)(retro_audio_sample_t);
typedef RETRO_CALLCONV void (*core_set_audio_sample_batch_fnt)(retro_audio_sample_batch_t);
typedef RETRO_CALLCONV void (*core_set_input_poll_fnt)(retro_input_poll_t);
typedef RETRO_CALLCONV void (*core_set_input_state_fnt)(retro_input_state_t);
typedef RETRO_CALLCONV bool (*core_serialize_fnt)(void *data, size_t size);
typedef RETRO_CALLCONV size_t (*core_serialize_size_fnt)(void);
typedef RETRO_CALLCONV bool (*core_unserialize_fnt)(const void *data, size_t size);
typedef RETRO_CALLCONV void (*core_get_system_av_info_fnt)(struct retro_system_av_info *info);

typedef struct {
	core_action_fnt core_init;
	core_action_fnt core_deinit;
	core_action_fnt core_run;
	core_action_fnt core_reset;
	core_info_fnt   core_get_info;
	core_loadg_fnt  core_load_game;
	core_unloadg_fnt core_unload_game;
	core_set_environment_fnt core_set_env_function;
	core_set_video_refresh_fnt core_set_video_refresh_function;
	core_set_audio_sample_fnt core_set_audio_sample_function;
	core_set_audio_sample_batch_fnt core_set_audio_sample_batch_function;
	core_set_input_poll_fnt core_set_input_poll_function;
	core_set_input_state_fnt core_set_input_state_function;
	core_serialize_fnt core_serialize;
	core_serialize_size_fnt core_serialize_size;
	core_unserialize_fnt core_unserialize;
	core_get_system_av_info_fnt core_get_system_av_info;

	LIBHANDLE handle;
} core_functions_t;

// Loads the core and gets a handler to its functions
core_functions_t *load_core(const char *filename);

// Unloads and frees the core
void unload_core(core_functions_t *core);

#endif

