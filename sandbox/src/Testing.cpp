#include "Shado.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "script/LuaScript.h"
#include "CelestialBodyRender.h"
#include "CelestialBody.h"

using namespace Shado;

class TestScene : public Scene 
{
public:
	TestScene() : Scene("Test scene"), camera(Application::get().getWindow().getAspectRatio())
	{

	}

	virtual ~TestScene() {
		
	}

	float fraction(float a, float b)
	{
		return a / b;
	}

	void onInit() override {
		
		camera.setRotationSpeed(360);

		shader = new Shader("celestialbody.glsl");
		glEnable(GL_DEPTH_TEST);

		// On start par le soleil
		CelestialBody sun = CelestialBody("Sun", 1.9885e30, 695, fraction(1, 25.05));
		sun.InitRender("assets/sun.jpg");
		solarSystem.push_back(sun);

		// Mercury
		CelestialBody mercury = CelestialBody("Mercury", 3.3011e23, 2.439, &sun, 57909050000, 47360, 20, fraction(1, 58.646), 2.04);
		mercury.InitRender("assets/mercury.jpg");
		solarSystem.push_back(mercury);

		// Venus
		CelestialBody venus = CelestialBody("Venus", 4.8675e24, 6.0518, &sun, 108208000000, 35020, 70, fraction(1, -243.0226), 2.64);
		venus.InitRender("assets/venus.jpg");
		solarSystem.push_back(venus);

		// Earth
		CelestialBody earth = CelestialBody("Earth", 5.972168e24, 6.378137, &sun, 149598023000, 29782.7, 90, fraction(1,1), 23.4392811);
		earth.InitRender("assets/earth.jpg");
		solarSystem.push_back(earth);

		// Moon
		// don't have the right orbit :(
		/*
		CelestialBody moon = CelestialBody("Moon", 7.342e22, 1.7381, &earth, 384399000, 1022, 0, fraction(1, 27.321661), 1.5424);
		moon.InitRender("assets/mercury.jpg");	// Yep on render la lune comme mercure, c'est le plus proche
		solarSystem.push_back(moon);
		*/

		// Mars
		CelestialBody mars = CelestialBody("Mars", 6.4171e23, 3.3962, &sun, 227939366000, 24070, 150, fraction(1, 1.027), 25.19);
		mars.InitRender("assets/mars.jpg");
		solarSystem.push_back(mars);

		// Jupiter
		CelestialBody jupiter = CelestialBody("Jupiter", 1.8982e27, 71.492, &sun, 778479000000, 13070, 30, fraction(24, 9.9250), 3.13);
		jupiter.InitRender("assets/jupiter.jpg");
		solarSystem.push_back(jupiter);

		// Saturn
		CelestialBody saturn = CelestialBody("Saturn", 5.6834e26, 58.232, &sun, 1.4335366e12, 9680, -90, fraction(24, 10.5), 26.73);
		saturn.InitRender("assets/saturn.jpg");
		solarSystem.push_back(saturn);

		// Uranus
		CelestialBody uranus = CelestialBody("Uranus", 8.6810e25, 25.362, &sun, 2.87097163e12, 6800, -120, -fraction(24, 17), 97.77);
		uranus.InitRender("assets/uranus.jpg");
		solarSystem.push_back(uranus);

		// Neptune
		CelestialBody neptune = CelestialBody("Neptune", 1.02413e26, 24.622, &sun, 4.49841e12, 5430, 0, -0.6713, 28.32);
		neptune.InitRender("assets/neptune.jpg");
		solarSystem.push_back(neptune);

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

		// Le deltat, pour la simulation
		// Warning si on eval plus que 10 jours
		float deltaTime = dt * 24 * 60 * 60 *10;

		for (int x = 0; x < solarSystem.size(); x++)
		{
			solarSystem[x].UpdateForce(&solarSystem);
		}

		for (int x = 0; x < solarSystem.size(); x++)
		{
			solarSystem[x].UpdateVelocity(deltaTime);
		}

		for (int x = 0; x < solarSystem.size(); x++)
		{
			solarSystem[x].UpdatePosition(deltaTime);
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
