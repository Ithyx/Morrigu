#ifndef MRG_CLASS_WINDOW
#define MRG_CLASS_WINDOW

#include "Events/Event.h"
#include "Renderer/Context.h"
#include "Renderer/WindowProperties.h"

#include <functional>
#include <memory>
#include <string>

namespace MRG
{
	using EventCallbackFunction = std::function<void(Event&)>;

	class Window
	{
	public:
		Window(Scope<WindowProperties> props);
		~Window();

		void onUpdate();

		[[nodiscard]] unsigned int getWidth() const { return m_properties->width; }
		[[nodiscard]] unsigned int getHeight() const { return m_properties->height; }
		[[nodiscard]] bool isVsync() const { return m_properties->VSync; }

		void setEventCallback(const EventCallbackFunction& callback) { m_properties->callback = callback; }
		void setVsync(bool enabled);

		[[nodiscard]] GLFWwindow* getGLFWWindow() const { return m_window; }

	private:
		void _init();
		void _shutdown();

		GLFWwindow* m_window;
		Scope<Context> m_context;
		Scope<WindowProperties> m_properties;

		static uint8_t s_GLFWWindowCount;
	};
}  // namespace MRG

#endif