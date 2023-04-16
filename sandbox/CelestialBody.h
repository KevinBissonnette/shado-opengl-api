#pragma once

#include <iostream>
#include <string>
#include <glm/glm.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <math.h>
#include <vector>
#include <Shado.h>
#include <Texture2D.h>
using namespace Shado;

using namespace std;
using namespace glm;

class CelestialBody
{
public:
	// ===============================
	//			Constant
	// ===============================

	const float PI = 3.14159265359;

	const float G = 0.00000000006674 * 24 * 60 * 60;

	const float BASE_SIZE = 1;

	// ===============================
	//			Variable
	// ===============================

	// ===== ] Display [ =====

	// The name of the celestial body, used mostly for display
	string name = "russel teapot";

	// Render Section
	unsigned int VAO, VBO, EBO;
	std::vector<unsigned int> indices;
	Ref<Texture2D> Texture;

	// ===== ] Config [ =====

	// The object mass, in 10^20 kg
	float mass = 1;

	// The orbital radius, in million kilometer
	float semiMajorAxis = 100;

	// The orbital speed, in million of km/day
	float orbitalSpeed = 2.5;

	// The celestial body radius, in thousand of Km
	// It will not be rendered to scale
	float radius = 6.37;

	// The angle made by the orbiting body, in radian
	// To prevent all of them alinging and slingshoting the sun or something
	float orbitAngle = 0;

	// Nombre de rotation sur sois m�me en une journ�e
	float siderealRotationPeriod = 0.99726968;

	// En degr�e, a quelle point l'axe de rotation est pench�, comparativement au soleil
	float axialTilt = 23.5;

	// The body that this celestial body is orbiting
	// Be sur that it is initialized before this one !
	// Will be place at (0,0,0) if empty
	CelestialBody* parentBody;

	// ===== ] Inner Variable [ =====

	// The amount of force, for the current frame
	// Not sure about the unit yet... giganewton aka million of kilonewton ?
	// Anyway, since all our units are on the same scale, it shouldn't be an issue (famous last words)
	vec3 currentForce = vec3(0, 0, 0);;

	// The velocity, for the current frame, in million of km/day
	vec3 currentVelocity = vec3(0, 0, 0);

	// The position, for the current frame, in million of km
	vec3 currentPosition = vec3(0, 0, 0);;

	// The rotation, for the current frame, in degree
	float currentRotation = 0;

	// The multiplier on the mesh size that will be aplied when dispay
	float displaySize = 1;

	vec3 rotationalAxis = vec3(0, 1, 0);

	// ===============================
	//			Function
	// ===============================

	// ===== ] Constructeur [ =====

	CelestialBody();
	CelestialBody(string name, float mass, float radius, float siderealRotationPeriod);
	CelestialBody(string name, float mass, float radius, CelestialBody* parentBody, float semiMajorAxis, float orbitalSpeed, float orbitAngle, float siderealRotationPeriod, float axialTilt);

	void InitRender(string texture_path);

	// ===== ] Updates [ =====

	void UpdateForce(vector<CelestialBody>* solarSystem);
	void UpdateVelocity(float deltaT);
	void UpdatePosition(float deltaT);
	void UpdateRender(vec3 camPos, Shader* shader);

	// ===== ] Render [ =====

	mat4 GetTransformationMatrix();

	// ===== ] Utils [ =====

	float GetDisplaySize(float radius);
	float degreeToRad(float degree);
	vec3 GetGravitationalForce(CelestialBody* cb);

	void createSphere(unsigned int slices, unsigned int stacks, vector<float>& vertices, vector<unsigned int>& indices, vector<float>& texCoords);
};
