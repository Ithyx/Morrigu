#ifndef MRG_VULKAN_IMPL_CONTEXT
#define MRG_VULKAN_IMPL_CONTEXT

#include "Renderer/Context.h"

#include <GLFW/glfw3.h>

namespace MRG::Vulkan
{
	class Context : public MRG::Context
	{
	public:
		explicit Context(GLFWwindow* window);
		Context(Context&&) = delete;
		~Context() override;

		Context& operator=(const Context&) = delete;
		Context& operator=(Context&&) = delete;

		void swapBuffers() override;
		void swapInterval(int interval) override;

	private:
		GLFWwindow* m_window;
	};
}  // namespace MRG::Vulkan

#endif
