#include "Shado.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "script/LuaScript.h"
#include "../CelestialBodyRender.h"
#include "../CelestialBody.h"

using namespace Shado;

class TestScene : public Scene 
{
public:
	TestScene() : Scene("Test scene"), camera(Application::get().getWindow().getAspectRatio())
	{

	}

	virtual ~TestScene() {
		
	}


	void onInit() override {
		
		shader = new Shader("celestialbody.glsl");
		glEnable(GL_DEPTH_TEST);

		// On start par le soleil
		CelestialBody sun = CelestialBody("sun", 19885000000, 695, 25);
		sun.InitRender("assets/sun.jpg");
		solarSystem.push_back(sun);

		// Mercury
		//CelestialBody mercury = CelestialBody("mercury", 3301.1, 2.439, &sun, 57.909050, 4.060800, 0.2, (float)1/(float)176, 2.04);
		CelestialBody mercury = CelestialBody("mercury", 3301.1, 2.439, &sun, 57.909050, 4.060800, 0.2, (float)1 / (float)176, 2.04);
		mercury.InitRender("assets/mercury.jpg");
		solarSystem.push_back(mercury);

		// Old code

		/*
		// Initialize celestial bodies
		CelestialBodyRender sun, mercury, venus, earth, mars, jupiter, saturn, uranus, neptune;

		sun.onInit({ -6.0f, 0.0f, 0.0f },"assets/sun.jpg"); 
		sun.setScale(1.0f);
		mercury.onInit({ -3.0f, 0.0f, 0.0f }, "assets/mercury.jpg");
		mercury.setScale(0.0553f);
		venus.onInit({ -2.0f, 0.0f, 0.0f }, "assets/venus.jpg");
		venus.setScale(0.0866f);
		earth.onInit({ -1.0f, 0.0f, 0.0f }, "assets/earth.jpg");
		earth.setScale(0.0913f);
		mars.onInit({ 0.0f, 0.0f, 0.0f }, "assets/mars.jpg");
		mars.setScale(0.0486f);
		jupiter.onInit({ 1.5f, 0.0f, 0.0f }, "assets/jupiter.jpg");
		jupiter.setScale(0.25f);
		saturn.onInit({ 3.0f, 0.0f, 0.0f }, "assets/saturn.jpg");
		saturn.setScale(0.20f);
		uranus.onInit({ 4.0f, 0.0f, 0.0f }, "assets/uranus.jpg");
		uranus.setScale(0.1f);
		neptune.onInit({ 5.0f, 0.0f, 0.0f }, "assets/neptune.jpg");
		neptune.setScale(0.1f);

		celestialBodies.push_back(sun);
		celestialBodies.push_back(mercury);
		celestialBodies.push_back(venus);
		celestialBodies.push_back(earth);
		celestialBodies.push_back(mars);
		celestialBodies.push_back(jupiter);
		celestialBodies.push_back(saturn);
		celestialBodies.push_back(uranus);
		celestialBodies.push_back(neptune);
		*/
	}

	void onUpdate(TimeStep dt) override 
	{
		camera.onUpdate(dt);
		/*float x = cameraDistance * sin(glm::radians(45.0f)) * cos(glm::radians(cameraHorizontalAngle));
		float y = 0.0f;
		float z = cameraDistance * cos(glm::radians(45.0f)) * sin(glm::radians(cameraHorizontalAngle));
		glm::vec3 camPos(x, y, z);

		for (const auto& p : celestialBodies) {
			p.onUpdate(camPos, shader);
		}*/

		for (int x = 0; x < solarSystem.size(); x++)
		{
			solarSystem[x].UpdateForce(&solarSystem);
		}

		for (int x = 0; x < solarSystem.size(); x++)
		{
			solarSystem[x].UpdateVelocity(dt);
		}

		for (int x = 0; x < solarSystem.size(); x++)
		{
			solarSystem[x].UpdatePosition(dt);
		}
	}


	void onDraw() override {
		float x = cameraDistance * sin(glm::radians(45.0f)) * cos(glm::radians(cameraHorizontalAngle));
		float y = 0.0f;
		float z = cameraDistance * cos(glm::radians(45.0f)) * sin(glm::radians(cameraHorizontalAngle));
		glm::vec3 camPos(x, y, z);

		for (auto& p : solarSystem) 
		{
			p.UpdateRender(camPos, shader);

			cout << p.name << " :" << endl << "(" << (p.currentPosition.x) << ',' << (p.currentPosition.y) << ',' << (p.currentPosition.z) << ")" << endl << endl;
		}
	}


	void onDestroy() override {
		
	}

	void onEvent(Event& e) override {

		camera.onEvent(e);
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) {
			if (event.getKeyCode() == SHADO_KEY_W) {
				cameraDistance -= 0.5f;
			}
			else if (event.getKeyCode() == SHADO_KEY_S) {
				cameraDistance += 0.5f;
			}
			else if (event.getKeyCode() == SHADO_KEY_A) {
				cameraHorizontalAngle -= 0.05f;
			}
			else if (event.getKeyCode() == SHADO_KEY_D) {
				cameraHorizontalAngle += 0.05f;
			}
			return false;
			});
	}

	void onImGuiRender() override {

		
	}


private:

	OrbitCameraController camera;

	Shader* shader;

	//std::vector<CelestialBodyRender> celestialBodies;

	std::vector<CelestialBody> solarSystem;

	float zoomLevel = 10.0f;
	float cameraDistance = 25.0f;
	float cameraHorizontalAngle = 90.0f;
};



int main(int argc, const char** argv)
{
	auto& application = Application::get();
	application.getWindow().resize(1920, 1080);
	application.submit(new TestScene);
	application.run();

	Application::destroy();
}
