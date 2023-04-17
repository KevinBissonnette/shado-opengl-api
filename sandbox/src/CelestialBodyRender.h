#pragma once

#include "Shado.h"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "script/LuaScript.h"

using namespace Shado;

class CelestialBodyRender {
public:

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

	void onInit(glm::vec3 position, std::string texture_path) {
		this->position = position;
		this->Texture = CreateRef<Texture2D>(texture_path);
		// Create sphere vertices and indices
		std::vector<float> vertices;
		std::vector<float> texCoords;
		createSphere(50, 50, vertices, indices, texCoords);


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

