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

internal VkSurfaceFormatKHR
vulkan_choose_swap_surface_format(VkSurfaceFormatKHR *formats, u32 count) {
	for (u32 i = 0; i < count; i++) {
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return formats[i];
		}
	}

	return formats[0];
}

internal VkPresentModeKHR
vulkan_choose_swap_present_mode(VkPresentModeKHR *modes, u32 count) {
	for (u32 i = 0; i < count; i++) {
		if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return modes[i];
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
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

	if (vkCreateSwapchainKHR(info->device, &create_info, nullptr, &info->swap_chain)) {
		logprint("vulkan_create_swap_chain()", "failed to create swap chain\n");
	}

	vkGetSwapchainImagesKHR(info->device, info->swap_chain, &info->swap_chain_images_count, nullptr);
	if (info->swap_chain_images != 0)
		platform_free(info->swap_chain_images);
	info->swap_chain_images = ARRAY_MALLOC(VkImage, info->swap_chain_images_count);
	vkGetSwapchainImagesKHR(info->device, info->swap_chain, &info->swap_chain_images_count, info->swap_chain_images);

	info->swap_chain_image_format = surface_format.format;
	info->swap_chain_extent = extent;
}

internal void
vulkan_create_image_views(Vulkan_Info *info) {
	arr_resize(&info->swap_chain_image_views, info->swap_chain_images_count);
	for (u32 i = 0; i < info->swap_chain_images_count; i++) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = info->swap_chain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = info->swap_chain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(info->device, &create_info, nullptr, &info->swap_chain_image_views[i]) != VK_SUCCESS) {
			logprint("vulkan_create_image_views()", "failed to create image view\n");
		} else {
			info->swap_chain_image_views.count++;
		}
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

internal void
vulkan_create_render_pass(Vulkan_Info *info) {
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = info->swap_chain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	
	// Subpass dependencies
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	
	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;


	if (vkCreateRenderPass(info->device, &render_pass_info, nullptr, &info->render_pass) != VK_SUCCESS) {
		logprint("vulkan_create_render_pass()", "failed to create render pass\n");
	}
}

internal void
vulkan_create_graphics_pipeline(Vulkan_Info *info) {
	File vert = load_file("../vert.spv");
	File frag = load_file("../frag.spv");

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

	// Vertex Input description
	VkVertexInputBindingDescription binding_description = {};
	binding_description.binding = 0;
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	
	VkVertexInputAttributeDescription attribute_descriptions[2] = {};
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0;
	attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attribute_descriptions[0].offset = offsetof(Vertex, pos);
	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = offsetof(Vertex, color);	

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description; // Optional
	vertex_input_info.vertexAttributeDescriptionCount = 2;
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions; // Optional

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float32)info->swap_chain_extent.width;
	viewport.height = (float32)info->swap_chain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = info->swap_chain_extent;

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
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;      // Optional
	rasterizer.depthBiasClamp = 0.0f;               // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f;         // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading      = 1.0f;                  // Optional
	multisampling.pSampleMask           = nullptr;               // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;              // Optional
	multisampling.alphaToOneEnable      = VK_FALSE; 			 // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable         = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending{};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;           // Optional
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f;             // Optional
	color_blending.blendConstants[1] = 0.0f;             // Optional
	color_blending.blendConstants[2] = 0.0f;             // Optional
	color_blending.blendConstants[3] = 0.0f;             // Optional

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount         = 0;       // Optional
	pipeline_layout_info.pSetLayouts            = nullptr; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0;       // Optional
	pipeline_layout_info.pPushConstantRanges    = nullptr; // Optional
	
	if (vkCreatePipelineLayout(info->device, &pipeline_layout_info, nullptr, &info->pipeline_layout) != VK_SUCCESS) {
		logprint("vulkan_create_graphics_pipeline()", "failed to create pipeline layout\n");
	}
	
	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount          = 2;
	pipeline_info.pStages             = shader_stages;
	pipeline_info.pVertexInputState   = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState      = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState   = &multisampling;
	pipeline_info.pDepthStencilState  = nullptr;               // Optional
	pipeline_info.pColorBlendState    = &color_blending;
	pipeline_info.pDynamicState       = &dynamic_state;
	pipeline_info.layout              = info->pipeline_layout;
	pipeline_info.renderPass          = info->render_pass;
	pipeline_info.subpass             = 0;
	pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;        // Optional
	pipeline_info.basePipelineIndex   = -1;                    // Optional

	if (vkCreateGraphicsPipelines(info->device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &info->graphics_pipeline) != VK_SUCCESS) {
		logprint("vulkan_create_graphics_pipeline()", "failed to create graphics pipelines\n");
	}

	vkDestroyShaderModule(info->device, vert_shader_module, nullptr);
	vkDestroyShaderModule(info->device, frag_shader_module, nullptr);
}

