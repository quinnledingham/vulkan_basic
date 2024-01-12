#include <SDL.h>

// Platform specific even while use SDL
#ifdef WINDOWS
	#pragma message("WINDOWS")
	#define WIN32_EXTRA_LEAN
	#define WIN32_EXTRA_LEAN
	#include <windows.h>

	//
	// We prefer the discrete GPU in laptops where available
	//
	extern "C"
	{
	    __declspec(dllexport) DWORD NvOptimusEnablement = 0x01;
	    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x01;
	}

#endif // WINDOWS

#ifdef OPENGL
#pragma message("OPENGL")
	#include <gl.h>
	#include <gl.c>
#elif VULKAN
	#pragma message("VULKAN")
	#include <SDL_vulkan.h>
	#include <vulkan/vulkan.h>
	
	// Compiling to SPIR in code
	#include <shaderc/env.h>
	#include <shaderc/shaderc.h>
	#include <shaderc/shaderc.hpp>
	#include <shaderc/status.h>
	#include <shaderc/visibility.h>

	#define VK_USE_PLATFORM_WIN32_KHR
#endif // OPENGL / VULKAN

#include <stdarg.h>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "types.h"

void *platform_malloc(u32 size) { return SDL_malloc(size); }
void platform_free(void *ptr)   { SDL_free(ptr); }
void platform_memory_copy(void *dest, void *src, u32 num_of_bytes) { SDL_memcpy(dest, src, num_of_bytes); }
void platform_memory_set(void *dest, s32 value, u32 num_of_bytes) { SDL_memset(dest, value, num_of_bytes); }

#define ARRAY_COUNT(n)     (sizeof(n) / sizeof(n[0]))
#define ARRAY_MALLOC(t, n) ((t*)platform_malloc(n * sizeof(t)))

#include "print.h"
#include "types_math.h"
#include "char_array.h"
#include "assets.h"
#include "data_structs.h"

#ifdef OPENGL


#elif VULKAN

	#include "vulkan.h"

#endif // OPENGL/ VULKAN

#include "application.h"
#include "print.cpp"
#include "assets.cpp"

#ifdef OPENGL

	#include "opengl.cpp"

#elif VULKAN

	#include "vulkan.cpp"

#endif // OPENGL/ VULKAN


