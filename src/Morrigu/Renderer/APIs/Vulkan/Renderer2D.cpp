#include "Renderer2D.h"

#include "Core/GLMIncludeHelper.h"
#include "Debug/Instrumentor.h"
#include "Renderer/APIs/Vulkan/Helper.h"
#include "Renderer/APIs/Vulkan/Shader.h"
#include "Renderer/APIs/Vulkan/VertexArray.h"

#include <ImGui/bindings/imgui_impl_vulkan.h>
#include <imgui.h>

#include <array>

namespace
{
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec2 texCoord;

		static inline std::initializer_list<MRG::BufferElement> layout = {{MRG::ShaderDataType::Float2, "a_pos"},
		                                                                  {MRG::ShaderDataType::Float2, "a_texCoord"}};
	};

	[[nodiscard]] MRG::Vulkan::QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice device, const VkSurfaceKHR surface)
	{
		MRG::Vulkan::QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;

			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete())
				break;
			++i;
		}

		return indices;
	}

	[[nodiscard]] VkSurfaceFormatKHR chooseSwapFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		for (const auto& format : formats) {
			if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return format;
		}

		// In case we don't find exactly what we are searching for, we just select the first one.
		// This is guaranteed to exist because of the isDeviceSuitable check beforehand
		// TODO: update this function to better select surface formats
		return formats[0];
	}

	[[nodiscard]] VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
	{
		// If possible, prefer triple buffering
		for (const auto& presentMode : presentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return presentMode;
		}

		// This mode is guaranteed to be present by the specs
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	[[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const MRG::WindowProperties* data)
	{
		if (capabilities.currentExtent.width != UINT32_MAX) {
			// We don't need to change anything in this case !
			return capabilities.currentExtent;
		}

		// Otherwise, we have to provide the best values possible
		VkExtent2D actualExtent{data->width, data->height};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

	[[nodiscard]] std::vector<VkImageView> createimageViews(const VkDevice device, const std::vector<VkImage>& images, VkFormat imageFormat)
	{
		std::vector<VkImageView> imageViews(images.size());
		for (std::size_t i = 0; i < images.size(); i++) {
			imageViews[i] = MRG::Vulkan::createImageView(device, images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		}

		return imageViews;
	}

	[[nodiscard]] MRG::Vulkan::SwapChain createSwapChain(const VkPhysicalDevice physicalDevice,
	                                                     const VkSurfaceKHR surface,
	                                                     const VkDevice device,
	                                                     const MRG::WindowProperties* data)
	{
		VkSwapchainKHR handle{};
		MRG::Vulkan::SwapChainSupportDetails SwapChainSupport = MRG::Vulkan::querySwapChainSupport(physicalDevice, surface);

		const auto surfaceFormat = chooseSwapFormat(SwapChainSupport.formats);
		const auto presentMode = chooseSwapPresentMode(SwapChainSupport.presentModes);
		const auto extent = chooseSwapExtent(SwapChainSupport.capabilities, data);

		auto imageCount = SwapChainSupport.capabilities.minImageCount + 1;
		if (SwapChainSupport.capabilities.maxImageCount > 0 && imageCount > SwapChainSupport.capabilities.maxImageCount)
			imageCount = SwapChainSupport.capabilities.maxImageCount;

		MRG::Vulkan::QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		uint32_t minImageCount = imageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		if (indices.graphicsFamily != indices.presentFamily) {
			// The queues are not the same!
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			// The queues are the same, no need to separated them!
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		createInfo.preTransform = SwapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		MRG_VKVALIDATE(vkCreateSwapchainKHR(device, &createInfo, nullptr, &handle), "failed to create swapChain!");

		vkGetSwapchainImagesKHR(device, handle, &imageCount, nullptr);
		std::vector<VkImage> images(imageCount);
		vkGetSwapchainImagesKHR(device, handle, &imageCount, images.data());

		const auto imageViews = createimageViews(device, images, surfaceFormat.format);

		return {handle, minImageCount, imageCount, images, surfaceFormat.format, extent, imageViews, {}, {}};
	}

	[[nodiscard]] VkDescriptorSetLayout createDescriptorSetLayout(const VkDevice device)
	{
		VkDescriptorSetLayout returnLayout;

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		MRG_VKVALIDATE(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &returnLayout), "failed to create descriptor set layout!");

		return returnLayout;
	}

	[[nodiscard]] VkPushConstantRange populatePushConstantsRanges()
	{
		VkPushConstantRange pushConstantsRange{};
		pushConstantsRange.offset = 0;
		pushConstantsRange.size = sizeof(MRG::Vulkan::PushConstants);
		pushConstantsRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		return pushConstantsRange;
	}

	[[nodiscard]] VkPipelineLayoutCreateInfo populatePipelineLayout(const VkDescriptorSetLayout* descriptorSetLayout,
	                                                                const VkPushConstantRange* pushConstantsRange)
	{
		VkPipelineLayoutCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineCreateInfo.setLayoutCount = 1;
		pipelineCreateInfo.pSetLayouts = descriptorSetLayout;
		pipelineCreateInfo.pushConstantRangeCount = 1;
		pipelineCreateInfo.pPushConstantRanges = pushConstantsRange;

		return pipelineCreateInfo;
	}

	VkFormat findSupportedFormat(const VkPhysicalDevice physicalDevice,
	                             const std::vector<VkFormat>& candidates,
	                             VkImageTiling tiling,
	                             VkFormatFeatureFlags features)
	{
		for (const auto& format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				return format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				return format;
		}

		MRG_ASSERT(false, "failed to find a supported format!");
		return VK_FORMAT_MAX_ENUM;
	}

	auto findDepthFormat(const VkPhysicalDevice physicalDevice)
	{
		return findSupportedFormat(physicalDevice,
		                           {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		                           VK_IMAGE_TILING_OPTIMAL,
		                           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	[[nodiscard]] VkRenderPass createRenderPass(const VkPhysicalDevice physicalDevice, VkDevice device, VkFormat swapChainFormat)
	{
		VkRenderPass returnRenderPass;

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat(physicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		MRG_VKVALIDATE(vkCreateRenderPass(device, &renderPassInfo, nullptr, &returnRenderPass), "failed to create render pass!")
		return returnRenderPass;
	}

	[[nodiscard]] VkPipeline createPipeline(const MRG::Vulkan::WindowProperties* data,
	                                        MRG::Ref<MRG::Vulkan::Shader> textureShader,
	                                        const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions,
	                                        const std::vector<VkVertexInputBindingDescription>& bindingDescriptions)
	{
		VkPipeline returnPipeline;

		// TODO: Add proper debug logging for selected rendering features
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = textureShader->m_vertexShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = textureShader->m_fragmentShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(data->swapChain.extent.width);
		viewport.height = static_cast<float>(data->swapChain.extent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = data->swapChain.extent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.colorWriteMask =
		  VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = data->pipeline.layout;
		pipelineInfo.renderPass = data->pipeline.renderPass;
		pipelineInfo.subpass = 0;

		MRG_VKVALIDATE(vkCreateGraphicsPipelines(data->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &returnPipeline),
		               "failed to create graphics pipeline!");

		return returnPipeline;
	}

	[[nodiscard]] std::vector<VkFramebuffer> createframeBuffers(const VkDevice device,
	                                                            const std::vector<VkImageView>& swapChainImagesViews,
	                                                            const VkImageView depthImageView,
	                                                            const VkRenderPass renderPass,
	                                                            const VkExtent2D swapChainExtent)
	{
		std::vector<VkFramebuffer> returnFrameBuffers(swapChainImagesViews.size());

		for (std::size_t i = 0; i < swapChainImagesViews.size(); ++i) {
			std::array<VkImageView, 2> attachments = {swapChainImagesViews[i], depthImageView};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			MRG_VKVALIDATE(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &returnFrameBuffers[i]), "failed to create framebuffer!");
		}

		return returnFrameBuffers;
	}

	[[nodiscard]] std::vector<MRG::Vulkan::Buffer> createUniformBuffers(const MRG::Vulkan::WindowProperties* data)
	{
		std::vector<MRG::Vulkan::Buffer> returnBuffers(data->swapChain.imageCount);

		VkDeviceSize bufferSize = sizeof(MRG::Vulkan::UniformBufferObject);

		for (std::size_t i = 0; i < data->swapChain.imageCount; ++i) {
			MRG::Vulkan::createBuffer(data->device,
			                          data->physicalDevice,
			                          bufferSize,
			                          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			                          returnBuffers[i]);
		}

		return returnBuffers;
	}

	[[nodiscard]] VkCommandPool createCommandPool(const VkDevice device, const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface)
	{
		VkCommandPool returnCommandPool;

		const auto queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		MRG_VKVALIDATE(vkCreateCommandPool(device, &poolInfo, nullptr, &returnCommandPool), "failed to create command pool!");

		return returnCommandPool;
	}

	[[nodiscard]] std::vector<VkCommandBuffer> allocateCommandBuffers(const MRG::Vulkan::WindowProperties* data)
	{
		std::vector<VkCommandBuffer> commandBuffers(data->swapChain.frameBuffers.size());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = data->commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		MRG_VKVALIDATE(vkAllocateCommandBuffers(data->device, &allocInfo, commandBuffers.data()), "failed to allocate command buffers!");
		MRG_ENGINE_TRACE("{} command buffers successfully allocated", commandBuffers.size());

		return commandBuffers;
	}

	[[nodiscard]] MRG::Vulkan::DepthBuffer createDepthBuffer(MRG::Vulkan::WindowProperties* data)
	{
		MRG::Vulkan::DepthBuffer depthBuffer;

		const auto depthFormat = findDepthFormat(data->physicalDevice);

		MRG::Vulkan::createImage(data->physicalDevice,
		                         data->device,
		                         data->swapChain.extent.width,
		                         data->swapChain.extent.height,
		                         depthFormat,
		                         VK_IMAGE_TILING_OPTIMAL,
		                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		                         depthBuffer.depthImage,
		                         depthBuffer.memoryHandle);
		depthBuffer.imageView = MRG::Vulkan::createImageView(data->device, depthBuffer.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

		return depthBuffer;
	}

	void cleanupSwapChain(MRG::Vulkan::WindowProperties* data, std::vector<MRG::Vulkan::DescriptorAllocator>& allocators)
	{
		vkDestroyImageView(data->device, data->swapChain.depthBuffer.imageView, nullptr);
		vkDestroyImage(data->device, data->swapChain.depthBuffer.depthImage, nullptr);
		vkFreeMemory(data->device, data->swapChain.depthBuffer.memoryHandle, nullptr);

		for (auto framebuffer : data->swapChain.frameBuffers) vkDestroyFramebuffer(data->device, framebuffer, nullptr);

		vkFreeCommandBuffers(
		  data->device, data->commandPool, static_cast<uint32_t>(data->commandBuffers.size()), data->commandBuffers.data());

		vkDestroyPipeline(data->device, data->pipeline.handle, nullptr);

		vkDestroyPipelineLayout(data->device, data->pipeline.layout, nullptr);

		vkDestroyRenderPass(data->device, data->pipeline.renderPass, nullptr);

		for (auto imageView : data->swapChain.imageViews) vkDestroyImageView(data->device, imageView, nullptr);

		vkDestroySwapchainKHR(data->device, data->swapChain.handle, nullptr);

		for (auto& allocator : allocators) { allocator.shutdown(); }
	}

	void recreateSwapChain(MRG::Vulkan::WindowProperties* data,
	                       MRG::Ref<MRG::Vulkan::Shader> textureShader,
	                       MRG::Ref<MRG::Vulkan::VertexArray> vertexArray,
	                       std::vector<MRG::Vulkan::DescriptorAllocator>& allocators)
	{
		int width = data->width, height = data->height;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(MRG::Renderer2D::getGLFWWindow(), &width, &height);
			glfwWaitEvents();
		}

		data->width = width;
		data->height = height;

		MRG_ENGINE_TRACE("Recreating swap chain");
		MRG_ENGINE_TRACE("Waiting for device to be idle...");
		vkDeviceWaitIdle(data->device);

		cleanupSwapChain(data, allocators);

		data->swapChain = createSwapChain(data->physicalDevice, data->surface, data->device, data);
		MRG_ENGINE_INFO("Vulkan swap chain succesfully recreated");

		data->pipeline.renderPass = createRenderPass(data->physicalDevice, data->device, data->swapChain.imageFormat);

		const auto pipelineLayoutCreateInfo = populatePipelineLayout(&data->descriptorSetLayout, &data->pushConstantRanges);
		MRG_VKVALIDATE(vkCreatePipelineLayout(data->device, &pipelineLayoutCreateInfo, nullptr, &data->pipeline.layout),
		               "failed to recreate pipeline layout!");
		MRG_ENGINE_TRACE("Vulkan graphics pipeline layout successfully created");
		data->pipeline.handle =
		  createPipeline(data, textureShader, vertexArray->getAttributeDescriptions(), {vertexArray->getBindingDescription()});

		data->swapChain.depthBuffer = createDepthBuffer(data);

		MRG_ENGINE_INFO("Vulkan graphics pipeline successfully created");
		data->swapChain.frameBuffers = createframeBuffers(data->device,
		                                                  data->swapChain.imageViews,
		                                                  data->swapChain.depthBuffer.imageView,
		                                                  data->pipeline.renderPass,
		                                                  data->swapChain.extent);
		MRG_ENGINE_TRACE("Framebuffers successfully created");

		for (auto& allocator : allocators) { allocator.init(data); }

		data->commandBuffers = allocateCommandBuffers(data);
	}

}  // namespace

namespace MRG::Vulkan
{
	void Renderer2D::init()
	{
		MRG_PROFILE_FUNCTION();

		m_data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(MRG::Renderer2D::getGLFWWindow()));
		m_textureShader = createRef<Shader>("resources/shaders/texture");

		// clang-format off
		const std::vector<Vertex> vertices = {{{-0.5f, -0.5f}, {0.0f, 0.0f}},
		                                      {{0.5f, -0.5f}, {1.0f, 0.0f}},
		                                      {{0.5f, 0.5f}, {1.0f, 1.0f}},
		                                      {{-0.5f, 0.5f}, {0.0f, 1.0f}}};
		// clang-format on

		const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

		try {
			m_data->swapChain = createSwapChain(m_data->physicalDevice, m_data->surface, m_data->device, m_data);
			MRG_ENGINE_INFO("Vulkan swap chain successfully created");

			m_data->pipeline.renderPass = createRenderPass(m_data->physicalDevice, m_data->device, m_data->swapChain.imageFormat);

			m_data->commandPool = createCommandPool(m_data->device, m_data->physicalDevice, m_data->surface);
			MRG_ENGINE_TRACE("Command pool successfully created");

			const auto vertexBuffer = createRef<VertexBuffer>(vertices.data(), static_cast<uint32_t>(sizeof(Vertex) * vertices.size()));
			vertexBuffer->layout = Vertex::layout;

			const auto indexBuffer = createRef<IndexBuffer>(indices.data(), static_cast<uint32_t>(indices.size()));

			m_whiteTexture = createRef<Texture2D>(1, 1);
			auto whiteTextureData = 0xffffffff;
			m_whiteTexture->setData(&whiteTextureData, sizeof(whiteTextureData));

			m_vertexArray = createRef<VertexArray>();
			m_vertexArray->addVertexBuffer(vertexBuffer);
			m_vertexArray->setIndexBuffer(indexBuffer);

			m_data->descriptorSetLayout = createDescriptorSetLayout(m_data->device);

			m_data->pushConstantRanges = populatePushConstantsRanges();

			const auto pipelineLayoutCreateInfo = populatePipelineLayout(&m_data->descriptorSetLayout, &m_data->pushConstantRanges);
			MRG_VKVALIDATE(vkCreatePipelineLayout(m_data->device, &pipelineLayoutCreateInfo, nullptr, &m_data->pipeline.layout),
			               "failed to create pipeline layout!");
			MRG_ENGINE_TRACE("Vulkan graphics pipeline layout successfully created");

			m_data->pipeline.handle =
			  createPipeline(m_data,
			                 m_textureShader,
			                 std::static_pointer_cast<MRG::Vulkan::VertexArray>(m_vertexArray)->getAttributeDescriptions(),
			                 {std::static_pointer_cast<MRG::Vulkan::VertexArray>(m_vertexArray)->getBindingDescription()});
			MRG_ENGINE_INFO("Vulkan graphics pipeline successfully created");

			m_data->swapChain.depthBuffer = createDepthBuffer(m_data);

			m_data->swapChain.frameBuffers = createframeBuffers(m_data->device,
			                                                    m_data->swapChain.imageViews,
			                                                    m_data->swapChain.depthBuffer.imageView,
			                                                    m_data->pipeline.renderPass,
			                                                    m_data->swapChain.extent);
			MRG_ENGINE_TRACE("Framebuffers successfully created");

			m_allocators.resize(m_data->swapChain.imageCount);
			for (auto& allocator : m_allocators) { allocator.init(m_data); }

			m_data->commandBuffers = allocateCommandBuffers(m_data);

			m_imageAvailableSemaphores.resize(m_maxFramesInFlight);
			m_inFlightFences.resize(m_maxFramesInFlight);
			m_imagesInFlight.resize(m_data->swapChain.imageCount, VK_NULL_HANDLE);

			VkSemaphoreCreateInfo semaphoreInfo{};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkFenceCreateInfo fenceInfo{};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (std::size_t i = 0; i < m_maxFramesInFlight; ++i) {
				MRG_VKVALIDATE(vkCreateSemaphore(m_data->device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]),
				               "failed to create semaphores for a frame!");
				MRG_VKVALIDATE(vkCreateFence(m_data->device, &fenceInfo, nullptr, &m_inFlightFences[i]),
				               "failed to create fences for a frame!");
			}
		} catch (const std::runtime_error& e) {
			MRG_ENGINE_ERROR("Vulkan error detected: {}", e.what());
		}
	}

	void Renderer2D::shutdown()
	{
		MRG_PROFILE_FUNCTION();

		vkDeviceWaitIdle(m_data->device);

		for (std::size_t i = 0; i < m_maxFramesInFlight; ++i) {
			vkDestroySemaphore(m_data->device, m_imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_data->device, m_inFlightFences[i], nullptr);
		}

		cleanupSwapChain(m_data, m_allocators);

		vkDestroyDescriptorSetLayout(m_data->device, m_data->descriptorSetLayout, nullptr);

		m_vertexArray->destroy();
		m_textureShader->destroy();

		m_whiteTexture->destroy();

		vkDestroyCommandPool(m_data->device, m_data->commandPool, nullptr);
	}

	void Renderer2D::onWindowResize(uint32_t, uint32_t) { m_shouldRecreateSwapChain = true; }

	bool Renderer2D::beginFrame()
	{
		MRG_PROFILE_FUNCTION();

		// wait for preview frames to be finished (only allow m_maxFramesInFlight)
		vkWaitForFences(m_data->device, 1, &m_inFlightFences[m_data->currentFrame], VK_TRUE, UINT64_MAX);

		m_batchedDrawCalls.clear();

		// Acquire an image from the swapchain
		try {
			const auto result = vkAcquireNextImageKHR(m_data->device,
			                                          m_data->swapChain.handle,
			                                          UINT64_MAX,
			                                          m_imageAvailableSemaphores[m_data->currentFrame],
			                                          VK_NULL_HANDLE,
			                                          &m_imageIndex);

			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				recreateSwapChain(m_data, m_textureShader, m_vertexArray, m_allocators);
				return false;
			}

			if (m_imagesInFlight[m_imageIndex] != VK_NULL_HANDLE) {
				vkWaitForFences(m_data->device, 1, &m_imagesInFlight[m_imageIndex], VK_TRUE, UINT64_MAX);
			}
			m_imagesInFlight[m_imageIndex] = m_inFlightFences[m_data->currentFrame];
		} catch (const std::runtime_error& e) {
			MRG_ENGINE_ERROR("Vulkan error detected: {}", e.what());
		}
		return true;
	}

	bool Renderer2D::endFrame()
	{
		MRG_PROFILE_FUNCTION();

		// Execute the command buffer with the current image
		vkWaitForFences(m_data->device, 1, &m_inFlightFences[m_data->currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_data->device, 1, &m_inFlightFences[m_data->currentFrame]);

		VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_data->currentFrame]};
		VkSemaphore signalSempahores[] = {m_imageAvailableSemaphores[m_data->currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_data->commandBuffers[m_imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSempahores;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkResetCommandBuffer(m_data->commandBuffers[m_imageIndex], 0);

		MRG_VKVALIDATE(vkBeginCommandBuffer(m_data->commandBuffers[m_imageIndex], &beginInfo), "failed to begin recording command bufer!");

		std::array<VkClearValue, 2> clearColors{};
		clearColors[0].color = {{m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a}};
		clearColors[1].depthStencil = {1.f, 0};

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_data->pipeline.renderPass;
		renderPassInfo.framebuffer = m_data->swapChain.frameBuffers[m_imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = m_data->swapChain.extent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
		renderPassInfo.pClearValues = clearColors.data();

		vkCmdBeginRenderPass(m_data->commandBuffers[m_imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_data->commandBuffers[m_imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_data->pipeline.handle);

		VkBuffer vertexBuffers[] = {std::static_pointer_cast<MRG::Vulkan::VertexBuffer>(m_vertexArray->getVertexBuffers()[0])->getHandle()};
		VkDeviceSize offsets[] = {0};
		auto indexBuffer = std::static_pointer_cast<MRG::Vulkan::IndexBuffer>(m_vertexArray->getIndexBuffer());
		vkCmdBindVertexBuffers(m_data->commandBuffers[m_imageIndex], 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(m_data->commandBuffers[m_imageIndex], indexBuffer->getHandle(), 0, VK_INDEX_TYPE_UINT32);

		m_descriptorSets = m_allocators[m_imageIndex].requestDescriptorSets(m_batchedDrawCalls, m_whiteTexture);
		m_pushConstants.resize(m_batchedDrawCalls.size());
		for (std::size_t i = 0; i < m_batchedDrawCalls.size(); ++i) {
			m_pushConstants[i].transform = m_modelMatrix * m_batchedDrawCalls[i].transform;
			m_pushConstants[i].tilingFactor = m_batchedDrawCalls[i].tiling;
			m_pushConstants[i].color = m_batchedDrawCalls[i].color;

			vkCmdBindDescriptorSets(m_data->commandBuffers[m_imageIndex],
			                        VK_PIPELINE_BIND_POINT_GRAPHICS,
			                        m_data->pipeline.layout,
			                        0,
			                        1,
			                        &m_descriptorSets[i],
			                        0,
			                        nullptr);

			vkCmdPushConstants(m_data->commandBuffers[m_imageIndex],
			                   m_data->pipeline.layout,
			                   VK_SHADER_STAGE_VERTEX_BIT,
			                   0,
			                   sizeof(PushConstants),
			                   &m_pushConstants[i]);

			vkCmdDrawIndexed(m_data->commandBuffers[m_imageIndex], indexBuffer->getCount(), 1, 0, 0, 0);
			++m_stats.drawCalls;
		}

		auto& io = ImGui::GetIO();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_data->commandBuffers[m_imageIndex]);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			const auto contextBkp = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(contextBkp);
		}

		vkCmdEndRenderPass(m_data->commandBuffers[m_imageIndex]);

		MRG_VKVALIDATE(vkEndCommandBuffer(m_data->commandBuffers[m_imageIndex]), "failed to record command buffer!");

		MRG_VKVALIDATE(vkQueueSubmit(m_data->graphicsQueue.handle, 1, &submitInfo, m_inFlightFences[m_data->currentFrame]),
		               "failed to submit draw command buffer!");

		VkSwapchainKHR swapChains[] = {m_data->swapChain.handle};

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSempahores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_imageIndex;

		const auto result = vkQueuePresentKHR(m_data->presentQueue.handle, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_shouldRecreateSwapChain) {
			recreateSwapChain(m_data, m_textureShader, m_vertexArray, m_allocators);
			m_shouldRecreateSwapChain = false;
		}

		m_data->currentFrame = (m_data->currentFrame + 1) % m_maxFramesInFlight;

		return true;
	}

	void Renderer2D::beginScene(const OrthoCamera& OrthoCamera)
	{
		MRG_PROFILE_FUNCTION();

		m_modelMatrix = OrthoCamera.getProjectionViewMatrix();
	}

	void Renderer2D::endScene() {}

	void Renderer2D::drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		MRG_PROFILE_FUNCTION();

		++m_stats.quadCount;

		m_batchedDrawCalls.emplace_back(
		  DrawCallType::Quad, glm::translate(glm::mat4{1.f}, position) * glm::scale(glm::mat4{1.f}, {size.x, size.y, 1.f}), color);
	}

	void Renderer2D::drawQuad(
	  const glm::vec3& position, const glm::vec2& size, const Ref<MRG::Texture2D>& texture, float tilingFactor, const glm::vec4& color)
	{
		MRG_PROFILE_FUNCTION();

		++m_stats.quadCount;

		m_batchedDrawCalls.emplace_back(DrawCallType::TexturedQuad,
		                                glm::translate(glm::mat4{1.f}, position) * glm::scale(glm::mat4{1.f}, {size.x, size.y, 1.f}),
		                                texture,
		                                tilingFactor,
		                                color);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		MRG_PROFILE_FUNCTION();

		++m_stats.quadCount;

		m_batchedDrawCalls.emplace_back(DrawCallType::Quad,
		                                glm::translate(glm::mat4{1.f}, position) * glm::rotate(glm::mat4{1.f}, rotation, {0.f, 0.f, 1.f}) *
		                                  glm::scale(glm::mat4{1.f}, {size.x, size.y, 1.f}),
		                                color);
	}

	void Renderer2D::drawRotatedQuad(const glm::vec3& position,
	                                 const glm::vec2& size,
	                                 float rotation,
	                                 const Ref<MRG::Texture2D>& texture,
	                                 float tilingFactor,
	                                 const glm::vec4& color)
	{
		MRG_PROFILE_FUNCTION();

		++m_stats.quadCount;

		m_batchedDrawCalls.emplace_back(DrawCallType::TexturedQuad,
		                                glm::translate(glm::mat4{1.f}, position) * glm::rotate(glm::mat4{1.f}, rotation, {0.f, 0.f, 1.f}) *
		                                  glm::scale(glm::mat4{1.f}, {size.x, size.y, 1.f}),
		                                texture,
		                                tilingFactor,
		                                color);
	}
}  // namespace MRG::Vulkan
