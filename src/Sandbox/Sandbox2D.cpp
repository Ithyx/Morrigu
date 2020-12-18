#include "Sandbox2D.h"

Macha::Macha() : MRG::Layer("Sandbox 2D"), m_camera(1280.f / 720.f) {}

void Macha::onAttach()
{
	MRG_PROFILE_FUNCTION();

	m_checkerboard = MRG::Texture2D::create("resources/textures/Checkerboard.png");
	m_camera.movementSpeed = 2.f;
}

void Macha::onDetach() { MRG_PROFILE_FUNCTION(); }

void Macha::onUpdate(MRG::Timestep ts)
{
	MRG_PROFILE_FUNCTION();

	m_frameTime = ts;

	m_camera.onUpdate(ts);

	MRG::Renderer2D::resetStats();

	{
		MRG_PROFILE_SCOPE("Render prep");
		MRG::Renderer2D::setClearColor(m_color);
		MRG::Renderer2D::clear();
	}

	{
		static float rotation = 0.f;
		rotation += ts * 50.f;
		if (rotation >= 360)
			rotation -= 360;

		MRG_PROFILE_SCOPE("Render draw");
		MRG::Renderer2D::beginScene(m_camera.getCamera());
		MRG::Renderer2D::drawRotatedQuad({1.0f, 0.0f}, {0.8f, 0.8f}, -45.0f, {0.8f, 0.2f, 0.3f, 1.0f});
		MRG::Renderer2D::drawQuad({-1.0f, 0.0f}, {0.8f, 0.8f}, {0.8f, 0.2f, 0.3f, 1.0f});
		MRG::Renderer2D::drawQuad({0.5f, -0.5f}, {0.5f, 0.75f}, {0.2f, 0.3f, 0.8f, 1.0f});
		MRG::Renderer2D::drawQuad({0.0f, 0.0f, -0.1f}, {20.0f, 20.0f}, m_checkerboard, 10.0f);
		MRG::Renderer2D::drawRotatedQuad({-2.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, glm::radians(rotation), m_color);
		MRG::Renderer2D::endScene();

		MRG::Renderer2D::beginScene(m_camera.getCamera());
		for (float y = -5.f; y < 5.f; y += 0.5f) {
			for (float x = -5.f; x < 5.f; x += 0.5f) {
				glm::vec4 color = {(x + 5.f) / 10.f, 0.4f, (y + 5.f) / 10.f, 0.7f};
				MRG::Renderer2D::drawQuad({x, y}, {0.45f, 0.45f}, color);
			}
		}
		MRG::Renderer2D::endScene();
	}
}

void Macha::onImGuiRender()
{
	MRG_PROFILE_FUNCTION();

	const auto fps = 1 / m_frameTime;
	const auto color = (fps < 30) ? ImVec4{1.f, 0.f, 0.f, 1.f} : ImVec4{0.f, 1.f, 0.f, 1.f};
	const auto stats = MRG::Renderer2D::getStats();

	ImGui::Begin("Debug");
	ImGui::Text("Renderer2D stats:");
	ImGui::Text("Draw calls: %d", stats.drawCalls);
	ImGui::Text("Quads: %d", stats.quadCount);
	ImGui::Text("Vertices: %d", stats.getVertexCount());
	ImGui::Text("Indices: %d", stats.getIndexCount());
	ImGui::Separator();
	ImGui::TextColored(color, "Frametime: %04.4f ms (%04.2f FPS)", m_frameTime.getMillieconds(), fps);
	ImGui::ColorEdit4("Shader color", glm::value_ptr(m_color));
	ImGui::End();
}

void Macha::onEvent(MRG::Event& event) { m_camera.onEvent(event); }
