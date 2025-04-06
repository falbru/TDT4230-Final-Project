#include "gamelogic.h"
#include "sceneGraph.hpp"
#include "utilities/camera.hpp"
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
#include <glm/gtx/transform.hpp>

Gloom::Camera *camera;

SceneNode *rootNode;
SceneNode *planetNode;

const int SAMPLES = 10;
const float Kr = 0.0025f;
const float Km = 0.0010f;
const float ESun = 20.0f;
const float g = 0;
const float scaleDepth = 0.5f;
float planetRadius = 10.0;
float atmosphereRadius = 10.25;
glm::vec3 waveLengths = glm::vec3(0.650f, 0.570f, 0.475f);
glm::vec3 sunDirection = glm::vec3(1.0f, 0.0f, 0.0f);

Gloom::Shader *geometryShader;
Gloom::Shader *skyFromSpaceShader;
Gloom::Shader *skyFromAtmosphereShader;

CommandLineOptions options;

glm::mat4 VP;

void cursorPosCallback(GLFWwindow *window, double x, double y) {
  int windowWidth, windowHeight;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  glViewport(0, 0, windowWidth, windowHeight);

  camera->handleCursorPosInput(x, y);
}

void mouseButtonCallback(GLFWwindow *, int button, int action, int) {
  camera->handleMouseButtonInputs(button, action);
}

void keyCallback(GLFWwindow *, int key, int, int action, int) {
  camera->handleKeyboardInputs(key, action);
}

void initGame(GLFWwindow *window, CommandLineOptions gameOptions) {
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetKeyCallback(window, keyCallback);

  geometryShader = new Gloom::Shader();
  geometryShader->makeBasicShader("../res/shaders/geometry.vert",
                                  "../res/shaders/geometry.frag");
  skyFromSpaceShader = new Gloom::Shader();
  skyFromSpaceShader->makeBasicShader("../res/shaders/skyFromSpace.vert",
                                      "../res/shaders/skyFromSpace.frag");
  skyFromAtmosphereShader = new Gloom::Shader();
  skyFromAtmosphereShader->makeBasicShader(
      "../res/shaders/skyFromAtmosphere.vert",
      "../res/shaders/skyFromAtmosphere.frag");

  Mesh planetMesh = generateSphere(planetRadius, 40, 40);
  unsigned int planetVAO = generateBuffer(planetMesh);

  Mesh atmosphereMesh = generateSphere(atmosphereRadius, 40, 40);
  unsigned int atmosphereVAO = generateBuffer(atmosphereMesh);

  // Construct scene
  rootNode = createSceneNode();
  planetNode = createSceneNode();
  SceneNode *atmosphereNode = createSceneNode();

  rootNode->children.push_back(planetNode);
  rootNode->children.push_back(atmosphereNode);

  planetNode->vertexArrayObjectID = planetVAO;
  planetNode->VAOIndexCount = planetMesh.indices.size();

  atmosphereNode->vertexArrayObjectID = atmosphereVAO;
  atmosphereNode->VAOIndexCount = atmosphereMesh.indices.size();
  atmosphereNode->nodeType = SceneNodeType::ATMOSPHERE;

  camera = new Gloom::Camera(glm::vec3(0, 0, -20));
  camera->lookAt(planetNode->position);

  getTimeDeltaSeconds();
}

void updateFrame(GLFWwindow *window) {
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double deltaTime = getTimeDeltaSeconds();

  static double time = 0.0;
  time += deltaTime;

  sunDirection.x = cos(time * 0.1);
  sunDirection.z = sin(time * 0.1);

  glm::mat4 projection =
      glm::perspective(glm::radians(80.0f),
                       float(windowWidth) / float(windowHeight), 0.1f, 350.f);

  camera->updateCamera(deltaTime);

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

  switch (node->nodeType) {
  case GEOMETRY:
    break;
  }

  for (SceneNode *child : node->children) {
    updateNodeTransformations(child, node->currentTransformationMatrix);
  }
}

void renderNode(SceneNode *node) {
  Gloom::Shader *shader;

  switch (node->nodeType) {
  case GEOMETRY:
    shader = geometryShader;
    glCullFace(GL_BACK);
    break;
  case ATMOSPHERE:
    shader = (glm::length(camera->getPosition()) > atmosphereRadius)
                 ? skyFromSpaceShader
                 : skyFromAtmosphereShader;
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
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  glViewport(0, 0, windowWidth, windowHeight);

  Gloom::Shader *shaders[3] = {geometryShader, skyFromSpaceShader,
                               skyFromAtmosphereShader};

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
