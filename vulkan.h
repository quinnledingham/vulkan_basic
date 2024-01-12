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

struct Vertex {
	Vector3 pos;
	Vector3 color;
	Vector2 uv;
};

struct Uniform_Buffer_Object {
	Matrix_4x4 model;
	Matrix_4x4 view;
	Matrix_4x4 projection;
};

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

const u16 indices[12] = {
	0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

struct Vulkan_Info {
	const char *device_extensions[1] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	static const u32 MAX_FRAMES_IN_FLIGHT = 2;
	u32 current_frame;

	s32 window_width;
	s32 window_height;
	bool8 framebuffer_resized = false;
	bool8 minimized;

	Vulkan_Validation_Layers validation_layers;
	Vulkan_Swap_Chain_Support_Details swap_chain_support_details;

	VkDebugUtilsMessengerEXT debug_messenger;

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
	Arr<VkImage> swap_chain_images;
	VkFormat swap_chain_image_format;
	VkExtent2D swap_chain_extent;
	Arr<VkImageView> swap_chain_image_views;
	Arr<VkFramebuffer> swap_chain_framebuffers;

	// sync
	Arr<VkSemaphore> image_available_semaphore;
	Arr<VkSemaphore> render_finished_semaphore;
	Arr<VkFence> in_flight_fence;

	// Buffers
/*
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
*/
	u32 vertices_offset;
	u32 indices_offset;
	
	VkBuffer combined_buffer;
	VkDeviceMemory combined_buffer_memory;

	//Arr<VkBuffer> uniform_buffers;
	//Arr<VkDeviceMemory> uniform_buffers_memory;
	//Arr<void*> uniform_buffers_mapped;
	void *uniform_buffers_mapped;
	VkDeviceSize uniforms_offset[MAX_FRAMES_IN_FLIGHT];


	VkDescriptorPool descriptor_pool;
	Arr<VkDescriptorSet> descriptor_sets;

	// Images
	VkImage texture_image;
	const VkFormat texture_image_format = VK_FORMAT_R8G8B8A8_SRGB;
	VkDeviceMemory texture_image_memory;
	VkImageView texture_image_view;
	VkSampler texture_sampler;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;
};
