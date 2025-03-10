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
uniform bool enabledAtmosphere;

const float PI = 3.14159;

layout(binding = 0) uniform sampler2D sampler;

float raySphereIntersect(vec3 r0, vec3 rd, vec3 s0, float sr) {
    float a = dot(rd, rd);
    vec3 s0_r0 = r0 - s0;
    float b = 2.0 * dot(rd, s0_r0);
    float c = dot(s0_r0, s0_r0) - (sr * sr);
    float discriminant = b*b - 4.0*a*c;

    if (discriminant < 0.0) {
        return -1.0;
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float t1 = (-b - sqrtDiscriminant) / (2.0 * a);
    float t2 = (-b + sqrtDiscriminant) / (2.0 * a);

    return (c < 0.0) ? t2 : t1;
}

void main() {
  if (!enabledAtmosphere) {
    color = texture(sampler, textureCoordinates);
    return;
  }
  vec3 invWaveLength = vec3(1.0 / pow(waveLengths.x, 4), 1.0 / pow(waveLengths.y, 4), 1.0 / pow(waveLengths.z, 4));

  float radiusScale = 1.0 / (atmosphereRadius - planetRadius);
  float scaleOverScaleDepth = radiusScale / scaleDepth;

  vec3 ray = position.xyz - cameraPosition;
  float far = length(ray);
  ray /= far;

  float near = raySphereIntersect(cameraPosition, ray, planetPosition, atmosphereRadius);
  vec3 start = cameraPosition + ray * near;
  far -= near;
  float depth = exp((planetRadius - atmosphereRadius) / scaleDepth);

  float sunRayLength = raySphereIntersect(position.xyz, sunDirection, planetPosition, atmosphereRadius);
  float cameraRayLength = raySphereIntersect(position.xyz, -ray, planetPosition, atmosphereRadius);

  float cameraOffset = depth * (sunRayLength - cameraRayLength);

  float sampleLength = far / fSamples;
  float scaledLength = sampleLength * radiusScale;
  vec3 sampleRay = ray * sampleLength;
  vec3 samplePoint = start + sampleRay * 0.5;

  vec3 scatteringColor = vec3(0.0);
  vec3 attenuate = vec3(0.0);
  for (int i = 0; i < nSamples; i++) {
    float height = length(samplePoint - planetPosition);

    float h = (height - planetRadius) / (atmosphereRadius - planetRadius);
    float depth = exp(-h/scaleDepth);

    float sunRayLength = raySphereIntersect(samplePoint, sunDirection, planetPosition, atmosphereRadius);
    float cameraRayLength = raySphereIntersect(samplePoint, -ray, planetPosition, atmosphereRadius);
    float scatter = (cameraOffset + depth*(sunRayLength - cameraRayLength));

    float planetRayLength = raySphereIntersect(samplePoint, sunDirection, planetPosition, planetRadius);
    if (planetRayLength > 0.0) {
      continue;
    }

    attenuate += exp(-scatter * (invWaveLength * Kr * 4 * PI + Km * 4 * PI));
    scatteringColor += attenuate * (depth * sampleLength / (atmosphereRadius - planetRadius));
    samplePoint += sampleRay;
  }

  color = texture(sampler, textureCoordinates);
  color.rgb = color.rgb * attenuate / fSamples;
  color.rgb += scatteringColor * (invWaveLength * Kr * ESun + Km*ESun) * 0.1 / fSamples;
  color.a = 1.0f;
}
