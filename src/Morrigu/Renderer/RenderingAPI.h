#ifndef MRG_CLASS_RENDERINGAPI
#define MRG_CLASS_RENDERINGAPI

#include "Renderer/VertexArray.h"

#include <glm/glm.hpp>

namespace MRG
{
	class RenderingAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL
		};

		virtual ~RenderingAPI() = default;

		virtual void init() = 0;

		virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void setClearColor(const glm::vec4& color) = 0;
		virtual void clear() = 0;

		virtual void drawIndexed(const Ref<VertexArray>& vertexArray) = 0;

		[[nodiscard]] inline static auto getAPI() { return s_API; }

		[[nodiscard]] static Scope<RenderingAPI> create();

	private:
		static API s_API;
	};
}  // namespace MRG

#endif