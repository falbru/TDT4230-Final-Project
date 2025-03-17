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

Gloom::Shader *shader;

CommandLineOptions options;

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

  shader = new Gloom::Shader();
  shader->makeBasicShader("../res/shaders/simple.vert",
                          "../res/shaders/simple.frag");
  shader->activate();

  Mesh planetMesh = generateSphere(1.0, 40, 40);
  unsigned int planetVAO = generateBuffer(planetMesh);

  // Construct scene
  rootNode = createSceneNode();
  planetNode = createSceneNode();

  rootNode->children.push_back(planetNode);

  planetNode->vertexArrayObjectID = planetVAO;
  planetNode->VAOIndexCount = planetMesh.indices.size();

  camera = new Gloom::Camera(glm::vec3(0, 0, -10));
  camera->lookAt(planetNode->position);

  getTimeDeltaSeconds();
}

void updateFrame(GLFWwindow *window) {
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double deltaTime = getTimeDeltaSeconds();

  glm::mat4 projection =
      glm::perspective(glm::radians(80.0f),
                       float(windowWidth) / float(windowHeight), 0.1f, 350.f);

  camera->updateCamera(deltaTime);

  glm::mat4 VP = projection * camera->getViewMatrix();

  updateNodeTransformations(rootNode, VP);
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
  glUniformMatrix4fv(3, 1, GL_FALSE,
                     glm::value_ptr(node->currentTransformationMatrix));

  switch (node->nodeType) {
  case GEOMETRY:
    if (node->vertexArrayObjectID != -1) {
      glBindVertexArray(node->vertexArrayObjectID);
      glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT,
                     nullptr);
    }
    break;
  }

  for (SceneNode *child : node->children) {
    renderNode(child);
  }
}

void renderFrame(GLFWwindow *window) {
  int windowWidth, windowHeight;
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  glViewport(0, 0, windowWidth, windowHeight);

  renderNode(rootNode);
}
