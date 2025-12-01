#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glad/glad.h>
#include <vector>

// ==== OpenGL preview ====
#include "opengl_sphere.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ==== ImGui ====
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

// ==== Raytracer includes ====
#include "../raytracer/hittable_list.h"
#include "../raytracer/material.h"
#include "../raytracer/rt_engine.h"
#include "../raytracer/rtweekend.h"
#include "../raytracer/sphere.h"

// =====================================================
//   MAIN
// =====================================================

// =====================================================
//  OpenGL Preview Shader Sources
// =====================================================
const char *preview_vs_src = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}
)";

const char *preview_fs_src = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(0.9, 0.4, 0.3, 1.0);  // sphere color
}
)";

int main() {
  // ----------------------
  // Init GLFW
  // ----------------------
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "Raytracer GUI", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // ----------------------
  // Init GLAD
  // ----------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    printf("Failed to load GLAD\n");
    return -1;
  }

  // ----------------------
  // Init ImGui
  // ----------------------
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::StyleColorsDark();

  // =====================================================
  //  OPENGL SPHERE PREVIEW SETUP
  // =====================================================
  SphereMesh previewMesh = CreateSphereMesh(30, 30);
  GLuint sphereVAO = previewMesh.vao;
  GLuint sphereIndexCount = previewMesh.indexCount;

  GLuint previewShader = createShaderProgram(preview_vs_src, preview_fs_src);

  // =====================================================
  //  RAYTRACER TEXTURE
  // =====================================================
  int rt_width = 400;
  int rt_height = 225;

  GLuint raytex;
  std::vector<uint32_t> pixels(rt_width * rt_height);

  glGenTextures(1, &raytex);
  glBindTexture(GL_TEXTURE_2D, raytex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rt_width, rt_height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  bool need_rerender = true;

  // =====================================================
  //   BUILD RAYTRACER SCENE
  // =====================================================
  RtScene scene;

  auto groundMat = make_shared<lambertian>(color(0.5, 0.5, 0.5));
  scene.world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, groundMat));

  auto mainMat = make_shared<lambertian>(color(0.8, 0.2, 0.2));
  auto sphere_obj = make_shared<sphere>(point3(0, 1, 0), 1.0, mainMat);
  scene.world.add(sphere_obj);

  RtCameraConfig cam;
  cam.aspect_ratio = (double)rt_width / rt_height;
  cam.samples_per_pixel = 10;
  cam.max_depth = 20;
  cam.vfov = 20;
  cam.lookfrom = point3(13, 2, 3);
  cam.lookat = point3(0, 1, 0);
  cam.vup = vec3(0, 1, 0);

  // =====================================================
  //  GUI SPHERE POSITION
  // =====================================================
  static float sphere_x = 0.0f;
  static float sphere_y = 1.0f;
  static float sphere_z = 0.0f;

  // =====================================================
  //   MAIN LOOP
  // =====================================================
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // --- RAYTRACING ---
    if (need_rerender) {
      printf("Raytracing...\n");
      render_rt_scene(scene, cam, pixels.data(), rt_width, rt_height);

      glBindTexture(GL_TEXTURE_2D, raytex);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rt_width, rt_height, GL_RGBA,
                      GL_UNSIGNED_BYTE, pixels.data());

      need_rerender = false;
    }

    // =====================================================
    //  PREVIEW OPENGL RENDERING
    // =====================================================
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);

    glm::mat4 proj =
        glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);

    glm::mat4 view =
        glm::lookAt(glm::vec3(5, 3, 5), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));

    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(sphere_x, sphere_y, sphere_z));

    glm::mat4 mvp = proj * view * model;

    glUseProgram(previewShader);
    glUniformMatrix4fv(glGetUniformLocation(previewShader, "MVP"), 1, GL_FALSE,
                       &mvp[0][0]);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

    glDisable(GL_DEPTH_TEST);

    // =====================================================
    //  IMGUI FRAME
    // =====================================================
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- Controls Window ---
    ImGui::Begin("Controls");
    ImGui::Text("Move sphere:");
    bool moved = ImGui::SliderFloat("X", &sphere_x, -5.0f, 5.0f) |
                 ImGui::SliderFloat("Y", &sphere_y, -5.0f, 5.0f) |
                 ImGui::SliderFloat("Z", &sphere_z, -5.0f, 5.0f);

    if (moved) {
      sphere_obj->set_center(sphere_x, sphere_y, sphere_z);
      need_rerender = true;
    }

    if (ImGui::Button("Re-render")) {
      need_rerender = true;
    }

    ImGui::End();

    // --- Raytraced result window ---
    ImGui::Begin("Raytraced Image");
    ImGui::Image((void *)(intptr_t)raytex, ImVec2(rt_width, rt_height));
    ImGui::End();

    // --- Draw ImGui ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  glDeleteProgram(previewShader);

  return 0;
}
