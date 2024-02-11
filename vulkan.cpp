internal VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
	print("validation layer: %s", callback_data->pMessage);
	return VK_FALSE;
}

internal void 
vulkan_populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = vulkan_debug_callback;
}

internal VkResult 
vulkan_create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger) {
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, create_info, allocator, debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

internal void 
vulkan_destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, allocator);
    }
}

internal void
vulkan_setup_debug_messenger(Vulkan_Info *info) {
	if (!info->validation_layers.enable)
		return;

	VkDebugUtilsMessengerCreateInfoEXT create_info;
    vulkan_populate_debug_messenger_create_info(create_info);

	if (vulkan_create_debug_utils_messenger_ext(info->instance, &create_info, nullptr, &info->debug_messenger) != VK_SUCCESS) {
		logprint("vulkan_create_debug_messenger()", "failed to set up debug messenger\n");
	}
}

internal bool8
vulkan_check_validation_layer_support(Vulkan_Validation_Layers validation_layers) {
    u32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    VkLayerProperties *available_layers = ARRAY_MALLOC(VkLayerProperties, layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    bool8 all_layers_found = true;
    for (u32 validation_index = 0; validation_index < validation_layers.count; validation_index++) {
    	bool8 layer_found = false;

    	for (u32 available_index = 0; available_index < layer_count; available_index++) {
    		if (equal(validation_layers.data[validation_index], available_layers[available_index].layerName)) {
    			layer_found = true;
    			break;
    		}
    	}

    	if (!layer_found) {
    		all_layers_found = false;
    		break;
    	}
    }

    platform_free(available_layers);

    return all_layers_found;
}

internal void
vulkan_create_instance(Vulkan_Info *vulkan_info) {
    VkInstance instance;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = vulkan_info->instance_extensions_count;
	create_info.ppEnabledExtensionNames = (const char *const *)vulkan_info->instance_extensions;

	VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
	if (vulkan_info->validation_layers.enable) {
        create_info.enabledLayerCount = vulkan_info->validation_layers.count;
        create_info.ppEnabledLayerNames = vulkan_info->validation_layers.data;

		vulkan_populate_debug_messenger_create_info(debug_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
	} else {
		create_info.enabledLayerCount = 0;
		create_info.pNext = nullptr;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
	    logprint("main()", "failed to create vulkan instance");
	}

	vulkan_info->instance = instance;
}


internal Vulkan_Queue_Family_Indices
vulkan_find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface) {
	Vulkan_Queue_Family_Indices indices;

	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

	VkQueueFamilyProperties *queue_families = ARRAY_MALLOC(VkQueueFamilyProperties, queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

	for (u32 queue_index = 0; queue_index < queue_family_count; queue_index++) {
		if (queue_families[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family_found = true;
			indices.graphics_family = queue_index;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, queue_index, surface, &present_support);
		if (present_support) {
			indices.present_family_found = true;
			indices.present_family = queue_index;
		}
	}

	platform_free(queue_families);

	return indices;
}

internal bool8
vulkan_check_device_extension_support(VkPhysicalDevice device, const char **device_extensions, u32 device_extensions_count) {
	u32 available_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &available_count, nullptr);

    VkExtensionProperties *available_extensions = ARRAY_MALLOC(VkExtensionProperties, available_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &available_count, available_extensions);

    bool8 all_extensions_found = true;
    for (u32 required_index = 0; required_index < device_extensions_count; required_index++) {
    	bool8 extension_found = false;
    	for (u32 available_index = 0; available_index < available_count; available_index++) {
    		if (equal(available_extensions[available_index].extensionName, device_extensions[required_index])) {
    			extension_found = true;
    			break;
    		}
    	}

    	if (!extension_found) {
    		all_extensions_found = false;
    		break;
    	}
    }
	
	platform_free(available_extensions);

    return all_extensions_found;
}

internal Vulkan_Swap_Chain_Support_Details
vulkan_query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
	Vulkan_Swap_Chain_Support_Details details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats_count, nullptr);
	if (details.formats_count != 0) {
		details.formats = ARRAY_MALLOC(VkSurfaceFormatKHR, details.formats_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats_count, details.formats);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_modes_count, nullptr);
	if (details.present_modes_count != 0) {
		details.present_modes = ARRAY_MALLOC(VkPresentModeKHR, details.present_modes_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_modes_count, nullptr);
	}

	return details;
}

internal bool8
vulkan_is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface, const char **device_extensions, u32 device_extensions_count) {
	/*
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	vkGetPhysicalDeviceFeatures(device, &device_features);

	return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader;
	*/

	Vulkan_Queue_Family_Indices indices = vulkan_find_queue_families(device, surface);

	bool8 extensions_supported = vulkan_check_device_extension_support(device, device_extensions, device_extensions_count);
	bool8 swap_chain_adequate = false;
	if (extensions_supported) {
		Vulkan_Swap_Chain_Support_Details swap_chain_support = vulkan_query_swap_chain_support(device, surface);
		swap_chain_adequate = swap_chain_support.formats_count && swap_chain_support.present_modes_count;
		platform_free(swap_chain_support.formats);
		platform_free(swap_chain_support.present_modes);
	}

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return indices.graphics_family_found && extensions_supported && swap_chain_adequate && supported_features.samplerAnisotropy;
}

