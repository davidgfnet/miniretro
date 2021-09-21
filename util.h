
#ifndef _UTIL_H__
#define _UTIL_H__

#include <stdint.h>
#include "libretro.h"

void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, unsigned factor, const char *filename);
void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, unsigned factor, int fd);

template <unsigned int factor>
static void *image_scale(const void *data, unsigned width, unsigned height) {
	if (factor < 2)
		return data;

	uint8_t *pixels = (uint8_t*)data;
	void *ret = malloc(width * height * 3 * factor * factor);
	uint8_t *out = (uint8_t*)ret;
	for (unsigned row = 0, orow = 0; row < height; row++) {
		for (unsigned j = 0; j < factor; j++, orow++) {
			for (unsigned col = 0, ocol = 0; col < width; col++) {
				for (unsigned i = 0; i < factor; i++, ocol++) {
					out[(orow * factor * width + ocol) * 3 + 0] = pixels[(row * width + col) * 3 + 0];
					out[(orow * factor * width + ocol) * 3 + 1] = pixels[(row * width + col) * 3 + 1];
					out[(orow * factor * width + ocol) * 3 + 2] = pixels[(row * width + col) * 3 + 2];
				}
			}
		}
	}
	return ret;
}

#endif

