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

#elif VULKAN
	#pragma message("VULKAN")
	#include <SDL_vulkan.h>
	#include <vulkan/vulkan.h>

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
#include "vulkan.h"
#include "application.h"

#include "print.cpp"
#include "assets.cpp"
#include "vulkan.cpp"

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

	//shaderc_compiler_t compiler = shaderc_compiler_initialize();
	//shaderc_compilation_result *result = shaderc_compile_into_spv(compiler, "#version 450\nvoid main() {}", 27, shaderc_glsl_vertex_shader, "main.vert", "main", nullptr);
	
	vulkan_pick_physical_device(info);
	vulkan_create_logical_device(info);
	vulkan_create_swap_chain(info);
	vulkan_create_image_views(info);
	vulkan_create_render_pass(info);
	vulkan_create_descriptor_set_layout(info);
	vulkan_create_graphics_pipeline(info);
	vulkan_create_command_pool(info);
	vulkan_create_depth_resources(info);
	vulkan_create_frame_buffers(info);

	Bitmap yogi = load_bitmap("../assets/bitmaps/yogi.png");
	vulkan_create_texture_image(info, &yogi);
	free_bitmap(yogi);
	vulkan_create_texture_image_view(info);
	vulkan_create_texture_sampler(info);
	
	u32 combined_size = sizeof(vertices) + sizeof(indices) + (sizeof(Uniform_Buffer_Object) * info->MAX_FRAMES_IN_FLIGHT);
	void *memory = platform_malloc(combined_size);
	memcpy(memory, (void*)&vertices, sizeof(vertices));
	memcpy((char*)memory + sizeof(vertices), (void*)&indices, sizeof(indices));

	info->uniforms_offset[0] = vulkan_get_alignment(sizeof(vertices) + sizeof(indices), 64);
	info->uniforms_offset[1] = vulkan_get_alignment(info->uniforms_offset[0], 64);

	vulkan_create_Vertex_buffer(info, &info->combined_buffer, &info->combined_buffer_memory, memory, combined_size);
	
	vulkan_setup_uniform_buffers(info);
	
	vulkan_create_descriptor_pool(info);
	vulkan_create_descriptor_sets(info);

	vulkan_create_command_buffers(info);
	vulkan_create_sync_objects(info);
}

internal bool8
sdl_process_input(Vulkan_Info *vulkan_info) {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: return true;

			case SDL_WINDOWEVENT:{
                SDL_WindowEvent *window_event = &event.window;
                
                switch(window_event->event) {
					case SDL_WINDOWEVENT_MINIMIZED:
						vulkan_info->minimized = true;
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED: {
						vulkan_info->window_width = window_event->data1;
						vulkan_info->window_height = window_event->data2;
                        vulkan_info->framebuffer_resized = true;

						if (vulkan_info->window_width != 0 && vulkan_info->window_height != 0) {
							vulkan_info->minimized = false;
						}
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

#elif VULKAN
	Vulkan_Info vulkan_info = {};
	sdl_init_vulkan(&vulkan_info, sdl_window);
#endif

    while(1) {
    	if (sdl_process_input(&vulkan_info))
    		break;

		sdl_update_time(&app.time);
		//print("%f\n", app.time.frames_per_s);

		vulkan_draw_frame(&vulkan_info);
    }

#ifdef OPENGL

#elif VULKAN
	vkDeviceWaitIdle(vulkan_info.device);
    vulkan_cleanup(&vulkan_info);
#endif

    SDL_DestroyWindow(sdl_window);

	return 0;
}