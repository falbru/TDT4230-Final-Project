#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

uniform mat4 M;
uniform mat4 VP;

void main() {
  gl_Position = VP * M * vec4(position, 1.0f);;
}