internal void
vulkan_pick_physical_device(Vulkan_Info *info) {
	info->physical_device = VK_NULL_HANDLE;

	u32 device_count = 0;
	vkEnumeratePhysicalDevices(info->instance, &device_count, nullptr);

	if (device_count == 0)
		logprint("vulkan_pick_physical_device()", "failed to find GPUs with Vulkan support");

	VkPhysicalDevice *devices = ARRAY_MALLOC(VkPhysicalDevice, device_count);
	vkEnumeratePhysicalDevices(info->instance, &device_count, devices);

	for (u32 device_index = 0; device_index < device_count; device_index++) {
		if (vulkan_is_device_suitable(devices[device_index], info->surface, info->device_extensions, ARRAY_COUNT(info->device_extensions))) {
			info->physical_device = devices[device_index];
			break;
		}
	}

	platform_free(devices);

	if (info->physical_device == VK_NULL_HANDLE) {
		logprint("vulkan_pick_physical_device()", "failed to find a suitable GPU\n");
	}
}

internal void
vulkan_create_logical_device(Vulkan_Info *info) {
	Vulkan_Queue_Family_Indices indices = vulkan_find_queue_families(info->physical_device, info->surface);

	// Specify the device queues we want
	VkDeviceQueueCreateInfo *queue_create_infos = ARRAY_MALLOC(VkDeviceQueueCreateInfo, indices.unique_families);
	u32 unique_queue_families[indices.unique_families] = { indices.graphics_family, indices.present_family };

	float32 queue_priority = 1.0f;
	for (u32 queue_index = 0; queue_index < indices.unique_families; queue_index++) {
		VkDeviceQueueCreateInfo queue_create_info = {};
	    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	    queue_create_info.queueFamilyIndex = unique_queue_families[queue_index];
	    queue_create_info.queueCount = 1;
	    queue_create_info.pQueuePriorities = &queue_priority;
	    queue_create_infos[queue_index] = queue_create_info;
	}

	// Features requested
	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;

	// Set up device
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.queueCreateInfoCount = indices.unique_families;
	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = ARRAY_COUNT(info->device_extensions);
	create_info.ppEnabledExtensionNames = (const char *const *)info->device_extensions;

	if (info->validation_layers.enable) {
		create_info.enabledLayerCount = info->validation_layers.count;
		create_info.ppEnabledLayerNames = info->validation_layers.data;
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(info->physical_device, &create_info, nullptr, &info->device) != VK_SUCCESS) {
		logprint("vulkan_create_logical_device", "failed to create logical device\n");
	}

	// Create the queues
	vkGetDeviceQueue(info->device, indices.graphics_family, 0, &info->graphics_queue);
	vkGetDeviceQueue(info->device, indices.present_family, 0, &info->present_queue);

	platform_free(queue_create_infos);
}

internal VkSurfaceFormatKHR
vulkan_choose_swap_surface_format(VkSurfaceFormatKHR *formats, u32 count) {
	for (u32 i = 0; i < count; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return formats[i];
		}
	}

	return formats[0];
}

// VSYNC OFF : VK_PRESENT_MODE_IMMEDIATE_KHR 
// VSYNC ON  : VK_PRESENT_MODE_MAILBOX_KHR
internal VkPresentModeKHR
vulkan_choose_swap_present_mode(VkPresentModeKHR *modes, u32 count) {
	for (u32 i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			return modes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR; // this mode is required to be supported
}

internal VkExtent2D
vulkan_choose_swap_extent(const VkSurfaceCapabilitiesKHR &capabilities, u32 window_width, u32 window_height) {
	if (capabilities.currentExtent.width != 0xFFFFFFFF) {
		return capabilities.currentExtent;
	} else {
		VkExtent2D actual_extent = { window_width, window_height };
		actual_extent.width = clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actual_extent;
	}
}

internal void
vulkan_create_swap_chain(Vulkan_Info *info) {
	Vulkan_Swap_Chain_Support_Details swap_chain_support = vulkan_query_swap_chain_support(info->physical_device, info->surface);

	VkSurfaceFormatKHR surface_format = vulkan_choose_swap_surface_format(swap_chain_support.formats, swap_chain_support.formats_count);
	VkPresentModeKHR present_mode = vulkan_choose_swap_present_mode(swap_chain_support.present_modes, swap_chain_support.present_modes_count);
	VkExtent2D extent = vulkan_choose_swap_extent(swap_chain_support.capabilities, info->window_width, info->window_height);

	u32 image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = info->surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	Vulkan_Queue_Family_Indices indices = vulkan_find_queue_families(info->physical_device, info->surface);
	u32 queue_family_indices[2] = { indices.graphics_family, indices.present_family };

	if (indices.graphics_family != indices.present_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	} else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0; // optional
		create_info.pQueueFamilyIndices = nullptr; // optional
	}

	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(info->device, &create_info, nullptr, &info->swap_chains[0])) {
		logprint("vulkan_create_swap_chain()", "failed to create swap chain\n");
	}
	
	u32 swap_chain_images_count = 0;
	vkGetSwapchainImagesKHR(info->device, info->swap_chains[0], &swap_chain_images_count, nullptr);
	info->swap_chain_images.resize(swap_chain_images_count);
	vkGetSwapchainImagesKHR(info->device, info->swap_chains[0], &swap_chain_images_count, info->swap_chain_images.get_data());

	info->swap_chain_image_format = surface_format.format;
	info->swap_chain_extent = extent;
}