const Vertex vertices[8] = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const u32 indices[12] = {
	0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

Shader shader = {};
Mesh mesh = {};
Uniform_Buffer_Object matrices_ubo = {};

#if OPENGL

internal void
sdl_init_opengl(SDL_Window *sdl_window)
{
    SDL_GL_LoadLibrary(NULL);
    
    // Request an OpenGL 4.6 context (should be core)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,    1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    
    // Also request a depth buffer
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    
    SDL_GLContext Context = SDL_GL_CreateContext(sdl_window);
    SDL_GL_SetSwapInterval(0); // vsync: 0 off, 1 on
    
    // Check OpenGL properties
    gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
    print("OpenGL loaded:\n");
    print("Vendor:   %s\n", glGetString(GL_VENDOR));
    print("Renderer: %s\n", glGetString(GL_RENDERER));
    print("Version:  %s\n", glGetString(GL_VERSION));

	// GL defaults
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPatchParameteri(GL_PATCH_VERTICES, 4); 
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_FRAMEBUFFER_SRGB); 
	
	shader.files[VERTEX_SHADER].filepath = "../assets/shaders/basic.vert";
	shader.files[FRAGMENT_SHADER].filepath = "../assets/shaders/basic.frag";
	load_shader(&shader);
	compile_shader(&shader);

	// set up test mesh
	mesh.vertices_count = 8;
	mesh.indices_count = 12;
	mesh.vertices = ARRAY_MALLOC(Vertex, mesh.vertices_count);
	mesh.indices = ARRAY_MALLOC(u32, mesh.indices_count);
	memcpy(mesh.vertices, vertices, sizeof(vertices));
	memcpy(mesh.indices, indices, sizeof(indices));
	init_gpu_mesh(&mesh);

	matrices_ubo = opengl_init_uniform_buffer_object(sizeof(Matrices), 0);

	s32 window_width;
	s32 window_height;
	SDL_GetWindowSize(sdl_window, &window_width, &window_height);

	u32 tag_uniform_block_index = glGetUniformBlockIndex(shader.handle, "ubo");
    glUniformBlockBinding(shader.handle, tag_uniform_block_index, matrices_ubo.opengl());

	Matrices ubo = {};
	ubo.model = create_transform_m4x4({ 0.0f, 0.0f, 0.0f }, get_rotation(0.0f, {0, 0, 1}), {1.0f, 1.0f, 1.0f});
	ubo.view = look_at({ 2.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	ubo.projection = perspective_projection(45.0f, (float32)window_width / (float32)window_height, 0.1f, 10.0f);
	ubo.projection.E[1][1] *= 1;

	opengl_update_uniform_buffer_object(matrices_ubo, ubo);
}

internal void
update_window(u32 width, u32 height, bool8 minimized)
{
    glViewport(0, 0, width, height);
    //window->aspect_ratio = (r32)window->dim.width / (r32)window->dim.height;
    //*window->update_matrices = true;
}

#elif VULKAN

internal void
sdl_init_vulkan(Vulkan_Info *info, SDL_Window *sdl_window) {
    SDL_GetWindowSize(sdl_window, &info->window_width, &info->window_height);

	if (SDL_Vulkan_GetInstanceExtensions(sdl_window, &info->instance_extensions_count, NULL) == SDL_FALSE) {
		logprint("main", "nullptr SDL_Vulkan_GetInstanceExtensions failed\n");
	}
	info->instance_extensions = ARRAY_MALLOC(const char *, info->instance_extensions_count);
	if (SDL_Vulkan_GetInstanceExtensions(sdl_window, &info->instance_extensions_count, info->instance_extensions) == SDL_FALSE) {
		logprint("main", "SDL_Vulkan_GetInstanceExtensions failed\n");
	}
	
	if (info->validation_layers.enable && !vulkan_check_validation_layer_support(info->validation_layers)) {
		logprint("vulkan_init()", "validation layers requested, but not avaiable\n");
	}

	vulkan_create_instance(info);
	vulkan_setup_debug_messenger(info);

	if (SDL_Vulkan_CreateSurface(sdl_window, info->instance, &info->surface) == SDL_FALSE) {
		logprint("main", "vulkan surface failed being created\n");
	}

	vulkan_pick_physical_device(info);
	vulkan_create_logical_device(info);
	vulkan_create_swap_chain(info);
	vulkan_create_image_views(info);
	vulkan_create_render_pass(info);
	vulkan_create_descriptor_set_layout(info);

	Vulkan_Graphics_Pipeline pipeline_info = {};
	pipeline_info.binding_description.binding = 0;
	pipeline_info.binding_description.stride = sizeof(Vertex);
	pipeline_info.binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	pipeline_info.attribute_descriptions[0].binding = 0;
	pipeline_info.attribute_descriptions[0].location = 0;
	pipeline_info.attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	pipeline_info.attribute_descriptions[0].offset = offsetof(Vertex, pos);

	pipeline_info.attribute_descriptions[1].binding = 0;
	pipeline_info.attribute_descriptions[1].location = 1;
	pipeline_info.attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	pipeline_info.attribute_descriptions[1].offset = offsetof(Vertex, color);	

	pipeline_info.attribute_descriptions[2].binding = 0;
	pipeline_info.attribute_descriptions[2].location = 2;
	pipeline_info.attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	pipeline_info.attribute_descriptions[2].offset = offsetof(Vertex, uv);

	vulkan_create_graphics_pipeline(info, &pipeline_info);

	vulkan_create_command_pool(info);
	vulkan_create_depth_resources(info);
	vulkan_create_frame_buffers(info);

	Bitmap yogi = load_bitmap("../assets/bitmaps/yogi.png");
	vulkan_create_texture_image(info, &yogi);
	free_bitmap(yogi);
	vulkan_create_texture_image_view(info);
	vulkan_create_texture_sampler(info);
	
	Matrices ubo = {};
	ubo.model = create_transform_m4x4({ 0.0f, 0.0f, 0.0f }, get_rotation(0.0f, {0, 0, 1}), {1.0f, 1.0f, 1.0f});
	ubo.view = look_at({ 2.0f, 2.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	ubo.projection = perspective_projection(45.0f, (float32)info->window_width / (float32)info->window_height, 0.1f, 10.0f);
	ubo.projection.E[1][1] *= -1;

	info->uniforms_offset[0] = vulkan_get_alignment(sizeof(vertices) + sizeof(indices), 64);
	info->uniforms_offset[1] = vulkan_get_alignment(info->uniforms_offset[0] + sizeof(Matrices), 64);
	info->uniform_size = sizeof(Matrices);

	u32 combined_size = (u32)info->uniforms_offset[1] + sizeof(Matrices);
	void *memory = platform_malloc(combined_size);
	memcpy(memory, (void*)&vertices, sizeof(vertices));
	memcpy((char*)memory + sizeof(vertices), (void*)&indices, sizeof(indices));
	memcpy((char*)memory + info->uniforms_offset[0], (void*)&ubo, sizeof(ubo));
	memcpy((char*)memory + info->uniforms_offset[1], (void*)&ubo, sizeof(ubo));

	vulkan_create_buffer(info->device, 
						 info->physical_device,
						 combined_size, 
						 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 info->combined_buffer,
						 info->combined_buffer_memory);

	vulkan_update_buffer(info, &info->combined_buffer, &info->combined_buffer_memory, memory, combined_size);
	
	//vulkan_setup_uniform_buffers(info);
	//matrices_ubo.handle = info->uniform_buffers_mapped;
	
	vulkan_create_descriptor_pool(info);
	vulkan_create_descriptor_sets(info);

	vulkan_create_command_buffers(info);
	vulkan_create_sync_objects(info);
}

internal void
update_window(u32 width, u32 height, bool8 minimized) {
	vulkan_info.minimized = minimized;
	vulkan_info.window_width = width;
	vulkan_info.window_height = height;
	vulkan_info.framebuffer_resized = true;
	if (vulkan_info.window_width != 0 && vulkan_info.window_height != 0) {
		vulkan_info.minimized = false;
	}
}

#endif // OPENGL / VULKAN

internal bool8
sdl_process_input() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: return true;

			case SDL_WINDOWEVENT:{
                SDL_WindowEvent *window_event = &event.window;
                bool8 minimized = false;	
				
                switch(window_event->event) {
					case SDL_WINDOWEVENT_MINIMIZED:
						minimized = true;
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
						update_window(window_event->data1, window_event->data2, minimized);
                    } break;
                }
            } break;
		}
	}

	return false;
}


