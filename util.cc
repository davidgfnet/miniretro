
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#include <cstdlib>
#include "util.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
	uint8_t r, g, b;
} pixel_t;

void *image_convert(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt) {
	pixel_t *buffer = (pixel_t*)malloc(width * height * 3);
	uint8_t *inbytes = (uint8_t*)data;
	if (fmt == RETRO_PIXEL_FORMAT_XRGB8888) {
		for (unsigned row = 0; row < height; row++) {
			uint32_t *inbuf = (uint32_t*)&inbytes[row * pitch];
			for (unsigned col = 0; col < width; col++) {
				buffer[row * width + col].r = inbuf[col] >> 16;
				buffer[row * width + col].g = inbuf[col] >>  8;
				buffer[row * width + col].b = inbuf[col];
			}
		}
	}
	else if (fmt == RETRO_PIXEL_FORMAT_RGB565) {
		for (unsigned row = 0; row < height; row++) {
			uint16_t *inbuf = (uint16_t*)&inbytes[row * pitch];
			for (unsigned col = 0; col < width; col++) {
				buffer[row * width + col].r = ((inbuf[col] >> 11) & 0x1F) << 3;
				buffer[row * width + col].g = ((inbuf[col] >>  5) & 0x3F) << 2;
				buffer[row * width + col].b = ((inbuf[col] & 0x1F) << 3);
			}
		}
	}
	else {
		for (unsigned row = 0; row < height; row++) {
			uint16_t *inbuf = (uint16_t*)&inbytes[row * pitch];
			for (unsigned col = 0; col < width; col++) {
				buffer[row * width + col].r = ((inbuf[col] >> 10) & 0x1F) << 3;
				buffer[row * width + col].g = ((inbuf[col] >>  5) & 0x1F) << 3;
				buffer[row * width + col].b = ((inbuf[col] & 0x1F) << 3);
			}
		}
	}
	return buffer;
}

void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, const char *filename) {
	void *convimg = image_convert(data, width, height, pitch, fmt);
	stbi_write_png(filename, width, height, 3, convimg, 3 * width);
	free(convimg);
}

