#ifndef MRG_VULKAN_IMPL_RENDERER2D
#define MRG_VULKAN_IMPL_RENDERER2D

#include "Renderer/Renderer2D.h"

#include "Renderer/APIs/Vulkan/Framebuffer.h"
#include "Renderer/APIs/Vulkan/Helper.h"
#include "Renderer/APIs/Vulkan/Shader.h"
#include "Renderer/APIs/Vulkan/Textures.h"
#include "Renderer/APIs/Vulkan/VertexArray.h"
#include "Renderer/APIs/Vulkan/WindowProperties.h"

#include <optional>

namespace MRG::Vulkan
{
	struct PushConstants
	{
		glm::mat4 viewProjection;
	};

	class Renderer2D : public Generic2DRenderer
	{
	public:
		Renderer2D() = default;
		Renderer2D(const Renderer2D&) = delete;
		Renderer2D(Renderer2D&&) = delete;
		~Renderer2D() override = default;

		Renderer2D& operator=(const Renderer2D&) = delete;
		Renderer2D& operator=(Renderer2D&&) = delete;

		void init() override;
		void shutdown() override;

		void onWindowResize(uint32_t width, uint32_t height) override;

		bool beginFrame() override;
		bool endFrame() override;

		void beginScene(const Camera& camera, const glm::mat4& transform) override;
		void beginScene(const EditorCamera& camera) override;
		void endScene() override;

		void drawQuad(const glm::mat4& transform, const glm::vec4& color, uint32_t objectID) override;
		void
		drawQuad(const glm::mat4& transform, const Ref<MRG::Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor) override;

		void drawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color) override;
		void drawQuad(const glm::vec3& position,
		              const glm::vec2& size,
		              const Ref<MRG::Texture2D>& texture,
		              float tilingFactor,
		              const glm::vec4& tintColor) override;

		void drawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color) override;
		void drawRotatedQuad(const glm::vec3& position,
		                     const glm::vec2& size,
		                     float rotation,
		                     const Ref<MRG::Texture2D>& texture,
		                     float tilingFactor,
		                     const glm::vec4& tintColor) override;

		void setRenderTarget(Ref<MRG::Framebuffer> renderTarget) override;
		void resetRenderTarget() override;
		[[nodiscard]] Ref<MRG::Framebuffer> getRenderTarget() const override { return m_renderTarget; }

		void setViewport(uint32_t, uint32_t, uint32_t, uint32_t) override {}
		void setClearColor(const glm::vec4& color) override
		{
			m_clearColor = color;
			if (m_renderTarget != nullptr) {
				m_renderTarget->setClearColor(color);
			}
		}
		void clear() override;

		void resetStats() override { m_stats = {}; };
		[[nodiscard]] RenderingStatistics getStats() const override { return m_stats; };

	private:
		void setupScene();
		void cleanupSwapChain();
		void recreateSwapChain();
		void updateDescriptor();
		void flushAndReset();

		WindowProperties* m_data{};
		uint32_t m_imageIndex{};
		std::size_t m_maxFramesInFlight = 2;
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkFence> m_inFlightFences, m_imagesInFlight;
		Ref<Texture2D> m_whiteTexture;
		VkDescriptorPool m_descriptorPool{};
		std::vector<VkDescriptorSet> m_descriptorSets;
		bool m_shouldRecreateSwapChain = false;

		bool m_sceneInProgress = false;

		PushConstants m_modelMatrix{};

		glm::vec4 m_clearColor = {0.f, 0.f, 0.f, 1.f};
		Ref<Framebuffer> m_renderTarget;

		RenderingStatistics m_stats;
	};
}  // namespace MRG::Vulkan

#endif
