struct Vulkan_Validation_Layers {
	const char *data[1] = { "VK_LAYER_KHRONOS_validation" };
	const u32 count = ARRAY_COUNT(data);

#ifdef DEBUG
	const bool8 enable = true;
#else
	const bool8 enable = false;
#endif // DEBUG
};

struct Vulkan_Queue_Family_Indices {
	bool8 graphics_family_found;
	bool8 present_family_found;

	u32 graphics_family;
	u32 present_family;

	static const u32 unique_families = 2;
};

struct Vulkan_Swap_Chain_Support_Details {
	VkSurfaceCapabilitiesKHR capabilities;
	
	VkSurfaceFormatKHR *formats;
	u32 formats_count;

	VkPresentModeKHR *present_modes;
	u32 present_modes_count;
};

const char *device_extensions[1] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

template<typename T>
struct Arr {
	T *data;
	u32 size;  // number of elements of memory allocated at data
	u32 count; // number of elements added to memory at data

	u32 data_size;
	u32 type_size;

	T& operator [] (int i) { 
		if (i >= (int)size) {
			print("WARNING: tried to access memory outside of Arr range. Returned last element instead\n");
			return data[size - 1];
		}

		return data[i]; 
	}

	T operator [] (int i) const {
		return (T)data[i];
	}

	T& get_data() {
		return (T)*data;
	}
};

template<typename T>
internal void
arr_resize(Arr<T> *arr, u32 size) {
	if (arr->data == 0)
		platform_free(arr->data);
	arr->type_size = sizeof(T);
	arr->data_size = arr->type_size * size;
	arr->data = (T*)platform_malloc(arr->data_size);
	arr->size = size;
	arr->count = 0;
}

struct Vertex {
	Vector2 pos;
	Vector3 color;
};

struct Uniform_Buffer_Object {
	Matrix_4x4 model;
	Matrix_4x4 view;
	Matrix_4x4 projection;
};

const Vertex vertices[4] = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const u16 indices[6] = {
	0, 1, 2, 2, 3, 0
};

struct Vulkan_Info {
	const u32 MAX_FRAMES_IN_FLIGHT = 2;
	u32 current_frame;

	s32 window_width;
	s32 window_height;
	bool8 framebuffer_resized = false;
	bool8 minimized;

	Vulkan_Validation_Layers validation_layers;
	Vulkan_Swap_Chain_Support_Details swap_chain_support_details;

	u32 instance_extensions_count;
	const char **instance_extensions;

	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkSurfaceKHR surface;
	VkRenderPass render_pass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	VkQueue graphics_queue;
	VkQueue present_queue;

	VkCommandPool command_pool;
	Arr<VkCommandBuffer> command_buffers;

	// swap_chain
	VkSwapchainKHR swap_chain;
	VkImage *swap_chain_images;
	u32 swap_chain_images_count;
	VkFormat swap_chain_image_format;
	VkExtent2D swap_chain_extent;
	Arr<VkImageView> swap_chain_image_views;
	Arr<VkFramebuffer> swap_chain_framebuffers;

	// sync
	Arr<VkSemaphore> image_available_semaphore;
	Arr<VkSemaphore> render_finished_semaphore;
	Arr<VkFence> in_flight_fence;

	// Buffers
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;

	Arr<VkBuffer> uniform_buffers;
	Arr<VkDeviceMemory> uniform_buffers_memory;
	Arr<void*> uniform_buffers_mapped;

	VkDescriptorPool descriptor_pool;
	Arr<VkDescriptorSet> descriptor_sets;
};
