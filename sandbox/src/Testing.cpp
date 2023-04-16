#include "Shado.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "script/LuaScript.h"

using namespace Shado;

void createSphere(unsigned int slices, unsigned int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices, std::vector<float>& texCoords) {
	for (unsigned int stack = 0; stack <= stacks; ++stack) {
		float stackAngle = stack * glm::pi<float>() / stacks - glm::pi<float>() / 2.0f;
		float y = std::sin(stackAngle);
		float stackRadius = std::cos(stackAngle);
		for (unsigned int slice = 0; slice <= slices; ++slice) {
			float sliceAngle = slice * 2.0f * glm::pi<float>() / slices;
			float x = stackRadius * std::cos(sliceAngle);
			float z = stackRadius * std::sin(sliceAngle);

			// Vertex position
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			// Vertex normal
			glm::vec3 normal(x, y, z);
			normal = glm::normalize(normal);
			vertices.push_back(normal.x);
			vertices.push_back(normal.y);
			vertices.push_back(normal.z);

			// Vertex texture coordinates
			float u = float(slice) / float(slices);
			float v = float(stack) / float(stacks);
			texCoords.push_back(u);
			texCoords.push_back(v);
		}
	}

	for (unsigned int stack = 0; stack < stacks; ++stack) {
		for (unsigned int slice = 0; slice < slices; ++slice) {
			unsigned int topLeft = stack * (slices + 1) + slice;
			unsigned int topRight = topLeft + 1;
			unsigned int bottomLeft = topLeft + slices + 1;
			unsigned int bottomRight = bottomLeft + 1;

			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}
}




class CelestialBody {
public:
	void onInit(glm::vec3 position,std::string texture_path) {
		this->position = position;
		this->Texture = CreateRef<Texture2D>(texture_path);
		// Create sphere vertices and indices
		std::vector<float> vertices;
		std::vector<float> texCoords;
		createSphere(50, 50, vertices, indices,texCoords);

		
		// Set up the VAO and VBO
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		// Vertex texture coordinates
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);


		// Unbind the VAO
		glBindVertexArray(0);
		

	}
	void setScale(float scale) {
		this->scale = scale;
	}
	void onUpdate(glm::vec3 camPos, Shader* shader) const {
		shader->bind();
		Texture->bind(0);
		// Set up the view and projection matrices
		glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), Application::get().getWindow().getAspectRatio(), 0.1f, 100.0f);

		// Set up the model matrix
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, position);
		model = glm::scale(model, glm::vec3(scale));
		shader->setMat4("view", view);
		shader->setMat4("projection", projection);
		shader->setMat4("model", model);
		//shader->setFloat3("lightPos", { 5.0f,5.0f,5.0f });
		//shader->setFloat3("viewPos", camPos);
		shader->setInt("objectTexture", 0);
		// Bind the VAO and draw the sphere
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

		Texture->unbind();

	}


private:
	unsigned int VAO, VBO, EBO;
	std::vector<unsigned int> indices;
	glm::vec3 position;
	float scale = 1.0f;
	Ref<Texture2D> Texture;

};

class TestScene : public Scene {
public:
	TestScene() :
		Scene("Test scene"),
		camera(Application::get().getWindow().getAspectRatio())
	{

	}

	virtual ~TestScene() {
		
	}

	void onInit() override {
		
		shader = new Shader("celestialbody.glsl");
		glEnable(GL_DEPTH_TEST);

		// Initialize celestial bodies
		CelestialBody sun, mercury, venus, earth, mars, jupiter, saturn, uranus, neptune;

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
	}

	void onUpdate(TimeStep dt) override {
    camera.onUpdate(dt);
    /*float x = cameraDistance * sin(glm::radians(45.0f)) * cos(glm::radians(cameraHorizontalAngle));
    float y = 0.0f;
    float z = cameraDistance * cos(glm::radians(45.0f)) * sin(glm::radians(cameraHorizontalAngle));
    glm::vec3 camPos(x, y, z);

    for (const auto& p : celestialBodies) {
        p.onUpdate(camPos, shader);
    }*/
}


	void onDraw() override {
		float x = cameraDistance * sin(glm::radians(45.0f)) * cos(glm::radians(cameraHorizontalAngle));
		float y = 0.0f;
		float z = cameraDistance * cos(glm::radians(45.0f)) * sin(glm::radians(cameraHorizontalAngle));
		glm::vec3 camPos(x, y, z);

		for (const auto& p : celestialBodies) {
			p.onUpdate(camPos, shader);
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
	std::vector<CelestialBody> celestialBodies;
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
