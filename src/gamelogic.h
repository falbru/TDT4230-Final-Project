#pragma once

#include <GLFW/glfw3.h>

#include "sceneGraph.hpp"
#include <utilities/window.hpp>

void updateNodeTransformations(SceneNode *node,
                               glm::mat4 transformationThusFar);
void initGame(GLFWwindow *window, CommandLineOptions options);
void updateFrame(GLFWwindow *window);
void renderFrame(GLFWwindow *window);