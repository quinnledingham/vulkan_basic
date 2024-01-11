#ifndef ASSETS_H
#define ASSETS_H

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_truetype.h>

/*
../assets/bitmaps/test.png

file_path = ../assets/bitmaps/test.png
file_name = test.png
folder_path = ../assets/bitmaps/
*/

struct File {
	const char *filepath;
	u32 size;
	void *memory;
};

struct Bitmap {
	u8 *memory;

	union {
		struct {
			s32 width;
			s32 height;
		};
		Vector2_s32 dim;
	};

	s32 pitch;
	s32 channels;
	
	void *gpu_handle; // information about bitmap on gpu
};

#endif // ASSETS_H