internal VkImageView
vulkan_create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags) {
	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VkImageView image_view;
    if (vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
        logprint("vulkan_create_image_view()", "failed to create texture image view\n");
    }

    return image_view;
}


internal void
vulkan_create_image_views(Vulkan_Info *info) {
	info->swap_chain_image_views.resize(info->swap_chain_images.get_size());
	for (u32 i = 0; i < info->swap_chain_images.get_size(); i++) {
		info->swap_chain_image_views[i] = vulkan_create_image_view(info->device, info->swap_chain_images[i], info->swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

internal VkFormat
vulkan_find_supported_format(VkPhysicalDevice physical_device, VkFormat *candidates, u32 candidates_count, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (u32 i = 0; i < candidates_count; i++) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
             return candidates[i];
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
		    return candidates[i];
		}
	}

	logprint("vulkan_find_supported_format()", "failed to find supported format\n");
	return {};
}

internal VkFormat
vulkan_find_depth_format(VkPhysicalDevice physical_device) {
	VkFormat candidates[3] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

	VkFormat format = vulkan_find_supported_format(physical_device, candidates, 3, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	return format;
}

internal void
vulkan_create_render_pass(Vulkan_Info *info) {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format         = info->swap_chain_image_format;
	color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = vulkan_find_depth_format(info->physical_device);
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;
	
	// Subpass dependencies
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	
	VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = ARRAY_COUNT(attachments);
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	if (vkCreateRenderPass(info->device, &render_pass_info, nullptr, &info->render_pass) != VK_SUCCESS) {
		logprint("vulkan_create_render_pass()", "failed to create render pass\n");
	}
}

internal VkShaderModule
vulkan_create_shader_module(VkDevice device, File code) {
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size;
	create_info.pCode = (u32*)code.memory;

	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		logprint("vulkan_create_shader_module()", "failed to create shader module\n");
	}

	return shader_module;
}

internal File
vulkan_load_shader(shaderc_compiler_t compiler, const char *filepath, shaderc_shader_kind shader_kind) {
	File file = load_file(filepath);
	const shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, (char*)file.memory, file.size, shader_kind, get_filename(filepath), "main", nullptr);

	u32 num_of_warnings = (u32)shaderc_result_get_num_warnings(result);
	u32 num_of_errors = (u32)shaderc_result_get_num_errors(result);

	if (num_of_warnings != 0 || num_of_errors != 0) {
		const char *error_message = shaderc_result_get_error_message(result);
		logprint("vulkan_load_shader()", "%s", error_message);
	}

	u32 length = (u32)shaderc_result_get_length(result);
	const char *bytes = shaderc_result_get_bytes(result);

	File result_file = {};
	result_file.memory = (void*)bytes;
	result_file.size = length;
	return result_file;
}

internal void
vulkan_create_graphics_pipeline(Vulkan_Info *info, Vulkan_Graphics_Pipeline *pipeline_info) {
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	File vert = vulkan_load_shader(compiler, "../assets/shaders/basic.vert", shaderc_glsl_vertex_shader);
	File frag = vulkan_load_shader(compiler, "../assets/shaders/basic.frag", shaderc_glsl_fragment_shader);

	//File vert = load_file("../vert.spv");
	//File frag = load_file("../frag.spv");

	VkShaderModule vert_shader_module = vulkan_create_shader_module(info->device, vert);
	VkShaderModule frag_shader_module = vulkan_create_shader_module(info->device, frag);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = ARRAY_COUNT(dynamic_states);
	dynamic_state.pDynamicStates = dynamic_states;

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &pipeline_info->binding_description;     // Optional
	vertex_input_info.vertexAttributeDescriptionCount = 3;
	vertex_input_info.pVertexAttributeDescriptions = pipeline_info->attribute_descriptions; // Optional

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;      // Optional
	rasterizer.depthBiasClamp = 0.0f;               // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;         // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading      = 1.0f;                  // Optional
	multisampling.pSampleMask           = nullptr;               // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;              // Optional
	multisampling.alphaToOneEnable      = VK_FALSE; 			 // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable         = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;           // Optional
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;             // Optional
	color_blending.blendConstants[1] = 0.0f;             // Optional
	color_blending.blendConstants[2] = 0.0f;             // Optional
	color_blending.blendConstants[3] = 0.0f;             // Optional
	
	VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f;               // Optional
	depth_stencil.maxDepthBounds = 1.0f;               // Optional
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.front = {};                          // Optional
	depth_stencil.back = {};                           // Optional

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount         = 1;                            // Optional
	pipeline_layout_info.pSetLayouts            = &info->descriptor_set_layout; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0;                            // Optional
	pipeline_layout_info.pPushConstantRanges    = nullptr;                      // Optional
	
	if (vkCreatePipelineLayout(info->device, &pipeline_layout_info, nullptr, &info->pipeline_layout) != VK_SUCCESS) {
		logprint("vulkan_create_graphics_pipeline()", "failed to create pipeline layout\n");
	}
	
	VkGraphicsPipelineCreateInfo pipeline_create_info = {};
	pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.stageCount          = 2;
	pipeline_create_info.pStages             = shader_stages;
	pipeline_create_info.pVertexInputState   = &vertex_input_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly;
	pipeline_create_info.pViewportState      = &viewport_state;
	pipeline_create_info.pRasterizationState = &rasterizer;
	pipeline_create_info.pMultisampleState   = &multisampling;
	pipeline_create_info.pDepthStencilState  = &depth_stencil;         // Optional
	pipeline_create_info.pColorBlendState    = &color_blending;
	pipeline_create_info.pDynamicState       = &dynamic_state;
	pipeline_create_info.layout              = info->pipeline_layout;
	pipeline_create_info.renderPass          = info->render_pass;
	pipeline_create_info.subpass             = 0;
	pipeline_create_info.basePipelineHandle  = VK_NULL_HANDLE;        // Optional
	pipeline_create_info.basePipelineIndex   = -1;                    // Optional

	if (vkCreateGraphicsPipelines(info->device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &info->graphics_pipeline) != VK_SUCCESS) {
		logprint("vulkan_create_graphics_pipeline()", "failed to create graphics pipelines\n");
	}

	vkDestroyShaderModule(info->device, vert_shader_module, nullptr);
	vkDestroyShaderModule(info->device, frag_shader_module, nullptr);
}

