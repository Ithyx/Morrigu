#include <Morrigu.h>

namespace MRG
{
	class MachaLayer : public Layer
	{
	public:
		MachaLayer();
		virtual ~MachaLayer() = default;

		void onAttach() override;
		void onDetach() override;

		void onUpdate(Timestep ts) override;
		void onImGuiRender() override;
		void onEvent(Event& event) override;

	private:
		OrthoCameraController m_camera;

		Ref<Texture2D> m_checkerboard;

		bool m_viewportFocused = false, m_viewportHovered = false;
		glm::vec2 m_viewportSize = {0.f, 0.f};

		Ref<Scene> m_activeScene;
		entt::entity m_squareEntity;

		Timestep m_frameTime;

		Ref<Framebuffer> m_renderTarget;
	};
}  // namespace MRG
