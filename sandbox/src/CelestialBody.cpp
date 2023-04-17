#include "CelestialBody.h"

CelestialBody::CelestialBody()
{
    // Please don't use this
}

CelestialBody::CelestialBody(string name, double mass, float radius, float siderealRotationPeriod)
{
    // C'est le constructeur pour le soleil, on a donc pas a géré l'orbite, car on est au centre du systeme solaire

    // On commence pas set les variable qu'on nous as passé
    this->name = name;
    this->mass = mass * FACTOR;
    this->radius = radius;
    this->siderealRotationPeriod = siderealRotationPeriod;

    // Le reste des variable orbital est set a 0
    // Techniquement useless, car elle serve juste au setup, mais sa peut être utilie si on décide de les calculer pour les display
    this->semiMajorAxis = 0;
    this->orbitalSpeed = 0;
    this->orbitAngle = 0;
    this->axialTilt = 0;

    // On calcule la taile a affichier
    // Pourquoi une variable quand on peut call la function ?
    // Pas éval un log a chaque frame, that why mate
    this->displaySize = GetDisplaySize(this->radius);

    // On calcul l'axe orbital
    // Et oui, le soleil tourne sur lui-même. Spin to win
    this->rotationalAxis = vec3(0, 1, 0);

    // On init maintenant la velocité et positon
    // Soit 0 pour toute
    this->currentVelocity = vec3(0, 0, 0);
    this->currentPosition = vec3(0, 0, 0);
}

CelestialBody::CelestialBody(string name, double mass, float radius, CelestialBody* parentBody, double semiMajorAxis, double orbitalSpeed, float orbitAngle, float siderealRotationPeriod, float axialTilt)
{
    // C'est le constructeur pour les diverse planete/planete naine/lune/astéroide/comet/whatever

    // On commence pas set les variable qu'on nous as passé
    this->name = name;
    this->mass = mass * FACTOR;
    this->radius = radius;
    this->parentBody = parentBody;
    this->semiMajorAxis = semiMajorAxis * FACTOR;
    this->orbitalSpeed = orbitalSpeed * FACTOR;
    this->orbitAngle = orbitAngle;
    this->siderealRotationPeriod = siderealRotationPeriod;

    // On calcule la taile a affichier
    // Pourquoi une variable quand on peut call la function ?
    // Think Mark Think ! Why call a log at each frame when you can call it once and just be done with it !
    this->displaySize = GetDisplaySize(this->radius);

    // Maintenant la section difficle, trouver la position et velocité initial

    // Les x sont donner par le sin de l'angle, alors que les z sont par le cos, tout les 2 multiplier par le semiMajorAxis
    vec3 relativePosition = vec3(sin(radians(orbitAngle)), 0, cos(radians(orbitAngle))) * this->semiMajorAxis;

    // La postion relative est aussi notre normal, on peut donc trouver la tangente, soi la direction pour la vélocité
    // Dans notre cas, car on est en 2d-ish, on peut just ajouté 90 degrée pour le trouver
    float tanRotation = degreeToRad(90);
    vec3 relativeVelocity = vec3(sin(radians(orbitAngle) + tanRotation), 0, cos(radians(orbitAngle) + tanRotation)) * this->orbitalSpeed;

    // On a désormais la position et vélocité relative au parent
    // Pour obtenir la postion/velocité absolut, il faut l'additionner a celle du parent
    // C'est pour cela qui est néssésaire que les parent soit init correctement avant l'enfant

    this->currentPosition = parentBody->currentPosition + relativePosition;
    this->currentVelocity = parentBody->currentVelocity + relativeVelocity;

    // On calcul l'axe orbital
    // On le fait ici car on a besoin de la vélocité
    // C'est vers le haut, avec une rotation de axialTilt selon la normal de l'orbite
    // No idea se que sa implique coté saison
    mat4 rotMat = rotate(axialTilt, normalize(relativePosition));

    // On applique la matrice de rotation a un vecteur vers le haut et voila !
    this->rotationalAxis = vec3(vec4(0, 1, 0, 0) * rotMat);
}

