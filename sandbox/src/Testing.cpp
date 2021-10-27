﻿#include "Shado.h"
#include "ShadoScript.h"
#include <iostream>

#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_world.h"

using namespace Shado;

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

class TestScene : public Scene {
public:
	TestScene() :
		Scene("Test scene"),
		camera(Application::get().getWindow().getAspectRatio()),
		orthoCamera(Application::get().getWindow().getAspectRatio()),
		world({ 0.0f, -1.0f })
	{
	}
	
	virtual ~TestScene() {}

	void onInit() override {
		/*EntityDefinition def;
		def.type = EntityType::DYNAMIC;
		def.texture = riven2;
		def.scale = { 0.25, 0.25 };

		for (float y = -1.5f; y < 1.5f; y += 0.3f)
			for (float x = -1.5f; x < 1.5f; x += 0.3f) {
				def.position = { x, y, -2 };
				objects.push_back({def, world});
			}
		*/

		EntityDefinition def2;
		def2.type = EntityType::DYNAMIC;
		def2.scale = { 0.45f, 0.45f };
		
		for (float y = -5.0f; y < 5.0f; y += 0.5f)
		{
			for (float x = -5.0f; x < 5.0f; x += 0.5f)
			{
				def2.color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f };
				def2.position = { x, y, -15 };
				objects.push_back({ def2 , world});
			}
		}


		EntityDefinition groundDef;
		groundDef.scale = { 10.0, 0.3 };
		groundDef.position = { 0, -0.75, 0 };

		objects.emplace_back(groundDef, world);
		
	}

	void onUpdate(TimeStep dt) override {
		camera.onUpdate(dt);
		orthoCamera.onUpdate(dt);
		
		world.Step(dt, 6, 2);
	}

	void onDraw() override {

		Renderer2D::BeginScene(orthoCamera.getCamera());

		for (const auto& object : objects) {
			object.draw();
		}

		Renderer2D::EndScene();
	}

	void onDestroy() override {}

	void onEvent(Event& e) override {

		camera.onEvent(e);
		orthoCamera.onEvent(e);

		EventDispatcher dispatcher(e);
		dispatcher.dispatch<KeyPressedEvent>([&](KeyPressedEvent& event) {

			if (event.getKeyCode() == SHADO_KEY_ESCAPE) {
				Application::get().getWindow().setMode(WindowMode::WINDOWED);
			}

			return false;
		});


		dispatcher.dispatch<MouseButtonPressedEvent>([&](MouseButtonPressedEvent& event) {
			Window& win = Application::get().getWindow();
			
			if (event.getMouseButton() == SHADO_MOUSE_BUTTON_LEFT) {
				float mapX = map(Input::getMouseX(), 0, win.getWidth(), -10, 10);
				float mapY = map(Input::getMouseY(), 0, win.getHeight(), -10, 10);
				
				EntityDefinition def2;
				def2.type = EntityType::DYNAMIC;
				def2.scale = { 0.45f, 0.45f };
				def2.position = {mapX, -mapY, 0};
				
				objects.emplace_back(def2, world);
			}
			return false;
		});
	}

	void onImGuiRender() override {
		ImGui::SliderFloat4("Sphere Colour", (float*)&color, 0, 1.0f);
		ImGui::SliderInt("Tiling factor", &tileFactor, 1, 100);
	}

private:
	Ref<Texture2D> riven1 = CreateRef<Texture2D>("assets/riven3.png");
	Ref<Texture2D> riven2 = CreateRef<Texture2D>("assets/riven2.jpg");

	b2World world;
	std::vector<Entity> objects;

	OrbitCameraController camera;
	OrthoCameraController orthoCamera;
	Color color = { 1, 1, 1 };
	int tileFactor = 1;
};

int main(int argc, const char** argv)
{
	auto& application = Application::get();
	application.getWindow().resize(1920, 1080);
	application.submit(new TestScene);
	application.run();

	Application::destroy();
}
