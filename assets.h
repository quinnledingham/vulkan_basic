#ifndef ASSETS_H
#define ASSETS_H

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stb_truetype.h>

/*
Assets and render are tied tightly together because a lot of the assets
have to interact with the renderer. What this means is that code in the
render code (opengl.cpp and vulkan.cpp) contains these structs.
@TODO maybe they should not be.
*/

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

File load_file(const char *filepath);

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

struct Uniform_Buffer_Object {
	void *handle; // OpenGL = u32; Vulkan = void*
	u32 size;
	u32 offset;

	u32 opengl() {
		return *(u32*)handle;
	}
};

struct Matrices {
	Matrix_4x4 model;
	Matrix_4x4 view;
	Matrix_4x4 projection;
};

struct Vertex {
	Vector3 pos;
	Vector3 color;
	Vector2 uv;
};

struct Mesh {
	Vertex *vertices;
	u32 vertices_count;

	u32 *indices;
	u32 indices_count;
    /*
    const u32 vertex_size = sizeof(Vertex);
    const u32 vertex_elements = 3;
    const u32 vertex_types[vertex_elements] = {};
    const u32 vertex_sizes[vertex_elements] = {};
    const u32 vertex_offsets[vertex_elements] = {};
    */
	void *gpu_info; // info that is used to draw this mesh
};

enum shader_types
{
    VERTEX_SHADER,                  // 0 (shader files array index)
    TESSELLATION_CONTROL_SHADER,    // 1
    TESSELLATION_EVALUATION_SHADER, // 2
    GEOMETRY_SHADER,                // 3
    FRAGMENT_SHADER,                // 4

    SHADER_TYPE_AMOUNT              // 5
};

struct Shader
{
    File files[SHADER_TYPE_AMOUNT];

    bool8 compiled;
    //b32 uniform_buffer_objects_generated;
    u32 handle;
};

u32 use_shader(Shader *shader); // returns the handle of a shader
void load_shader(Shader *shader);
void compile_shader(Shader *shader);

#endif // ASSETS_H
