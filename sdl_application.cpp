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
#include "vulkan.h"

#include "print.cpp"
#include "vulkan.cpp"

internal bool8
sdl_process_input() {
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT: return true;

			case SDL_WINDOWEVENT:
            {
                SDL_WindowEvent *window_event = &event.window;
                
                switch(window_event->event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                    {
                        
                    } break;
                }
            } break;
		}
	}

	return false;
}

int main(int argc, char *argv[]) {
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

    SDL_Window *sdl_window = SDL_CreateWindow("play_nine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 900, 800, sdl_window_flags);
    if (sdl_window == NULL) {
    	print(SDL_GetError());
    	return 1;
    }

    #ifdef OPENGL

    #elif VULKAN
    	Vulkan_Info vulkan_info;
    	//vulkan_info.extensions = ARRAY_MALLOC(const char *, 10);
    	if (SDL_Vulkan_GetInstanceExtensions(sdl_window, &vulkan_info.instance_extensions_count, vulkan_info.instance_extensions) == SDL_FALSE) {
    		logprint("main", "SDL_Vulkan_GetInstanceExtensions failed\n");
    	}
    	
    	if (vulkan_info.validation_layers.enable && !vulkan_check_validation_layer_support(vulkan_info.validation_layers)) {
			logprint("vulkan_init()", "validation layers requested, but not avaiable\n");
		}

		vulkan_create_instance(&vulkan_info);

		if (SDL_Vulkan_CreateSurface(sdl_window, vulkan_info.instance, &vulkan_info.surface) == SDL_FALSE) {
			logprint("main", "vulkan surface failed being created\n");
		}

		vulkan_pick_physical_device(&vulkan_info);
		vulkan_create_logical_device(&vulkan_info);
    #endif

    while(1) {
    	if (sdl_process_input())
    		break;
    }

    #ifdef OPENGL

    #elif VULKAN
    	//vkDestroyInstance(instance, nullptr);
    #endif

    SDL_DestroyWindow(sdl_window);

	return 0;
}