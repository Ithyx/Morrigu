//
// Created by Mathis Lamidey on 2021-04-03.
//

#include <imgui.h>
#include <Morrigu.h>

struct GradientUniform
{
	alignas(16) glm::vec3 colorA;
	alignas(16) glm::vec3 colorB;
};

class SampleLayer : public MRG::Layer
{
public:
	void onAttach() override
	{
		mainCamera.position    = {0.f, 0.f, 3.f};
		mainCamera.aspectRatio = 1280.f / 720.f;
		mainCamera.setPerspective(glm::radians(70.f), 0.1f, 200.f);
		mainCamera.recalculateViewProjection();

		const auto testShader   = application->renderer.createShader("TestShader.vert.spv", "TestShader.frag.spv");
		const auto testMaterial = application->renderer.createMaterial<MRG::TexturedVertex>(testShader);

		m_testQuad = application->renderer.createRenderObject(MRG::Utils::Meshes::sphere<MRG::TexturedVertex>(), testMaterial);
		m_suzanne = application->renderer.createRenderObject(MRG::Utils::Meshes::loadMeshFromFile<MRG::TexturedVertex>("monkey_smooth.obj"),
		                                                     application->renderer.defaultTexturedMaterial);
		m_boxy =
		  application->renderer.createRenderObject(MRG::Utils::Meshes::loadMeshFromFile<MRG::TexturedVertex>("boxy.obj"), testMaterial);

		m_suzanne->translate({1.5f, 0.f, 0.f});
		m_suzanne->setVisible(false);
		m_boxy->translate({-1.5f, -1.5f, 0.f});
		m_boxy->setVisible(false);
		m_boxy->rotate({0.f, 1.f, 0.f}, glm::radians(90.f));

		application->renderer.uploadMesh(m_testQuad->mesh);
		application->renderer.uploadMesh(m_suzanne->mesh);
		application->renderer.uploadMesh(m_boxy->mesh);

		renderables.emplace_back(m_testQuad->getRenderData());
		renderables.emplace_back(m_suzanne->getRenderData());
		renderables.emplace_back(m_boxy->getRenderData());
	}

	void onDetach() override { MRG_INFO("my final message. goodb ye") }
	void onUpdate(MRG::Timestep) override { application->renderer.clearColor.b = std::abs(std::sin(application->elapsedTime * 3.14f)); }
	void onImGuiUpdate(MRG::Timestep) override { ImGui::ShowDemoWindow(); }
	void onEvent(MRG::Event& event) override
	{
		MRG::EventDispatcher dispatcher{event};
		dispatcher.dispatch<MRG::WindowResizeEvent>([this](MRG::WindowResizeEvent& event) { return mainCamera.onResize(event); });
		dispatcher.dispatch<MRG::KeyReleasedEvent>([this](MRG::KeyReleasedEvent&) {
			m_suzanne->setVisible(false);
			m_boxy->setVisible(false);
			m_testQuad->setVisible(true);
			return true;
		});
		dispatcher.dispatch<MRG::KeyPressedEvent>([this](MRG::KeyPressedEvent&) {
			m_suzanne->setVisible(true);
			m_boxy->setVisible(true);
			m_testQuad->setVisible(false);
			return true;
		});
	}

private:
	MRG::Ref<MRG::RenderObject<MRG::TexturedVertex>> m_suzanne{}, m_boxy{}, m_testQuad{};
};

class SampleApp : public MRG::Application
{
public:
	SampleApp()
	    : MRG::Application(MRG::ApplicationSpecification{.windowName            = "Morrigu sample app",
	                                                     .rendererSpecification = {.applicationName      = "SampleApp",
	                                                                               .windowWidth          = 1280,
	                                                                               .windowHeight         = 720,
	                                                                               .preferredPresentMode = vk::PresentModeKHR::eMailbox}})
	{
		pushLayer(new SampleLayer());
	};
};

int main()
{
	MRG::Logger::init();
	SampleApp application{};

	application.run();
}
