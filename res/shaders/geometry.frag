#version 430 core

in layout(location = 0) vec4 position;
in layout(location = 1) vec2 textureCoordinates;

out vec4 color;

uniform float planetRadius;
uniform float atmosphereRadius;
uniform vec3 planetPosition;
uniform vec3 cameraPosition;
uniform vec3 sunDirection;
uniform vec3 waveLengths;
uniform int nSamples;
uniform float fSamples;
uniform float Kr;
uniform float Km;
uniform float ESun;
uniform float g;
uniform float scaleDepth;

const float PI = 3.14159;

layout(binding = 0) uniform sampler2D sampler;

float scale(float fCos)
{
	float x = 1.0 - fCos;
	return scaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

void main() {

  vec3 invWaveLength = vec3(1.0 / pow(waveLengths.x, 4), 1.0 / pow(waveLengths.y, 4), 1.0 / pow(waveLengths.z, 4));

  float radiusScale = 1.0 / (atmosphereRadius - planetRadius);
  float scaleOverScaleDepth = radiusScale / scaleDepth;

  vec3 ray = position.xyz - cameraPosition;
  float far = length(ray);
  ray /= far;

  float cameraHeight = length(cameraPosition - planetPosition);

	float B = 2.0 * dot(cameraPosition, ray);
	float C = cameraHeight*cameraHeight - atmosphereRadius*atmosphereRadius;
	float det = max(0.0, B*B - 4.0 * C);
	float near = 0.5 * (-B - sqrt(det));

  vec3 start = cameraPosition + ray * near;
  far -= near;
  float depth = exp((planetRadius - atmosphereRadius) / scaleDepth);
  float cameraAngle = dot(-ray, position.xyz) / length(position.xyz);
  float lightAngle = dot(sunDirection, position.xyz) / length(position.xyz);
  float cameraScale = scale(cameraAngle);
  float lightScale = scale(lightAngle);
  float cameraOffset = depth * cameraScale;
  float temp = lightScale + cameraScale;

  float sampleLength = far / fSamples;
  float scaledLength = sampleLength * radiusScale;
  vec3 sampleRay = ray * sampleLength;
  vec3 samplePoint = start + sampleRay * 0.5;

  vec3 frontColor = vec3(0.0);
  vec3 attenuate = vec3(0.0);
  for (int i = 0; i < nSamples; i++) {
    float height = length(samplePoint - planetPosition);
    float depth = exp(scaleOverScaleDepth * (planetRadius - height));
    float scatter = depth * temp - cameraOffset;

    attenuate += exp(-scatter * (invWaveLength * Kr * 4 * PI + Km * 4 * PI));
    frontColor += attenuate * (depth * scaledLength);
    samplePoint += sampleRay;
  }

  vec3 primaryColor = frontColor * (invWaveLength * Kr * ESun + Km*ESun);
  vec3 secondaryColor = attenuate;

  // color.rgb = primaryColor + 0.25 * secondaryColor;
  // vec3 scatteringColor = primaryColor + 0.25 * secondaryColor;
  // scatteringColor.r = min(1.0, scatteringColor.r);
  // scatteringColor.g = min(1.0, scatteringColor.g);
  // scatteringColor.b = min(1.0, scatteringColor.b);
  color = texture(sampler, textureCoordinates);
  // color.rgb = color.rgb * secondaryColor;
  // color.rgb += primaryColor;
}
