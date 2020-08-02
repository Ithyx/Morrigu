#include "VertexArray.h"

#include "Renderer/APIs/OpenGL/VertexArray.h"
#include "Renderer/Renderer.h"

namespace MRG
{
	VertexArray* VertexArray::create()
	{
		switch (Renderer::getAPI()) {
		case RenderingAPI::API::OpenGL: {
			return new OpenGL::VertexArray;
		} break;

		case RenderingAPI::API::None:
		default: {
			MRG_CORE_ASSERT(false, "UNSUPPORTED RENDERER API OPTION ! ({})", Renderer::getAPI());
			return nullptr;
		} break;
		}
	}
}  // namespace MRG