#include "Sandbox2D.h"

class VulkanSandbox : public MRG::Layer
{
public:
	VulkanSandbox() : Layer("Vulkan layer"), m_camera(1280.f / 720.f) {}

	void onAttach() override
	{
		m_checkerboard = MRG::Texture2D::create("resources/textures/Checkerboard.png");
		m_character = MRG::Texture2D::create("resources/textures/Character.png");
	}
	void onDetach() override {}
	void onUpdate(MRG::Timestep ts) override
	{
		m_camera.onUpdate(ts);

		if (MRG::Input::isKeyDown(MRG::Key::Space))
			MRG_INFO("FPS: {}", 1 / ts);

		MRG::Renderer2D::beginScene(m_camera.getCamera());
		MRG::Renderer2D::drawQuad({0.f, 0.f, -0.1f}, {10.f, 10.f}, m_checkerboard, 10.f);
		MRG::Renderer2D::drawRotatedQuad({-1.f, 0.f}, {0.8f, 0.8f}, glm::radians(-45.f), {0.8f, 0.2f, 0.3f, 1.f});
		MRG::Renderer2D::drawQuad({0.5f, -0.5f}, {1.0f, 1.0f}, m_character, 1.f);
		MRG::Renderer2D::endScene();
	}
	void onEvent(MRG::Event& event) override { m_camera.onEvent(event); }

private:
	MRG::OrthoCameraController m_camera;
	MRG::Ref<MRG::Texture2D> m_checkerboard, m_character;
};

class Sandbox : public MRG::Application
{
public:
	Sandbox() { pushLayer(new VulkanSandbox{}); }
	~Sandbox() {}
};

MRG::Application* MRG::createApplication() { return new Sandbox(); }
