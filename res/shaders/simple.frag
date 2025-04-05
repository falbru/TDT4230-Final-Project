#version 430 core

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;
in layout(location = 2) vec4 position;

out vec4 color;

uniform float planetRadius;
uniform float atmosphereRadius;
uniform vec3 planetPosition;
uniform vec3 cameraPosition;
uniform vec3 sunDirection;

void main() {
  color = vec4(0.5 * normal + 0.5, 1.0);
}
