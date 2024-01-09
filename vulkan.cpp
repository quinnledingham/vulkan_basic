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

	if (vulkan_info->validation_layers.enable) {
		create_info.enabledLayerCount = vulkan_info->validation_layers.count;
		create_info.ppEnabledLayerNames = vulkan_info->validation_layers.data;
	} else {
		create_info.enabledLayerCount = 0;
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

	return indices;
}

internal bool8
vulkan_check_device_extension_support(VkPhysicalDevice device) {
	u32 available_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &available_count, nullptr);

    VkExtensionProperties *available_extensions = ARRAY_MALLOC(VkExtensionProperties, available_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &available_count, available_extensions);

    bool8 all_extensions_found = true;
    for (u32 required_index = 0; required_index < ARRAY_COUNT(device_extensions); required_index++) {
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
vulkan_is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
	/*
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceProperties(device, &device_properties);
	vkGetPhysicalDeviceFeatures(device, &device_features);

	return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader;
	*/

	Vulkan_Queue_Family_Indices indices = vulkan_find_queue_families(device, surface);

	bool8 extensions_supported = vulkan_check_device_extension_support(device);
	bool8 swap_chain_adequate = false;
	if (extensions_supported) {
		Vulkan_Swap_Chain_Support_Details swap_chain_support = vulkan_query_swap_chain_support(device, surface);
		swap_chain_adequate = swap_chain_support.formats_count && swap_chain_support.present_modes_count;
	}

	return indices.graphics_family_found && extensions_supported && swap_chain_adequate;
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
		if (vulkan_is_device_suitable(devices[device_index], info->surface)) {
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

	// No features requested
	VkPhysicalDeviceFeatures device_features = {};

	// Set up device
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.queueCreateInfoCount = indices.unique_families;
	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = ARRAY_COUNT(device_extensions);
	create_info.ppEnabledExtensionNames = (const char *const *)device_extensions;

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
}

/*
internal void
vulkan_cleanup() {
	vkDestroyDevice(device, nullptr);
}
*/
