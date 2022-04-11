#include "Shado.h"
#include "panels/SceneHierarchyPanel.h"

namespace Shado {


	class EditorLayer : public Layer {
	public:
		EditorLayer();
		~EditorLayer() override;
		void onInit() override;
		void onUpdate(TimeStep dt) override;
		void onDraw() override;
		void onImGuiRender() override;
		void onDestroy() override;
		void onEvent(Event& event) override;


	private:
		OrthoCameraController m_camera_controller;

		bool m_viewportFocused = false, m_viewportHovered = false;
		glm::vec2 m_ViewportSize = {0, 0};
		Ref<FrameBuffer> buffer;

		ImVec2 m_viewportPanelSize;

		// Debug
		Entity m_Square;
		Entity m_Camera;
		Entity m_CameraSecondary;
		Ref<Scene> m_ActiveScene;

		// Panels
		SceneHierarchyPanel m_sceneHierarchyPanel;
	};


}