internal void
sdl_update_time(App_Time *time) {
    s64 ticks = SDL_GetPerformanceCounter();

    // s
    time->frame_time_s = (float32)get_seconds_elapsed(time->performance_frequency, time->last_frame_ticks, ticks);
    time->last_frame_ticks = ticks; // set last ticks for next frame

    // time->start has to be initialized before
    time->run_time_s = (float32)get_seconds_elapsed(time->performance_frequency, time->start_ticks, ticks);

    // fps
    time->frames_per_s = (float32)(1.0 / time->frame_time_s);
	
	if (time->avg_fps_sum_count == UINT32_MAX) {
		time->avg_fps_sum_count = 0;
		time->avg_fps = 0.0f;
		logprint("sdl_update_time()", "reseted avg fps\n");
	}

	time->avg_fps *= time->avg_fps_sum_count;
	time->avg_fps += time->frames_per_s;
	time->avg_fps /= ++time->avg_fps_sum_count;
}

int main(int argc, char *argv[]) {
	print("starting application...\n");

	App app = {};

	app.time.performance_frequency = SDL_GetPerformanceFrequency();
    app.time.start_ticks = SDL_GetPerformanceCounter();
    app.time.last_frame_ticks = app.time.start_ticks;

	u32 sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO;
    if (SDL_Init(sdl_init_flags)) {
    	print(SDL_GetError());
    	return 1;
    }

    u32 sdl_window_flags = SDL_WINDOW_RESIZABLE;

#ifdef OPENGL
	sdl_window_flags = sdl_window_flags | SDL_WINDOW_OPENGL;
#elif VULKAN
	sdl_window_flags = sdl_window_flags | SDL_WINDOW_VULKAN;
#endif // OPENGL / VULKAN

    SDL_Window *sdl_window = SDL_CreateWindow("vulkan_basic", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 900, 800, sdl_window_flags);
    if (sdl_window == NULL) {
    	print(SDL_GetError());
    	return 1;
    }

    s32 window_width;
    s32 window_height;
    SDL_GetWindowSize(sdl_window, &window_width, &window_height);
	
#ifdef OPENGL
	sdl_init_opengl(sdl_window);
#elif VULKAN
	//Vulkan_Info vulkan_info = {};
	sdl_init_vulkan(&vulkan_info, sdl_window);
	//vulkan_update_uniform_buffer_object(matrices_ubo, ubo);
	VkBuffer buffers[] = { vulkan_info.combined_buffer };
	VkDeviceSize offsets[] = { 0 };
#endif

    while(1) {
    	if (sdl_process_input())
    		break;

		sdl_update_time(&app.time);
		print("fps: %f,    avg_fps: %f\n", app.time.frames_per_s, app.time.avg_fps);

#if OPENGL
		 u32 gl_clear_flags = 
            GL_COLOR_BUFFER_BIT  | 
            GL_DEPTH_BUFFER_BIT  | 
            GL_STENCIL_BUFFER_BIT;
    
        glClear(gl_clear_flags);
				
		use_shader(&shader);
		draw_mesh(&mesh);

		SDL_GL_SwapWindow(sdl_window);		
#elif VULKAN
		if (vulkan_info.minimized)
			continue;

		vulkan_start_frame(&vulkan_info);

		// Drawing Buffer
		
		vkCmdBindVertexBuffers(vulkan_info.command_buffer, 0, 1, buffers, offsets);
		vkCmdBindIndexBuffer(vulkan_info.command_buffer, vulkan_info.combined_buffer, sizeof(vertices), VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(vulkan_info.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_info.pipeline_layout, 0, 1, &vulkan_info.descriptor_sets[vulkan_info.current_frame], 0, nullptr);
		vkCmdDrawIndexed(vulkan_info.command_buffer, ARRAY_COUNT(indices), 1, 0, 0, 0);
		
		vulkan_draw_frame(&vulkan_info);
#endif // VULKAN
    }

#ifdef OPENGL

#elif VULKAN
	vkDeviceWaitIdle(vulkan_info.device);
    vulkan_cleanup(&vulkan_info);
#endif

    SDL_DestroyWindow(sdl_window);

	return 0;
}