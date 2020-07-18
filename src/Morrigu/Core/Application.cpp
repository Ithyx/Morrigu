#include "Application.h"

#include <glad/glad.h>

#include <functional>

namespace MRG
{
	Application* Application::s_instance = nullptr;

	Application::Application()
	{
		MRG_CORE_ASSERT(s_instance == nullptr, "Application already exists !");
		s_instance = this;

		m_window = std::make_unique<Window>(WindowProperties{"Morrigu", 1280, 720, true});
		m_window->setEventCallback([this](Event& event) { onEvent(event); });
	}
	Application::~Application() { /*glfwTerminate();*/ }

	void Application::onEvent(Event& event)
	{
		EventDispatcher dispatcher{event};
		dispatcher.dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) -> bool { return onWindowClose(event); });

		for (auto it = m_layerStack.end(); it != m_layerStack.begin();) {
			(*--it)->onEvent(event);
			if (event.handled)
				break;
		}
	}

	void Application::run()
	{
		while (m_running) {
			glClearColor(1, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			for (auto& layer : m_layerStack) layer->onUpdate();
			m_window->onUpdate();
		}
	}

	void Application::pushLayer(Layer* newLayer)
	{
		m_layerStack.pushLayer(newLayer);
		newLayer->onAttach();
	}
	void Application::pushOverlay(Layer* newOverlay)
	{
		m_layerStack.pushOverlay(newOverlay);
		newOverlay->onAttach();
	}

	bool Application::onWindowClose(WindowCloseEvent& event)
	{
		m_running = false;
		return true;
	}
}  // namespace MRG