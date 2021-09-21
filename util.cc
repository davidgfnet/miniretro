
// Copyright 2021 David Guillen Fandos <david@davidgf.net>
// Released under the GPL2 license

#include <cstdlib>
#include <unistd.h>
#include "util.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct {
	uint8_t r, g, b;
} pixel_t;

typedef void* (*img_conv)(const void *data, unsigned width, unsigned height);

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

img_conv scalers[] = {
	NULL,
	image_scale<1>, image_scale<2>, image_scale<3>, image_scale<4>,
	image_scale<5>, image_scale<6>, image_scale<7>, image_scale<8>,
};

void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, unsigned factor, const char *filename) {
	void *convimg = image_convert(data, width, height, pitch, fmt);
	void *scalimg = scalers[factor](convimg, width, height);
	stbi_write_png(filename, width * factor, height * factor, 3, scalimg, 3 * width * factor);
	free(convimg);
	if (scalimg != convimg)
		free(scalimg);
}

static void cb_write(void *context, void *data, int size) {
	int fd = *(int*)context;
	write(fd, data, size);
}

void dump_image(const void *data, unsigned width, unsigned height, size_t pitch, enum retro_pixel_format fmt, unsigned factor, int fd) {
	void *convimg = image_convert(data, width, height, pitch, fmt);
	void *scalimg = scalers[factor](convimg, width, height);
	stbi_write_bmp_to_func(cb_write, &fd, width * factor, height * factor, 3, scalimg);
	free(convimg);
	if (scalimg != convimg)
		free(scalimg);
}

