//
// Created by Mathis Lamidey on 2021-05-18.
//

#ifndef MORRIGU_RENDEROBJECT_H
#define MORRIGU_RENDEROBJECT_H

#include "Rendering/Material.h"
#include "Rendering/Mesh.h"

#include "Utils/GLMIncludeHelper.h"

namespace MRG
{
	struct RenderData
	{
		// mesh data
		vk::Buffer vertexBuffer;
		std::size_t vertexNumber{};

		// material data
		vk::PipelineLayout materialPipelineLayout;
		vk::Pipeline materialPipeline;
		vk::DescriptorSet level2Descriptor;

		// RO data
		vk::DescriptorSet level3Descriptor;
		Ref<bool> isVisible{};
	};

	template<Vertex VertexType>
	class RenderObject
	{
	public:
		RenderObject(vk::Device device,
		             VmaAllocator allocator,
		             const Ref<Mesh<VertexType>>& mesh,
		             const Ref<Material<VertexType>>& material,
		             Ref<Texture> defaultTexture,
		             DeletionQueue& deletionQueue)
		    : mesh{mesh}, material{material}, m_device{device}, m_allocator{allocator}
		{
			// Pool creation
			std::array<vk::DescriptorPoolSize, 2> sizes{
			  vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,
			                         std::max(static_cast<uint32_t>(material->shader->l3UBOSizes.size()), 1u)},
			  vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
			                         std::max(static_cast<uint32_t>(material->shader->l3ImageBindings.size()), 1u)},
			};
			vk::DescriptorPoolCreateInfo poolInfo{
			  .maxSets       = 1,
			  .poolSizeCount = static_cast<uint32_t>(sizes.size()),
			  .pPoolSizes    = sizes.data(),
			};

			m_descriptorPool = m_device.createDescriptorPool(poolInfo);
			deletionQueue.push([this, device]() { device.destroyDescriptorPool(m_descriptorPool); });

			vk::DescriptorSetAllocateInfo setAllocInfo{
			  .descriptorPool     = m_descriptorPool,
			  .descriptorSetCount = 1,
			  .pSetLayouts        = &material->shader->level3DSL,
			};
			level3Descriptor = m_device.allocateDescriptorSets(setAllocInfo).back();

			for (const auto& [bindingSlot, size] : material->shader->l3UBOSizes) {
				const auto newBuffer = Utils::Allocators::createBuffer(
				  allocator, size, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, deletionQueue);
				m_uniformBuffers.insert(std::make_pair(bindingSlot, newBuffer));

				vk::DescriptorBufferInfo descriptorBufferInfo{
				  .buffer = newBuffer.buffer,
				  .offset = 0,
				  .range  = size,
				};
				vk::WriteDescriptorSet setWrite{
				  .dstSet          = level3Descriptor,
				  .dstBinding      = bindingSlot,
				  .descriptorCount = 1,
				  .descriptorType  = vk::DescriptorType::eUniformBuffer,
				  .pBufferInfo     = &descriptorBufferInfo,
				};

				m_device.updateDescriptorSets(setWrite, {});
			}

			for (const auto& imageBinding : material->shader->l3ImageBindings) {
				m_sampledImages.insert(std::make_pair(imageBinding, defaultTexture));
				bindTexture(imageBinding, defaultTexture);
			}
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
			  .dstSet          = level3Descriptor,
			  .dstBinding      = bindingSlot,
			  .descriptorCount = 1,
			  .descriptorType  = vk::DescriptorType::eCombinedImageSampler,
			  .pImageInfo      = &imageBufferInfo,
			};
			m_device.updateDescriptorSets(textureUpdate, {});
		}

		RenderData getRenderData()
		{
			return RenderData{
			  .vertexBuffer           = mesh->vertexBuffer.buffer,
			  .vertexNumber           = mesh->vertices.size(),
			  .materialPipelineLayout = material->pipelineLayout,
			  .materialPipeline       = material->pipeline,
			  .level2Descriptor       = material->level2Descriptor,
			  .level3Descriptor       = level3Descriptor,
			  .isVisible              = isVisible,
			};
		}

		Ref<Mesh<VertexType>> mesh;
		Ref<Material<VertexType>> material;
		Ref<glm::mat4> modelMatrix{createRef<glm::mat4>(1.f)};
		Ref<bool> isVisible{createRef<bool>(true)};
		vk::DescriptorSet level3Descriptor;

		void rotate(const glm::vec3& axis, float radRotation) { *modelMatrix = glm::rotate(*modelMatrix, radRotation, axis); }
		void scale(const glm::vec3& scaling) { *modelMatrix = glm::scale(*modelMatrix, scaling); }
		void translate(const glm::vec3& translation) { *modelMatrix = glm::translate(*modelMatrix, translation); }
		void setVisible(bool visible) { *isVisible = visible; }

	private:
		vk::Device m_device;
		VmaAllocator m_allocator;

		vk::DescriptorPool m_descriptorPool;

		std::map<uint32_t, AllocatedBuffer> m_uniformBuffers;
		std::map<uint32_t, Ref<Texture>> m_sampledImages;
	};
}  // namespace MRG

#endif  // MORRIGU_RENDEROBJECT_H
