#type vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    // Apply position uniform to the vertex position
    vec4 newPos = vec4(aPos, 1.0);
    
    gl_Position = projection * view * model * newPos;
    FragPos = vec3(model * newPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
}

#type fragment
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D objectTexture;

void main() {
    // Sample the texture using the texture coordinates
    vec3 texColor = texture(objectTexture, TexCoord).rgb;

    // Output the final color
    FragColor = vec4(texColor, 1.0);
}
