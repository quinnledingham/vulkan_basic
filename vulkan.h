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

struct Vulkan_Info {
	Vulkan_Validation_Layers validation_layers;
	Vulkan_Swap_Chain_Support_Details swap_chain_support_details;

	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;

	VkQueue graphics_queue;
	VkQueue present_queue;

	VkSurfaceKHR surface;

	u32 instance_extensions_count;
	const char *instance_extensions[2];
};