internal void
vulkan_create_frame_buffers(Vulkan_Info *info) {
	arr_resize(&info->swap_chain_framebuffers, info->swap_chain_image_views.count);
	for (u32 i = 0; i < info->swap_chain_image_views.count; i++) {
		VkImageView attachments[] = {
			info->swap_chain_image_views[i]
		};
	
		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	    framebuffer_info.renderPass      = info->render_pass;
	    framebuffer_info.attachmentCount = 1;
	    framebuffer_info.pAttachments    = attachments;
	    framebuffer_info.width           = info->swap_chain_extent.width;
	    framebuffer_info.height          = info->swap_chain_extent.height;
	    framebuffer_info.layers          = 1;

		if (vkCreateFramebuffer(info->device, &framebuffer_info, nullptr, &info->swap_chain_framebuffers[i]) != VK_SUCCESS) {
			logprint("vulkan_create_frame_buffers()", "failed to create framebuffer\n");
		} else {
			info->swap_chain_framebuffers.count++;
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
	arr_resize(&info->command_buffers, info->MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = info->command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = info->MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(info->device, &alloc_info, &info->command_buffers.get_data()) != VK_SUCCESS) {
		logprint("vulkan_create_command_buffer()", "failed to allocate command buffers\n");
	}
}

internal void
vulkan_record_command_buffer(Vulkan_Info *info, VkCommandBuffer command_buffer, u32 image_index) {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;				   // Optional
	begin_info.pInheritanceInfo = nullptr; // Optional
	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
		logprint("vulkan_record_command_buffer()", "failed to begin recording command buffer\n");
	}	

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = info->render_pass;
	render_pass_info.framebuffer = info->swap_chain_framebuffers[image_index];
	render_pass_info.renderArea.offset = {0, 0};
	render_pass_info.renderArea.extent = info->swap_chain_extent;

	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, info->graphics_pipeline);
	
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(info->swap_chain_extent.width);
	viewport.height = static_cast<float>(info->swap_chain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = info->swap_chain_extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	VkBuffer vertex_buffers[] = { info->vertex_buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

	vkCmdDraw(command_buffer, ARRAY_COUNT(vertices), 1, 0, 0);

	vkCmdEndRenderPass(command_buffer);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
		logprint("vulkan_record_command_buffer()", "failed to record command buffer\n");
	}
}

internal void
vulkan_create_sync_objects(Vulkan_Info *info) {
	arr_resize(&info->image_available_semaphore, info->MAX_FRAMES_IN_FLIGHT);
	arr_resize(&info->render_finished_semaphore, info->MAX_FRAMES_IN_FLIGHT);
	arr_resize(&info->in_flight_fence, info->MAX_FRAMES_IN_FLIGHT);

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
vulkan_create_vertex_buffer(Vulkan_Info *info) {
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = sizeof(vertices[0]) * ARRAY_COUNT(vertices);
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(info->device, &buffer_info, nullptr, &info->vertex_buffer) != VK_SUCCESS) {
		logprint("vulkan_create_vertex_buffer()", "failed to create vertex buffer\n");
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(info->device, info->vertex_buffer, &memory_requirements);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = vulkan_find_memory_type(info->physical_device, memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(info->device, &alloc_info, nullptr, &info->vertex_buffer_memory) != VK_SUCCESS) {
		logprint("vulkan_create_vertex_buffer()", "failed to allocate vertex buffer memory\n");
	}

	vkBindBufferMemory(info->device, info->vertex_buffer, info->vertex_buffer_memory, 0);

	void *data;
	vkMapMemory(info->device, info->vertex_buffer_memory, 0, buffer_info.size, 0, &data);
	memcpy(data, vertices, buffer_info.size);
	vkUnmapMemory(info->device, info->vertex_buffer_memory);
}

internal void
vulkan_cleanup_swap_chain(Vulkan_Info *info) {
	for (u32 i = 0; i < info->swap_chain_framebuffers.count; i++) {
		vkDestroyFramebuffer(info->device, info->swap_chain_framebuffers[i], nullptr);
	}

	for (u32 i = 0; i < info->swap_chain_image_views.count; i++) {
		vkDestroyImageView(info->device, info->swap_chain_image_views[i], nullptr);
	}

	vkDestroySwapchainKHR(info->device, info->swap_chain, nullptr);
}

internal void
vulkan_recreate_swap_chain(Vulkan_Info *info) {
	if (info->window_width == 0 || info->window_height == 0) {
		SDL_Event event;
		SDL_WindowEvent *window_event = &event.window;
		info->window_width = window_event->data1;
		info->window_height = window_event->data2;
		SDL_WaitEvent(&event);
		int i = 0;
	}

	vkDeviceWaitIdle(info->device);

	vulkan_cleanup_swap_chain(info);

	vulkan_create_swap_chain(info);
	vulkan_create_image_views(info);
	vulkan_create_frame_buffers(info);
}

internal void
vulkan_cleanup(Vulkan_Info *info) {
	vulkan_cleanup_swap_chain(info);

	// Vertex buffer
	vkDestroyBuffer(info->device, info->vertex_buffer, nullptr);
	vkFreeMemory(info->device, info->vertex_buffer_memory, nullptr);

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
	vkDestroyInstance(info->instance, nullptr);
}

internal void
vulkan_draw_frame(Vulkan_Info *info) {
	if (info->minimized)
		return;

	// Waiting for the previous frame
	vkWaitForFences(info->device, 1, &info->in_flight_fence[info->current_frame], VK_TRUE, UINT64_MAX);
	
	u32 image_index;
	VkResult result = vkAcquireNextImageKHR(info->device, info->swap_chain, UINT64_MAX, info->image_available_semaphore[info->current_frame], VK_NULL_HANDLE, &image_index);
	
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		vulkan_recreate_swap_chain(info);
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		logprint("vulkan_draw_frame()", "failed to acquire swap chain");
	}

	vkResetFences(info->device, 1, &info->in_flight_fence[info->current_frame]);

	vkResetCommandBuffer(info->command_buffers[info->current_frame], 0);
	vulkan_record_command_buffer(info, info->command_buffers[info->current_frame], image_index);

	// Submitting the command buffer
	VkSemaphore wait_semaphores[] = { info->image_available_semaphore[info->current_frame] };
	VkSemaphore signal_semaphores[] = { info->render_finished_semaphore[info->current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &info->command_buffers[info->current_frame];	
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	if (vkQueueSubmit(info->graphics_queue, 1, &submit_info, info->in_flight_fence[info->current_frame]) != VK_SUCCESS) {
		logprint("vulkan_draw_frame()", "failed to submit draw command buffer\n");
	}

	// Presentation
	VkSwapchainKHR swap_chains[] = { info->swap_chain};
	
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(info->present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || info->framebuffer_resized) {
		info->framebuffer_resized = false;
		vulkan_recreate_swap_chain(info);
		return;
	} else if (result != VK_SUCCESS) {
		logprint("vulkan_draw_frame()", "failed to acquire swap chain");
	}

	info->current_frame = (info->current_frame + 1) % info->MAX_FRAMES_IN_FLIGHT;
}