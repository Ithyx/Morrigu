//
// Created by Mathis Lamidey on 2021-05-18.
//

#ifndef MORRIGU_MATERIAL_H
#define MORRIGU_MATERIAL_H

#include "Rendering/PipelineBuilder.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Vertex.h"
#include "Rendering/VkInitialize.h"
#include "Rendering/VkTypes.h"
#include "Utils/Allocators.h"

namespace MRG
{
	template<Vertex VertexType>
	class Material
	{
	public:
		explicit Material(vk::Device device,
		                  VmaAllocator allocator,
		                  const Ref<Shader>& shader,
		                  vk::PipelineCache pipelineCache,
		                  vk::RenderPass renderPass,
		                  vk::DescriptorSetLayout level0DSL,
		                  vk::DescriptorSetLayout level1DSL,
		                  const Ref<Texture>& defaultTexture,
		                  DeletionQueue& deletionQueue)
		    : m_device{device}, m_allocator{allocator}, m_shader{shader}
		{
			// @TODO(Ithyx): replace with device limit
			constexpr static const auto ARBITRARY_UNIFORM_LIMIT        = 72;
			constexpr static const auto ARBITRARY_SAMPLED_IMAGES_LIMIT = 1200;

			// Pool creation
			std::array<vk::DescriptorPoolSize, 2> sizes{
			  vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, ARBITRARY_UNIFORM_LIMIT},
			  vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, ARBITRARY_SAMPLED_IMAGES_LIMIT},
			};
			vk::DescriptorPoolCreateInfo poolInfo{
			  .maxSets       = ARBITRARY_UNIFORM_LIMIT + ARBITRARY_SAMPLED_IMAGES_LIMIT,
			  .poolSizeCount = static_cast<uint32_t>(sizes.size()),
			  .pPoolSizes    = sizes.data(),
			};

			m_descriptorPool = m_device.createDescriptorPool(poolInfo);
			deletionQueue.push([this]() { m_device.destroyDescriptorPool(m_descriptorPool); });

			vk::DescriptorSetAllocateInfo setAllocInfo{
			  .descriptorPool     = m_descriptorPool,
			  .descriptorSetCount = 1,
			  .pSetLayouts        = &m_shader->level2DSL,
			};
			level2Descriptor = m_device.allocateDescriptorSets(setAllocInfo).back();

			for (const auto& [bindingSlot, size] : m_shader->uboSizes) {
				const auto newBuffer = Utils::Allocators::createBuffer(
				  m_allocator, size, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, deletionQueue);
				m_uniformBuffers.insert(std::make_pair(bindingSlot, newBuffer));

				vk::DescriptorBufferInfo descriptorBufferInfo{
				  .buffer = newBuffer.buffer,
				  .offset = 0,
				  .range  = size,
				};
				vk::WriteDescriptorSet setWrite{
				  .dstSet          = level2Descriptor,
				  .dstBinding      = bindingSlot,
				  .descriptorCount = 1,
				  .descriptorType  = vk::DescriptorType::eUniformBuffer,
				  .pBufferInfo     = &descriptorBufferInfo,
				};

				m_device.updateDescriptorSets(setWrite, {});
			}

			for (const auto& imageBinding : m_shader->imageBindings) {
				m_sampledImages.insert(std::make_pair(imageBinding, defaultTexture));
				bindTexture(imageBinding, defaultTexture);
			}

			std::array<vk::DescriptorSetLayout, 3> setLayouts{level0DSL, level1DSL, m_shader->level2DSL};
			vk::PipelineLayoutCreateInfo layoutInfo{
			  .setLayoutCount         = static_cast<uint32_t>(setLayouts.size()),
			  .pSetLayouts            = setLayouts.data(),
			  .pushConstantRangeCount = static_cast<uint32_t>(m_shader->pcRanges.size()),
			  .pPushConstantRanges    = m_shader->pcRanges.data(),
			};
			pipelineLayout = m_device.createPipelineLayout(layoutInfo);
			deletionQueue.push([this]() { m_device.destroyPipelineLayout(pipelineLayout); });

			const VertexInputDescription vertexInfo = VertexType::getVertexDescription();
			vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
			  .vertexBindingDescriptionCount   = static_cast<uint32_t>(vertexInfo.bindings.size()),
			  .pVertexBindingDescriptions      = vertexInfo.bindings.data(),
			  .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInfo.attributes.size()),
			  .pVertexAttributeDescriptions    = vertexInfo.attributes.data(),
			};

			PipelineBuilder pipelineBuilder{
			  .shaderStages{VkInit::pipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eVertex, m_shader->vertexShaderModule),
			                VkInit::pipelineShaderStageCreateInfo(vk::ShaderStageFlagBits::eFragment, m_shader->fragmentShaderModule)},
			  .vertexInputInfo{vertexInputStateCreateInfo},
			  .inputAssemblyInfo{VkInit::pipelineInputAssemblyStateCreateInfo(vk::PrimitiveTopology::eTriangleList)},
			  .rasterizerInfo{VkInit::pipelineRasterizationStateCreateInfo(vk::PolygonMode::eFill)},
			  .multisamplingInfo{VkInit::pipelineMultisampleStateCreateInfo()},
			  .depthStencilStateCreateInfo{VkInit::pipelineDepthStencilStateCreateInfo(true, true, vk::CompareOp::eLessOrEqual)},
			  .colorBlendAttachment{VkInit::pipelineColorBlendAttachmentState()},
			  .pipelineLayout{pipelineLayout},
			  .pipelineCache{pipelineCache},
			};

			pipeline = pipelineBuilder.build_pipeline(m_device, renderPass);
			deletionQueue.push([this]() { m_device.destroyPipeline(pipeline); });
		}

		template<typename UniformType>
		void uploadUniform(uint32_t bindingSlot, const UniformType& uniformData)
		{
			MRG_ENGINE_ASSERT(m_uniformBuffers.contains(bindingSlot), "Invalid binding slot!")
			void* data;
			vmaMapMemory(m_allocator, m_uniformBuffers[bindingSlot].allocation, &data);
			memcpy(data, &uniformData, sizeof(UniformType));
			vmaUnmapMemory(m_allocator, m_uniformBuffers[bindingSlot].allocation);
		}

		void bindTexture(uint32_t bindingSlot, const Ref<Texture>& texture)
		{
			MRG_ENGINE_ASSERT(m_sampledImages.contains(bindingSlot), "Invalid binding slot!")
			vk::DescriptorImageInfo imageBufferInfo{
			  .sampler     = texture->sampler,
			  .imageView   = texture->imageView,
			  .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
			};
			vk::WriteDescriptorSet textureUpdate = {
			  .dstSet          = level2Descriptor,
			  .dstBinding      = bindingSlot,
			  .descriptorCount = 1,
			  .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
			  .pImageInfo      = &imageBufferInfo,
			};
			m_device.updateDescriptorSets(textureUpdate, {});
		}

		vk::Pipeline pipeline;
		vk::PipelineLayout pipelineLayout;
		vk::DescriptorSet level2Descriptor;

	private:
		vk::Device m_device;
		VmaAllocator m_allocator;

		Ref<Shader> m_shader;
		vk::DescriptorPool m_descriptorPool;

		std::map<uint32_t, AllocatedBuffer> m_uniformBuffers;
		std::map<uint32_t, Ref<Texture>> m_sampledImages;
	};
}  // namespace MRG

#endif  // MORRIGU_MATERIAL_H