internal void
vulkan_create_frame_buffers(Vulkan_Info *info) {
	info->swap_chain_framebuffers.resize(info->swap_chain_image_views.get_size());
	for (u32 i = 0; i < info->swap_chain_image_views.get_size(); i++) {
		VkImageView attachments[] = {
			info->swap_chain_image_views[i],
			info->depth_image_view
		};
	
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    framebuffer_info.renderPass      = info->render_pass;
	    framebuffer_info.attachmentCount = ARRAY_COUNT(attachments);
	    framebuffer_info.pAttachments    = attachments;
	    framebuffer_info.width           = info->swap_chain_extent.width;
	    framebuffer_info.height          = info->swap_chain_extent.height;
	    framebuffer_info.layers          = 1;

		if (vkCreateFramebuffer(info->device, &framebuffer_info, nullptr, &info->swap_chain_framebuffers[i]) != VK_SUCCESS) {
			logprint("vulkan_create_frame_buffers()", "failed to create framebuffer\n");
		}
	}
}

internal void
vulkan_create_command_pool(Vulkan_Info *info) {
	Vulkan_Queue_Family_Indices queue_family_indices = vulkan_find_queue_families(info->physical_device, info->surface);
	
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family;

	if (vkCreateCommandPool(info->device, &pool_info, nullptr, &info->command_pool) != VK_SUCCESS) {
		logprint("vulkan_create_command_pool()", "failed to create command pool\n");
	}
}

internal void
vulkan_create_command_buffers(Vulkan_Info *info) {
	info->command_buffers.resize(info->MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = info->command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = info->MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(info->device, &alloc_info, (VkCommandBuffer*)info->command_buffers.get_data()) != VK_SUCCESS) {
		logprint("vulkan_create_command_buffer()", "failed to allocate command buffers\n");
	}
}

internal void
vulkan_create_sync_objects(Vulkan_Info *info) {
	info->image_available_semaphore.resize(info->MAX_FRAMES_IN_FLIGHT);
	info->render_finished_semaphore.resize(info->MAX_FRAMES_IN_FLIGHT);
	//info->in_flight_fence.resize(info->MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (u32 i = 0; i < info->MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(info->device, &semaphore_info, nullptr, &info->image_available_semaphore[i]) != VK_SUCCESS ||
	    	vkCreateSemaphore(info->device, &semaphore_info, nullptr, &info->render_finished_semaphore[i]) != VK_SUCCESS ||
	    	vkCreateFence    (info->device, &fence_info,     nullptr, &info->in_flight_fence[i]          ) != VK_SUCCESS) {
	    	logprint("vulkan_create_sync_objects()", "failed to create semaphores\n");
		}
	}
}

internal u32
vulkan_find_memory_type(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	logprint("vulkan_find_memory_type()", "failed to find suitable memory type\n");
	return 0;
}

internal void
vulkan_create_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory) {
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
		logprint("vulkan_create_buffer()", "failed to create buffer\n");
		return;
	}

	VkMemoryRequirements memory_requirements = {};
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
	
	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = vulkan_find_memory_type(physical_device, memory_requirements.memoryTypeBits, properties);
	
	if (vkAllocateMemory(device, &allocate_info, nullptr, &buffer_memory) != VK_SUCCESS) {
		logprint("vulkan_create_buffer()", "failed to allocate buffer memory\n");
		return;
	}

	vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

