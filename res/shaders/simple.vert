#version 430 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;

uniform mat4 M;
uniform mat4 VP;

void main() {
  normal_out = normal_in;
  textureCoordinates_out = textureCoordinates_in;
  gl_Position = VP * M * vec4(position, 1.0f);
}
