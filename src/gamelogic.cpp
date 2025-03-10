#include "gamelogic.h"
#include "imgui.h"
#include "sceneGraph.hpp"
#include "utilities/camera.hpp"
#include "utilities/imageLoader.hpp"
#include <GLFW/glfw3.h>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <fmt/format.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include <utilities/glutils.h>
#include <utilities/mesh.h>
#include <utilities/shader.hpp>
#include <utilities/shapes.h>
#include <utilities/timeutils.h>
#define GLM_ENABLE_EXPERIMENTAL
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glm/gtx/transform.hpp>

Gloom::Camera *camera;
SceneNode *rootNode;
SceneNode *planetNode;
SceneNode *atmosphereNode;

Gloom::Shader *planetShader;
Gloom::Shader *atmopshereShader;

glm::mat4 VP;

// SIMULATION CONSTANTS
const float PI = 3.14159265359f;

const int SAMPLES = 50;
const float g = -0.5f;
const float planetRadius = 10.0;
const glm::vec3 waveLengths = glm::vec3(0.650f, 0.570f, 0.475f);

// SIMULATION OPTIONS
bool atmosphereEnabled = true;
bool sunOrbitEarth = false;
float Kr = 0.0025f;
float Km = 0.0010f;
float ESun = 10.0f;
float scaleDepth = 0.25f;
float atmosphereRadius = 10.25f;
float sunAngle = 0.0f;
float planetAngle = 343.0f / 360.0f * 2.0f * PI;
float cameraZoom = 1.0f;

void updateCameraPosition() {
  glm::vec3 startPosition = glm::vec3(0.0f, 0.0f, -planetRadius - 6.5f);
  glm::vec3 endPosition =
      glm::vec3(atmosphereRadius, 0.0f, -atmosphereRadius / 1.414 + 1.0f);

  glm::vec3 interpolatedPosition =
      startPosition + (endPosition - startPosition) * (cameraZoom - 1.0f);
  camera->setPosition(interpolatedPosition);
}

void cursorPosCallback(GLFWwindow *window, double x, double y) {
  int windowWidth, windowHeight;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  glViewport(0, 0, windowWidth, windowHeight);

  ImGuiIO &io = ImGui::GetIO();
  io.AddMousePosEvent(x, y);
}

void mouseButtonCallback(GLFWwindow *, int button, int action, int) {
  ImGuiIO &io = ImGui::GetIO();
  io.AddMouseButtonEvent(button, action);
}

unsigned int genTexture(const PNGImage &img) {
  unsigned int textureId;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width, img.height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, img.pixels.data());
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return textureId;
}

void initGame(GLFWwindow *window, CommandLineOptions gameOptions) {
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);

  planetShader = new Gloom::Shader();
  planetShader->makeBasicShader("../res/shaders/planet.vert",
                                "../res/shaders/planet.frag");
  atmopshereShader = new Gloom::Shader();
  atmopshereShader->makeBasicShader("../res/shaders/atmosphere.vert",
                                    "../res/shaders/atmosphere.frag");

  int earthTextureID = genTexture(loadPNGFile("../res/textures/earth.png"));

  Mesh sphereMesh = generateSphere(planetRadius, 100, 100);
  unsigned int sphereVAO = generateBuffer(sphereMesh);

  // Construct scene
  rootNode = createSceneNode();
  planetNode = createSceneNode();
  atmosphereNode = createSceneNode();

  rootNode->children.push_back(planetNode);
  planetNode->children.push_back(atmosphereNode);

  planetNode->vertexArrayObjectID = sphereVAO;
  planetNode->VAOIndexCount = sphereMesh.indices.size();
  planetNode->textureID = earthTextureID;

  atmosphereNode->vertexArrayObjectID = sphereVAO;
  atmosphereNode->VAOIndexCount = sphereMesh.indices.size();
  atmosphereNode->nodeType = SceneNodeType::ATMOSPHERE;

  camera = new Gloom::Camera(glm::vec3(0, 0, -planetRadius - 6.5f));
  camera->lookAt(planetNode->position);
  updateCameraPosition();

  getTimeDeltaSeconds();
}

void updateFrame(GLFWwindow *) {
  double deltaTime = getTimeDeltaSeconds();

  glm::mat4 projection =
      glm::perspective(glm::radians(80.0f),
                       float(windowWidth) / float(windowHeight), 0.1f, 350.f);

  updateCameraPosition();
  camera->updateCamera(deltaTime);

  planetNode->rotation.y = planetAngle;

  if (sunOrbitEarth) {
    sunAngle += deltaTime;
    if (sunAngle > 2 * PI) {
      sunAngle -= 2 * PI;
    }
  }

  VP = projection * camera->getViewMatrix();

  updateNodeTransformations(rootNode, glm::mat4(1.0f));
}