internal VkCommandBuffer
vulkan_begin_single_time_commands(VkDevice device, VkCommandPool command_pool) {
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = command_pool;
    allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

internal void
vulkan_end_single_time_commands(VkCommandBuffer command_buffer, VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);

	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

internal void
vulkan_copy_buffer(Vulkan_Info *info, VkBuffer src_buffer, VkBuffer dest_buffer, VkDeviceSize size, u32 src_offset, u32 dest_offset) {
	VkCommandBuffer command_buffer = vulkan_begin_single_time_commands(info->device, info->command_pool);

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = src_offset;  // Optional
	copy_region.dstOffset = dest_offset; // Optional
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, src_buffer, dest_buffer, 1, &copy_region);
		
	vulkan_end_single_time_commands(command_buffer, info->device, info->command_pool, info->graphics_queue);
}

// return the offset to the memory set in the buffer
internal u32
vulkan_update_buffer(Vulkan_Info *info, VkBuffer *buffer, VkDeviceMemory *memory, void *in_data, u32 in_data_size) {
	VkDeviceSize buffer_size = in_data_size;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	
	vulkan_create_buffer(info->device, 
						 info->physical_device,
						 buffer_size, 
						 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						 staging_buffer,
						 staging_buffer_memory);

	void *data;
	vkMapMemory(info->device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, in_data, buffer_size);
	vkUnmapMemory(info->device, staging_buffer_memory);

	vulkan_copy_buffer(info, staging_buffer, *buffer, buffer_size, 0, info->combined_buffer_offset);
	
	vkDestroyBuffer(info->device, staging_buffer, nullptr);
	vkFreeMemory(info->device, staging_buffer_memory, nullptr);

    u32 return_offset = info->combined_buffer_offset;
    info->combined_buffer_offset += in_data_size;
    return return_offset;
}

internal void
vulkan_create_descriptor_set_layout(Vulkan_Info *info) {
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr; // Optional
	
	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = { ubo_layout_binding, sampler_layout_binding };	

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = ARRAY_COUNT(bindings);
	layout_info.pBindings = bindings;
	
	if (vkCreateDescriptorSetLayout(info->device, &layout_info, nullptr, &info->descriptor_set_layout) != VK_SUCCESS) {
		logprint("vulkan_create_descriptor_set_layout()", "failed to create descriptor set layout\n");
	}
}

internal void
vulkan_create_descriptor_pool(Vulkan_Info *info) {
	VkDescriptorPoolSize pool_sizes[2] = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = info->MAX_FRAMES_IN_FLIGHT;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = info->MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes = pool_sizes;
	pool_info.maxSets = info->MAX_FRAMES_IN_FLIGHT;

	if (vkCreateDescriptorPool(info->device, &pool_info, nullptr, &info->descriptor_pool) != VK_SUCCESS) {
		logprint("vulkan_crate_descriptor_pool()", "failed to create descriptor pool\n");
	}
}

internal VkDeviceSize
vulkan_get_alignment(VkDeviceSize in, u32 alignment) {
	while(in % alignment != 0) {
		in++;
	}
	return in;
}

internal void
vulkan_create_descriptor_sets(Vulkan_Info *info) {
	Arr<VkDescriptorSetLayout> layouts;
	layouts.resize(info->MAX_FRAMES_IN_FLIGHT);
	for (u32 i = 0; i < info->MAX_FRAMES_IN_FLIGHT; i++) {
		layouts[i] = info->descriptor_set_layout;
	}

	VkDescriptorSetAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocate_info.descriptorPool = info->descriptor_pool;
	allocate_info.descriptorSetCount = (u32)info->MAX_FRAMES_IN_FLIGHT;
	allocate_info.pSetLayouts = (VkDescriptorSetLayout*)layouts.get_data();

	info->descriptor_sets.resize(info->MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(info->device, &allocate_info, (VkDescriptorSet*)info->descriptor_sets.get_data()) != VK_SUCCESS) {
		logprint("vulkan_create_descriptor_sets()", "failed to allocate descriptor sets\n");
	}

	for (u32 i = 0; i < info->MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = info->combined_buffer;
        buffer_info.offset = info->uniforms_offset[i];
        buffer_info.range = info->uniform_size;

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = info->texture_image_view;
        image_info.sampler = info->texture_sampler;

        VkWriteDescriptorSet descriptor_writes[2] = {};
        descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[0].dstSet = info->descriptor_sets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buffer_info;

        descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[1].dstSet = info->descriptor_sets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pImageInfo = &image_info;

        vkUpdateDescriptorSets(info->device, ARRAY_COUNT(descriptor_writes), descriptor_writes, 0, nullptr);
	}
}
/*
internal void
vulkan_setup_uniform_buffers(Vulkan_Info *info) {
	VkDeviceSize buffer_size = VK_WHOLE_SIZE;
	VkDeviceSize offset = info->uniforms_offset[0];
	vkMapMemory(info->device, info->combined_buffer_memory, offset, buffer_size, 0, &info->uniform_buffers_mapped);
}
*/
internal void
vulkan_create_image(VkDevice device, VkPhysicalDevice physical_device, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory) {
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &image_info, nullptr, &image) != VK_SUCCESS) {
		logprint("vulkan_create_image()", "failed to create image\n");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(device, image, &memory_requirements);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = memory_requirements.size;
	allocate_info.memoryTypeIndex = vulkan_find_memory_type(physical_device, memory_requirements.memoryTypeBits, properties);
	
	if (vkAllocateMemory(device, &allocate_info, nullptr, &image_memory)) {
		logprint("vulkan_create_image()", "failed to allocate image memory\n");
	}

	vkBindImageMemory(device, image, image_memory, 0);
}

