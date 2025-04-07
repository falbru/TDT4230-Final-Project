// Local headers
#include "program.hpp"
#include "gamelogic.h"
#include "glad/glad.h"
#include "utilities/window.hpp"
#include <glm/glm.hpp>
// glm::translate, glm::rotate, glm::scale, glm::perspective
#include <SFML/Audio.hpp>
#include <SFML/System/Time.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <utilities/glutils.h>
#include <utilities/shader.hpp>
#include <utilities/shapes.h>
#include <utilities/timeutils.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void runProgram(GLFWwindow *window, CommandLineOptions options) {
  // Enable depth (Z) buffer (accept "closest" fragment)
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  // Configure miscellaneous OpenGL settings
  glEnable(GL_CULL_FACE);

  // Disable built-in dithering
  glDisable(GL_DITHER);

  // Enable transparency
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set default colour after clearing the colour buffer
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  // Setup Dear ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontFromFileTTF("../res/Inter.ttf", 20);
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init();

  initGame(window, options);

  // Rendering Loop
  while (!glfwWindowShouldClose(window)) {
    // Clear colour and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Handle other events
    glfwPollEvents();
    handleKeyboardInput(window);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(); // Show demo window! :)

    updateFrame(window);
    renderFrame(window);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Flip buffers
    glfwSwapBuffers(window);
  }
}

void handleKeyboardInput(GLFWwindow *window) {
  // Use escape key for terminating the GLFW window
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}