void updateNodeTransformations(SceneNode *node,
                               glm::mat4 transformationThusFar) {
  glm::mat4 transformationMatrix =
      glm::translate(node->position) * glm::translate(node->referencePoint) *
      glm::rotate(node->rotation.y, glm::vec3(0, 1, 0)) *
      glm::rotate(node->rotation.x, glm::vec3(1, 0, 0)) *
      glm::rotate(node->rotation.z, glm::vec3(0, 0, 1)) *
      glm::scale(node->scale) * glm::translate(-node->referencePoint);

  node->currentTransformationMatrix =
      transformationThusFar * transformationMatrix;

  for (SceneNode *child : node->children) {
    updateNodeTransformations(child, node->currentTransformationMatrix);
  }
}

void renderNode(SceneNode *node) {
  Gloom::Shader *shader;

  switch (node->nodeType) {
  case GEOMETRY:
    glBindTexture(0, node->textureID);
    shader = planetShader;
    glCullFace(GL_BACK);
    break;
  case ATMOSPHERE:
    shader = atmopshereShader;
    glCullFace(GL_FRONT);
    break;
  }

  shader->activate();

  glUniformMatrix4fv(shader->getUniformFromName("M"), 1, GL_FALSE,
                     glm::value_ptr(node->currentTransformationMatrix));

  if (node->vertexArrayObjectID != -1) {
    glBindVertexArray(node->vertexArrayObjectID);
    glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
  }

  for (SceneNode *child : node->children) {
    renderNode(child);
  }
}

void renderFrame(GLFWwindow *window) {
  int windowWidth, windowHeight;
  glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
  glViewport(0, 0, windowWidth, windowHeight);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  {
    ImGui::Begin("Simulation Options");

    if (ImGui::CollapsingHeader("Camera")) {
      ImGui::SliderFloat("Zoom", &cameraZoom, 1.0f, 2.0f);
    }

    if (ImGui::CollapsingHeader("Planet")) {
      ImGui::Checkbox("Enable atmosphere", &atmosphereEnabled);
      ImGui::SliderAngle("Planet angle", &planetAngle);

      ImGui::Text("Atmosphere constants:");

      ImGui::SliderFloat("Kr", &Kr, 0.0f, 0.005f);
      ImGui::SliderFloat("Km", &Km, 0.0f, 0.005f);
      ImGui::SliderFloat("ESun", &ESun, 0.0f, 50.0f);
      ImGui::SliderFloat("Scale Depth", &scaleDepth, 0.0f, 1.0f);
      ImGui::SliderFloat("atmosphere Depth", &atmosphereRadius, planetRadius,
                         planetRadius + 2.0f);
    }
    if (ImGui::CollapsingHeader("Sun")) {
      ImGui::Checkbox("Orbit around planet", &sunOrbitEarth);
      ImGui::SliderAngle("Sun angle", &sunAngle);
    }

    ImGui::End();
  }
  atmosphereNode->scale = glm::vec3(atmosphereRadius / planetRadius);

  glm::vec3 sunDirection = glm::vec3(cos(sunAngle), 0.0, sin(sunAngle));

  Gloom::Shader *shaders[2] = {planetShader, atmopshereShader};

  for (Gloom::Shader *shader : shaders) {
    shader->activate();

    glUniform1i(shader->getUniformFromName("nSamples"), SAMPLES);
    glUniform1f(shader->getUniformFromName("fSamples"), (float)SAMPLES);
    glUniform1f(shader->getUniformFromName("Kr"), Kr);
    glUniform1f(shader->getUniformFromName("Km"), Km);
    glUniform1f(shader->getUniformFromName("ESun"), ESun);
    glUniform1f(shader->getUniformFromName("g"), g);
    glUniform1f(shader->getUniformFromName("scaleDepth"), scaleDepth);

    glUniformMatrix4fv(shader->getUniformFromName("VP"), 1, GL_FALSE,
                       glm::value_ptr(VP));
    glUniform1i(shader->getUniformFromName("enabledAtmosphere"),
                atmosphereEnabled);
    glUniform3fv(shader->getUniformFromName("planetPosition"), 1,
                 glm::value_ptr(planetNode->position));
    glUniform1f(shader->getUniformFromName("planetRadius"), planetRadius);
    glUniform1f(shader->getUniformFromName("atmosphereRadius"),
                atmosphereRadius);
    glUniform3fv(shader->getUniformFromName("cameraPosition"), 1,
                 glm::value_ptr(camera->getPosition()));
    glUniform3fv(shader->getUniformFromName("sunDirection"), 1,
                 glm::value_ptr(sunDirection));
    glUniform3fv(shader->getUniformFromName("waveLengths"), 1,
                 glm::value_ptr(waveLengths));
  }

  renderNode(rootNode);
}
