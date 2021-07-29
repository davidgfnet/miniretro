
#ifndef _UTIL_H__
#define _UTIL_H__

#include <stdint.h>
#include "libretro.h"

void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, const char *filename);
void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, int fd);

#endif

