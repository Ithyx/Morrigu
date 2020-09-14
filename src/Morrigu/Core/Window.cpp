#include "Window.h"

#include "Core/Logger.h"
#include "Debug/Instrumentor.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"
#include "Renderer/Renderer.h"

namespace MRG
{
	static void GLFWErrorCallback(int error, const char* description)
	{
		MRG_ENGINE_ERROR("GLFW error detected: {0}: {1}", error, description);
	}

	uint8_t Window::s_GLFWWindowCount = 0;

	Window::Window(Scope<WindowProperties> props)
	{
		MRG_PROFILE_FUNCTION();

		m_properties = std::move(props);
		_init();
	}
	Window::~Window()
	{
		MRG_PROFILE_FUNCTION();

		_shutdown();
	}

	void Window::_init()
	{
		MRG_PROFILE_FUNCTION();

		MRG_ENGINE_INFO("Creating window {0} ({1}x{2})", m_properties->title, m_properties->width, m_properties->height);

		if (s_GLFWWindowCount == 0) {
			MRG_PROFILE_SCOPE("glfwInit");
			MRG_ENGINE_INFO("Initializing GLFW");
			[[maybe_unused]] auto success = glfwInit();
			MRG_CORE_ASSERT(success, "Could not initialize GLFW !");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		{
			MRG_PROFILE_SCOPE("glfwCreateWindow")
#ifdef MRG_DEBUG
			if (Renderer::getAPI() == RenderingAPI::API::OpenGL)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
			if (Renderer::getAPI() == RenderingAPI::API::Vulkan)
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			m_window = glfwCreateWindow(m_properties->width, m_properties->height, m_properties->title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;
		}

		glfwSetWindowUserPointer(m_window, m_properties.get());

		m_context = Context::create(m_window);

		setVsync(m_properties->VSync);

		// GLFW callbacks
		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
			const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));
			data->width = width;
			data->height = height;

			WindowResizeEvent resize{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
			data->callback(resize);
		});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
			const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));

			WindowCloseEvent close{};
			data->callback(close);
		});

		glfwSetKeyCallback(
		  m_window,
		  [](GLFWwindow* window, int keycode, [[maybe_unused]] int scancode, [[maybe_unused]] int action, [[maybe_unused]] int mods) {
			  const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));

			  switch (action) {
			  case GLFW_PRESS: {
				  KeyPressedEvent press{static_cast<KeyCode>(keycode), 0};
				  data->callback(press);
			  } break;

			  case GLFW_REPEAT: {
				  KeyPressedEvent press{static_cast<KeyCode>(keycode), ++data->keyRepeats};
				  data->callback(press);
			  } break;

			  case GLFW_RELEASE: {
				  data->keyRepeats = 0;
				  KeyReleasedEvent release{static_cast<KeyCode>(keycode)};
				  data->callback(release);
			  } break;

			  default: {
				  MRG_ENGINE_WARN("Ignoring unrecognized GLFW key event (type {})", action);
			  } break;
			  }
		  });

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, [[maybe_unused]] int mods) {
			const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));

			switch (action) {
			case GLFW_PRESS: {
				MouseButtonPressedEvent press{static_cast<MouseCode>(button)};
				data->callback(press);
			} break;

			case GLFW_RELEASE: {
				MouseButtonReleasedEvent release{static_cast<MouseCode>(button)};
				data->callback(release);
			} break;

			default: {
				MRG_ENGINE_WARN("Ignoring unrecognized GLFW mouse event (type {})", action);
			} break;
			}
		});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset) {
			const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));

			MouseScrolledEvent scroll{static_cast<float>(xOffset), static_cast<float>(yOffset)};
			data->callback(scroll);
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos) {
			const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));

			MouseMovedEvent move{static_cast<float>(xPos), static_cast<float>(yPos)};
			data->callback(move);
		});

		glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int codePoint) {
			const auto data = static_cast<WindowProperties*>(glfwGetWindowUserPointer(window));

			KeyTypedEvent typedChar{static_cast<KeyCode>(codePoint)};
			data->callback(typedChar);
		});
	}

	void Window::_shutdown()
	{
		MRG_PROFILE_FUNCTION();

		glfwDestroyWindow(m_window);

		--s_GLFWWindowCount;
		if (s_GLFWWindowCount == 0) {
			MRG_ENGINE_INFO("Terminating GLFW");
#ifdef MRG_PLATFORM_WINDOWS
			// For some reason, that causes an exception on linux. This is most likely a GLFW bug.
			glfwTerminate();
#endif
		}
	}

	void Window::onUpdate()
	{
		MRG_PROFILE_FUNCTION();

		glfwPollEvents();
		m_context->swapBuffers();
	}

	void Window::setVsync(bool enabled)
	{
		MRG_PROFILE_FUNCTION();

		if (enabled) {
			m_context->swapInterval(1);
		} else {
			m_context->swapInterval(0);
		}

		m_properties->VSync = enabled;
	}

}  // namespace MRG