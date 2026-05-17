#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cmath>
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
//  Preview shader sources
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
    FragColor = vec4(0.9, 0.4, 0.3, 1.0);
}
)";

// =====================================================
//  Mouse picking helpers
// =====================================================
struct ViewportCamera {
  glm::vec3 eye;
  glm::vec3 lookat;
  glm::vec3 up;
  float vfov_deg;
};

struct Ray3 {
  glm::vec3 origin;
  glm::vec3 dir;
};

// Construct a world-space ray from a mouse pixel inside the preview panel.
// Mouse coords are panel-local with origin at the top-left (ImGui convention).
static Ray3 ray_from_mouse(const ViewportCamera &c, float mx, float my, int w,
                           int h) {
  glm::vec3 forward = glm::normalize(c.lookat - c.eye);
  glm::vec3 right = glm::normalize(glm::cross(forward, c.up));
  glm::vec3 up = glm::cross(right, forward);

  float aspect = (float)w / (float)h;
  float h_world = 2.0f * std::tan(glm::radians(c.vfov_deg) * 0.5f);
  float w_world = h_world * aspect;

  float ndc_x = (mx / (float)w) * 2.0f - 1.0f;
  float ndc_y = 1.0f - (my / (float)h) * 2.0f; // flip y: image is top-down

  glm::vec3 dir = glm::normalize(forward +
                                 (ndc_x * w_world * 0.5f) * right +
                                 (ndc_y * h_world * 0.5f) * up);
  return {c.eye, dir};
}

// Standard analytic ray-sphere intersection: returns true if the ray hits.
static bool ray_hits_sphere(const Ray3 &r, const glm::vec3 &C, float radius) {
  glm::vec3 oc = r.origin - C;
  float a = glm::dot(r.dir, r.dir);
  float b = 2.0f * glm::dot(oc, r.dir);
  float c = glm::dot(oc, oc) - radius * radius;
  return (b * b - 4.0f * a * c) > 0.0f;
}