void CelestialBody::InitRender(string texture_path)
{
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

void CelestialBody::UpdateForce(vector<CelestialBody>* solarSystem)
{
    // Cette function calcule la force gravitationnel a partir des autres corps céleste

    // On commence par reset la force, elle a été utilisé dans le tick précédent
    this->currentForce = vec3(0, 0, 0);

    // Calcule de la force d'attraction gravitationelle
    for (int x = 0; x < solarSystem->size(); x++)
    {
        // Car la liste inclue tout les astres, on fait sur de ne pas s'inclure soit même

        // On fetch le corps celeste
        CelestialBody cb = (*solarSystem)[x];

        // On valide que on essait pas de calculer la gravité sur nous-même
        // Non seulement sa fait pas de sens, mais sa fait aussi une division par 0
        if (cb.name != this->name)
        {
            // On délegue le calcule a une function
            this->currentForce += GetGravitationalForce(&cb);
        }
    }
}

void CelestialBody::UpdateVelocity(float deltaT)
{
    // Function pour mettre a jour la velocité

    // C'est le même calcule pour litéralement tout object newtonien
    // Et même la, c'est juste l'accélération qui change...

    // On obtient d'abort l'accélération
    // F = M * A, du coup, A = F / M
    vec3 acceleration = this->currentForce / this->mass;

    // V = Vi + A*T
    this->currentVelocity = this->currentVelocity + acceleration * deltaT;
}

void CelestialBody::UpdatePosition(float deltaT)
{
    // Function pour mettre a jour la position

    // C'est le même calcule pour litéralement tout object newtonien

    // D = Di + V*T
    this->currentPosition = this->currentPosition + this->currentVelocity * deltaT;

    // On Maj aussi la rotation ici
    // Pas de calcul physique avec la vélocité angulaire, on fait juste spinner a une vitesse constante

    currentRotation += siderealRotationPeriod * deltaT;

    // Pour évité des erreur de float, on s'assure que la rotation reste entre -180 et et 180
    // Car on peut théoriquement faire plusieur rotation dans une update, on utilise des whiles
    // Over engenering ? Non, c'est du futurproofing
    while (currentRotation < -180)
    {
        currentRotation += 360;
    }
    while (currentRotation > 180)
    {
        currentRotation -= 360;
    }
}

void CelestialBody::UpdateRender(vec3 camPos, Shader* shader)
{
    shader->bind();
    Texture->bind(0);
    // Set up the view and projection matrices
    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), Application::get().getWindow().getAspectRatio(), 0.1f, 100.0f);

    // Set up the model matrix

    glm::mat4 model = GetTransformationMatrix();
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
double map(double x, double in_min, double in_max, double out_min, double out_max);
mat4 CelestialBody::GetTransformationMatrix()
{
    // On applique les transformation dans l'ordre suivant
    // Taille * Tranlation * Rotation
    
    glm::vec3 tempPos = { map(currentPosition.x, 0.0f, 1.0e20, 0.0f, 10.0f),map(currentPosition.y, 0.0f, 1.0e20, 0.0f, 10.0f),map(currentPosition.z, 0.0f, 1.0e20, 0.0f, 10.0f) };
    return /*mat4(displaySize) * */translate(mat4(1.0f),tempPos) * rotate(this->currentRotation, this->rotationalAxis) *scale(glm::mat4(1.0f),glm::vec3(1));
}

float CelestialBody::GetDisplaySize(float radius)
{
    return log(radius) * BASE_SIZE;
}

float CelestialBody::degreeToRad(float degree)
{
    return degree * PI / 180;
}

vec3 CelestialBody::GetGravitationalForce(CelestialBody* cb)
{
    vec3 detla = this->currentPosition - cb->currentPosition;
    vec3 dir = -normalize(detla);
    float r = length(detla);

    vec3 force = G * ((this->mass * cb->mass) / (r * r)) * dir;

    return force;
}

void CelestialBody::createSphere(unsigned int slices, unsigned int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices, std::vector<float>& texCoords)
{
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

double map(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