internal bool8
vulkan_has_stencil_component(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

internal void
vulkan_transition_image_layout(Vulkan_Info *info, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBuffer command_buffer = vulkan_begin_single_time_commands(info->device, info->command_pool);
	
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0; // TODO
	barrier.dstAccessMask = 0; // TODO

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
	    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	    if (vulkan_has_stencil_component(format)) {
	        barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	    }
	} else {
	    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
	    barrier.srcAccessMask = 0;
	    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
	    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
	    barrier.srcAccessMask = 0;
	    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
	    logprint("vulkan_create_texture_image()", "unsupported layout transition\n");
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vulkan_end_single_time_commands(command_buffer, info->device, info->command_pool, info->graphics_queue);
}

internal void
vulkan_create_depth_resources(Vulkan_Info *info) {
	VkFormat depth_format = vulkan_find_depth_format(info->physical_device);
	vulkan_create_image(info->device, info->physical_device, info->swap_chain_extent.width, info->swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, info->depth_image, info->depth_image_memory);
	info->depth_image_view = vulkan_create_image_view(info->device, info->depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
	vulkan_transition_image_layout(info, info->depth_image, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);	
}

internal void
vulkan_cleanup_swap_chain(Vulkan_Info *info) {
	for (u32 i = 0; i < info->swap_chain_framebuffers.get_size(); i++) {
		vkDestroyFramebuffer(info->device, info->swap_chain_framebuffers[i], nullptr);
	}

	for (u32 i = 0; i < info->swap_chain_image_views.get_size(); i++) {
		vkDestroyImageView(info->device, info->swap_chain_image_views[i], nullptr);
	}

	vkDestroySwapchainKHR(info->device, info->swap_chains[0], nullptr);
}

internal void
vulkan_recreate_swap_chain(Vulkan_Info *info) {
	if (info->window_width == 0 || info->window_height == 0) {
		/*SDL_Event event;
		SDL_WindowEvent *window_event = &event.window;
		info->window_width = window_event->data1;
		info->window_height = window_event->data2;
		SDL_WaitEvent(&event);
		int i = 0;*/
	}

	vkDeviceWaitIdle(info->device);

	vulkan_cleanup_swap_chain(info);

	vkDestroyImageView(info->device, info->depth_image_view, nullptr);
    vkDestroyImage(info->device, info->depth_image, nullptr);
    vkFreeMemory(info->device, info->depth_image_memory, nullptr);

	vulkan_create_swap_chain(info);
	vulkan_create_image_views(info);
	vulkan_create_depth_resources(info);
	vulkan_create_frame_buffers(info);
}

internal void
vulkan_cleanup(Vulkan_Info *info) {
	vulkan_cleanup_swap_chain(info);
	
	// Depth buffer
	vkDestroyImageView(info->device, info->depth_image_view, nullptr);
    vkDestroyImage(info->device, info->depth_image, nullptr);
    vkFreeMemory(info->device, info->depth_image_memory, nullptr);

	// Texture Image
	vkDestroySampler(info->device, info->texture_sampler, nullptr);
	vkDestroyImageView(info->device, info->texture_image_view, nullptr);
	vkDestroyImage(info->device, info->texture_image, nullptr);
    vkFreeMemory(info->device, info->texture_image_memory, nullptr);

	// Uniform buffer
	for (u32 i = 0; i < info->MAX_FRAMES_IN_FLIGHT; i++) {
		//vkDestroyBuffer(info->device, info->uniform_buffers[i], nullptr);
		//vkFreeMemory(info->device, info->uniform_buffers_memory[i], nullptr);
	}

	vkDestroyDescriptorPool(info->device, info->descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(info->device, info->descriptor_set_layout, nullptr);

	vkDestroyBuffer(info->device, info->combined_buffer, nullptr);
	vkFreeMemory(info->device, info->combined_buffer_memory, nullptr);
	
	vkDestroyPipeline(info->device, info->graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(info->device, info->pipeline_layout, nullptr);
	
	vkDestroyRenderPass(info->device, info->render_pass, nullptr);

	for (u32 i = 0; i < info->MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(info->device, info->image_available_semaphore[i], nullptr);
	    vkDestroySemaphore(info->device, info->render_finished_semaphore[i], nullptr);
	    vkDestroyFence(info->device, info->in_flight_fence[i], nullptr);
	}
	
	vkDestroyCommandPool(info->device, info->command_pool, nullptr);

	vkDestroyDevice(info->device, nullptr);

	vkDestroySurfaceKHR(info->instance, info->surface, nullptr);
	
	if (info->validation_layers.enable)
		vulkan_destroy_debug_utils_messenger_ext(info->instance, info->debug_messenger, nullptr);

	vkDestroyInstance(info->instance, nullptr);
}

internal void
vulkan_copy_buffer_to_image(Vulkan_Info *info, VkBuffer buffer, VkImage image, u32 width, u32 height) {
	VkCommandBuffer command_buffer = vulkan_begin_single_time_commands(info->device, info->command_pool);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	
	vulkan_end_single_time_commands(command_buffer, info->device, info->command_pool, info->graphics_queue);
}

// TODO: Make it so that images can be created asynchronously (vulkan-tutorial = Texture mapping/Images)

internal void
vulkan_create_texture_image(Vulkan_Info *info, Bitmap *bitmap) {
    VkDeviceSize image_size = bitmap->width * bitmap->height * bitmap->channels;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    vulkan_create_buffer(info->device, info->physical_device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	vkMapMemory(info->device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, bitmap->memory, image_size);
	vkUnmapMemory(info->device, staging_buffer_memory);

	vulkan_create_image(info->device, info->physical_device, bitmap->width, bitmap->height, info->texture_image_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, info->texture_image, info->texture_image_memory);

	vulkan_transition_image_layout(info, info->texture_image, info->texture_image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vulkan_copy_buffer_to_image(info, staging_buffer, info->texture_image, (u32)bitmap->width, (u32)bitmap->height);
    vulkan_transition_image_layout(info, info->texture_image, info->texture_image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(info->device, staging_buffer, nullptr);
    vkFreeMemory(info->device, staging_buffer_memory, nullptr);
}

internal void
vulkan_create_texture_image_view(Vulkan_Info *info) {
	info->texture_image_view = vulkan_create_image_view(info->device, info->texture_image, info->texture_image_format, VK_IMAGE_ASPECT_COLOR_BIT);
}

internal void
vulkan_create_texture_sampler(Vulkan_Info *info) {
	VkPhysicalDeviceProperties properties = {};
	vkGetPhysicalDeviceProperties(info->physical_device, &properties);
	
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	if (vkCreateSampler(info->device, &sampler_info, nullptr, &info->texture_sampler) != VK_SUCCESS) {
        logprint("vulkan_create_texture_sampler()", "failed to create texture sampler\n");
    }
}

internal void
vulkan_init_presentation_settings(Vulkan_Info *info) {

	// Start frame
	info->begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	info->begin_info.flags = 0;				   // Optional
	info->begin_info.pInheritanceInfo = nullptr; // Optional

	info->clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	info->clear_values[1].depthStencil = {1.0f, 0};

	info->render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	info->render_pass_info.renderPass = info->render_pass;
	info->render_pass_info.framebuffer = info->swap_chain_framebuffers[info->image_index];
	info->render_pass_info.renderArea.offset = {0, 0};
	info->render_pass_info.renderArea.extent = info->swap_chain_extent;
	info->render_pass_info.clearValueCount = ARRAY_COUNT(info->clear_values);
	info->render_pass_info.pClearValues = info->clear_values;

	info->scissor.offset = {0, 0};
	info->scissor.extent = info->swap_chain_extent;

	info->viewport.x = 0.0f;
	info->viewport.y = 0.0f;
	info->viewport.width = static_cast<float>(info->swap_chain_extent.width);
	info->viewport.height = static_cast<float>(info->swap_chain_extent.height);
	info->viewport.minDepth = 0.0f;
	info->viewport.maxDepth = 1.0f;

	// End of frame
	info->submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info->submit_info.waitSemaphoreCount = 1;
	info->submit_info.pWaitSemaphores = &info->image_available_semaphore[info->current_frame];
	info->submit_info.pWaitDstStageMask = info->wait_stages;
	info->submit_info.commandBufferCount = 1;
	info->submit_info.pCommandBuffers = &info->command_buffers[info->current_frame];	
	info->submit_info.signalSemaphoreCount = 1;
	info->submit_info.pSignalSemaphores = &info->render_finished_semaphore[info->current_frame];

	info->present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info->present_info.waitSemaphoreCount = 1;
	info->present_info.pWaitSemaphores = &info->render_finished_semaphore[info->current_frame];
	info->present_info.swapchainCount = ARRAY_COUNT(info->swap_chains);
	info->present_info.pSwapchains = info->swap_chains;
	info->present_info.pImageIndices = &info->image_index;
	info->present_info.pResults = nullptr; // Optional
}

// Requires updated current_frame and image_index
inline void
vulkan_update_presentation_settings(Vulkan_Info *info) {

	// Start of frame
	info->render_pass_info.framebuffer = info->swap_chain_framebuffers[info->image_index];

	// End of frame
	info->submit_info.pWaitSemaphores = &info->image_available_semaphore[info->current_frame];
	info->submit_info.pCommandBuffers = &info->command_buffers[info->current_frame];	
	info->submit_info.pSignalSemaphores = &info->render_finished_semaphore[info->current_frame];

	info->present_info.pWaitSemaphores = &info->render_finished_semaphore[info->current_frame];
	info->present_info.pImageIndices = &info->image_index;
}

//
// Render
//

void vulkan_clear_color(Vector4 color) {
	vulkan_info.clear_values[0].color = {{color.r, color.g, color.b, color.a}};
}

void vulkan_start_frame() {
	vulkan_info.command_buffer = vulkan_info.command_buffers[vulkan_info.current_frame];

	// Waiting for the previous frame
	vkWaitForFences(vulkan_info.device, 1, &vulkan_info.in_flight_fence[vulkan_info.current_frame], VK_TRUE, UINT64_MAX);
	VkResult result = vkAcquireNextImageKHR(vulkan_info.device,
                                            vulkan_info.swap_chains[0],
                                            UINT64_MAX,
                                            vulkan_info.image_available_semaphore[vulkan_info.current_frame],
                                            VK_NULL_HANDLE,
                                            &vulkan_info.image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
         vulkan_recreate_swap_chain(&vulkan_info);
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		logprint("vulkan_draw_frame()", "failed to acquire swap chain");
	}

	vulkan_update_presentation_settings(&vulkan_info);

	vkResetFences(vulkan_info.device, 1, &vulkan_info.in_flight_fence[vulkan_info.current_frame]);
	vkResetCommandBuffer(vulkan_info.command_buffer, 0);
	
	if (vkBeginCommandBuffer(vulkan_info.command_buffer, &vulkan_info.begin_info) != VK_SUCCESS) {
		logprint("vulkan_record_command_buffer()", "failed to begin recording command buffer\n");
	}	

	vkCmdBeginRenderPass(vulkan_info.command_buffer, &vulkan_info.render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(vulkan_info.command_buffer, 0, 1, &vulkan_info.viewport);
	vkCmdSetScissor(vulkan_info.command_buffer, 0, 1, &vulkan_info.scissor);
	vkCmdBindPipeline(vulkan_info.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkan_info.graphics_pipeline);
}

void vulkan_end_frame() {
	vkCmdEndRenderPass(vulkan_info.command_buffer);

	if (vkEndCommandBuffer(vulkan_info.command_buffer) != VK_SUCCESS) {
		logprint("vulkan_record_command_buffer()", "failed to record command buffer\n");
	}

	if (vkQueueSubmit(vulkan_info.graphics_queue, 1, &vulkan_info.submit_info, vulkan_info.in_flight_fence[vulkan_info.current_frame]) != VK_SUCCESS) {
		logprint("vulkan_draw_frame()", "failed to submit draw command buffer\n");
	}
	
	VkResult result = vkQueuePresentKHR(vulkan_info.present_queue, &vulkan_info.present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulkan_info.framebuffer_resized) {
		vulkan_info.framebuffer_resized = false;
		vulkan_recreate_swap_chain(&vulkan_info);
		return;
	} else if (result != VK_SUCCESS) {
		logprint("vulkan_draw_frame()", "failed to acquire swap chain");
	}

	vulkan_info.current_frame = (vulkan_info.current_frame + 1) % vulkan_info.MAX_FRAMES_IN_FLIGHT;
}

void vulkan_init_mesh(Mesh *mesh) {
    Vulkan_Mesh *vulkan_mesh = (Vulkan_Mesh*)platform_malloc(sizeof(Vulkan_Mesh));

    u32 vertices_size = mesh->vertices_count * sizeof(Vertex);
    u32 indices_size = mesh->indices_count * sizeof(u32);   
    u32 buffer_size = vertices_size + indices_size;

    void *memory = platform_malloc(buffer_size);

    memcpy(memory, (void*)mesh->vertices, vertices_size);
    memcpy((char*)memory + vertices_size, (void*)mesh->indices, indices_size);

    vulkan_mesh->vertices_offset = vulkan_update_buffer(&vulkan_info, &vulkan_info.combined_buffer, &vulkan_info.combined_buffer_memory, memory, buffer_size);
    vulkan_mesh->indices_offset = vulkan_mesh->vertices_offset + vertices_size;
    
    platform_free(memory);
    mesh->gpu_info = (void*)vulkan_mesh;
}

void vulkan_draw_mesh(Mesh *mesh) {
    Vulkan_Mesh *vulkan_mesh = (Vulkan_Mesh*)mesh->gpu_info;
    VkDeviceSize offsets[] = { vulkan_mesh->vertices_offset };
    vkCmdBindVertexBuffers(vulkan_info.command_buffer, 0, 1, &vulkan_info.combined_buffer, offsets);
    vkCmdBindIndexBuffer(vulkan_info.command_buffer, vulkan_info.combined_buffer, vulkan_mesh->indices_offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vulkan_info.command_buffer, mesh->indices_count, 1, 0, 0, 0);
}

internal void
vulkan_update_uniform_buffer_object(Uniform_Buffer_Object ubo, Matrices matrices) {
    u32 memory_size = (u32)vulkan_info.uniforms_offset[1] + sizeof(Matrices);
    void *memory = platform_malloc(memory_size);
    memcpy((char*)memory + vulkan_info.uniforms_offset[0], (void*)&matrices, sizeof(Matrices));
    memcpy((char*)memory + vulkan_info.uniforms_offset[1], (void*)&matrices, sizeof(Matrices));
    vulkan_update_buffer(&vulkan_info, &vulkan_info.combined_buffer, &vulkan_info.combined_buffer_memory, memory, memory_size);
    platform_free(memory);
}