// Intersect a ray with the plane defined by point P and normal N.
static glm::vec3 ray_plane_intersect(const Ray3 &r, const glm::vec3 &P,
                                     const glm::vec3 &N) {
  float t = glm::dot(P - r.origin, N) / glm::dot(r.dir, N);
  return r.origin + t * r.dir;
}

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
  //  OpenGL sphere preview setup
  // =====================================================
  SphereMesh previewMesh = CreateSphereMesh(30, 30);
  GLuint sphereVAO = previewMesh.vao;
  GLuint sphereIndexCount = previewMesh.indexCount;

  GLuint previewShader = createShaderProgram(preview_vs_src, preview_fs_src);

  // =====================================================
  //  Raytracer texture
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
  //  FBO for OpenGL preview panel
  // =====================================================
  int preview_w = 640;
  int preview_h = 480;

  GLuint previewFBO, previewColor, previewDepth;
  glGenFramebuffers(1, &previewFBO);
  glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);

  glGenTextures(1, &previewColor);
  glBindTexture(GL_TEXTURE_2D, previewColor);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, preview_w, preview_h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         previewColor, 0);

  glGenRenderbuffers(1, &previewDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, previewDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, preview_w,
                        preview_h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, previewDepth);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Preview framebuffer is not complete!\n");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // =====================================================
  //  Build raytracer scene
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
  //  GUI / interaction state
  // =====================================================
  float sphere_x = 0.0f;
  float sphere_y = 1.0f;
  float sphere_z = 0.0f;
  const float sphere_radius = 1.0f;

  // Preview camera (matches the lookAt below). Kept in one place so the
  // picking math and the draw both use the same numbers.
  const ViewportCamera viewportCam = {
      glm::vec3(5.0f, 3.0f, 5.0f),  // eye
      glm::vec3(0.0f, 1.0f, 0.0f),  // lookat
      glm::vec3(0.0f, 1.0f, 0.0f),  // world up
      45.0f                          // vertical FOV in degrees
  };

  bool dragging_sphere = false;

  // =====================================================
  //  Main loop
  // =====================================================
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // --- Raytrace on demand ---
    if (need_rerender) {
      printf("Raytracing...\n");
      render_rt_scene(scene, cam, pixels.data(), rt_width, rt_height);

      glBindTexture(GL_TEXTURE_2D, raytex);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rt_width, rt_height, GL_RGBA,
                      GL_UNSIGNED_BYTE, pixels.data());

      need_rerender = false;
    }

    // --- Render preview into FBO ---
    glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
    glViewport(0, 0, preview_w, preview_h);

    glClearColor(0.10f, 0.10f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 proj = glm::perspective(glm::radians(viewportCam.vfov_deg),
                                      (float)preview_w / (float)preview_h,
                                      0.1f, 100.0f);
    glm::mat4 view =
        glm::lookAt(viewportCam.eye, viewportCam.lookat, viewportCam.up);
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(sphere_x, sphere_y, sphere_z));
    glm::mat4 mvp = proj * view * model;

    glUseProgram(previewShader);
    glUniformMatrix4fv(glGetUniformLocation(previewShader, "MVP"), 1, GL_FALSE,
                       &mvp[0][0]);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);

    glDisable(GL_DEPTH_TEST);

    // --- Back to default framebuffer ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int win_w, win_h;
    glfwGetFramebufferSize(window, &win_w, &win_h);
    glViewport(0, 0, win_w, win_h);

    glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // =====================================================
    //  ImGui frame
    // =====================================================
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- Controls ---
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

    // --- OpenGL Preview window with mouse picking ---
    ImGui::Begin("OpenGL Preview");

    // Reserve the image rectangle with an InvisibleButton so it captures
    // mouse input; otherwise the click would fall through to the window
    // and ImGui would drag the panel instead.
    ImVec2 imgMin = ImGui::GetCursorScreenPos();
    ImVec2 imgSize((float)preview_w, (float)preview_h);

    ImGui::InvisibleButton("##preview_image", imgSize);
    bool itemHovered = ImGui::IsItemHovered();

    // Draw the texture into that same rectangle via the draw list.
    ImGui::GetWindowDrawList()->AddImage(
        (void *)(intptr_t)previewColor, imgMin,
        ImVec2(imgMin.x + imgSize.x, imgMin.y + imgSize.y),
        ImVec2(0, 1), ImVec2(1, 0));

    // Panel-local mouse coords (only meaningful when hovering the image).
    ImVec2 mousePos = ImGui::GetMousePos();
    float mx = mousePos.x - imgMin.x;
    float my = mousePos.y - imgMin.y;

    // Click on the sphere -> start dragging.
    if (!dragging_sphere && itemHovered && ImGui::IsMouseClicked(0)) {
      Ray3 r = ray_from_mouse(viewportCam, mx, my, preview_w, preview_h);
      glm::vec3 C(sphere_x, sphere_y, sphere_z);
      if (ray_hits_sphere(r, C, sphere_radius)) {
        dragging_sphere = true;
      }
    }

    // While dragging: move the sphere on the camera-aligned plane through
    // its current position. Update the preview every frame, but defer the
    // (slow) raytracer re-render until the mouse is released.
    if (dragging_sphere) {
      if (ImGui::IsMouseDown(0)) {
        Ray3 r = ray_from_mouse(viewportCam, mx, my, preview_w, preview_h);
        glm::vec3 forward =
            glm::normalize(viewportCam.lookat - viewportCam.eye);
        glm::vec3 plane_point(sphere_x, sphere_y, sphere_z);
        glm::vec3 new_center = ray_plane_intersect(r, plane_point, forward);

        // Keep within the slider range so the UI stays consistent.
        new_center.x = glm::clamp(new_center.x, -5.0f, 5.0f);
        new_center.y = glm::clamp(new_center.y, -5.0f, 5.0f);
        new_center.z = glm::clamp(new_center.z, -5.0f, 5.0f);

        sphere_x = new_center.x;
        sphere_y = new_center.y;
        sphere_z = new_center.z;
        sphere_obj->set_center(sphere_x, sphere_y, sphere_z);
      } else {
        dragging_sphere = false;
        need_rerender = true;
      }
    }

    ImGui::End();

    // --- Raytraced result ---
    ImGui::Begin("Raytraced Image");
    ImGui::Image((void *)(intptr_t)raytex, ImVec2(rt_width, rt_height));
    ImGui::End();

    // --- Submit ---
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  // Cleanup
  glDeleteProgram(previewShader);
  glDeleteFramebuffers(1, &previewFBO);
  glDeleteTextures(1, &previewColor);
  glDeleteRenderbuffers(1, &previewDepth);

  return 0;
}
