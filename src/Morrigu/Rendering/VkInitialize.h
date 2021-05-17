//
// Created by Mathis Lamidey on 2021-04-11.
//

#ifndef MORRIGU_VKINITIALIZE_H
#define MORRIGU_VKINITIALIZE_H

#include "Rendering/VkTypes.h"

namespace MRG::VkInit
{
	[[nodiscard]] vk::CommandPoolCreateInfo cmdPoolCreateInfo(vk::CommandPoolCreateFlags flags, uint32_t queueFamiliyIndex);
	[[nodiscard]] vk::CommandBufferAllocateInfo cmdBufferAllocateInfo(VkCommandPool pool, vk::CommandBufferLevel level, uint32_t count);

	[[nodiscard]] vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(vk::ShaderStageFlagBits stage,
	                                                                              vk::ShaderModule shaderModule);
	[[nodiscard]] vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology topology);
	[[nodiscard]] vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(vk::PolygonMode polygonMode);
	[[nodiscard]] vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo();
	[[nodiscard]] vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState();
}  // namespace MRG::VkInit

#endif  // MORRIGU_VKINITIALIZE_H