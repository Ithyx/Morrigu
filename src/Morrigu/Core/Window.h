#ifndef MRG_CLASS_WINDOW
#define MRG_CLASS_WINDOW

#include "Events/Event.h"
#include "Renderer/Context.h"
#include "Renderer/WindowProperties.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace MRG
{
	class GLFWWindowWrapper
	{
	public:
		explicit GLFWWindowWrapper(WindowProperties* props);
		~GLFWWindowWrapper();

		GLFWwindow* handle;
	};

	using EventCallbackFunction = std::function<void(Event&)>;

	class Window
	{
	public:
		explicit Window(Scope<WindowProperties> props);

		void onUpdate();

		[[nodiscard]] uint32_t getWidth() const { return m_properties->width; }
		[[nodiscard]] uint32_t getHeight() const { return m_properties->height; }
		[[nodiscard]] bool isVsync() const { return m_properties->VSync; }

		void setEventCallback(const EventCallbackFunction& callback) { m_properties->callback = callback; }
		void setVsync(bool enabled);

		[[nodiscard]] GLFWwindow* getGLFWWindow() const { return m_window.handle; }

	private:
		GLFWWindowWrapper m_window;
		Scope<WindowProperties> m_properties;
		Scope<Context> m_context;

		static uint8_t s_GLFWWindowCount;
		friend GLFWWindowWrapper::GLFWWindowWrapper(WindowProperties* props), GLFWWindowWrapper::~GLFWWindowWrapper();
	};
}  // namespace MRG

#endif
