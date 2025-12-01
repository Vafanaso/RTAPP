#ifndef OPENGL_SPHERE_H
#define OPENGL_SPHERE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp> // <-- REQUIRED for glm::pi and glm::two_pi

#include <vector>

struct SphereMesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  GLsizei indexCount = 0;
};

inline SphereMesh CreateSphereMesh(int stacks = 30, int slices = 30) {
  SphereMesh mesh;

  std::vector<glm::vec3> vertices;
  std::vector<unsigned int> indices;

  for (int i = 0; i <= stacks; i++) {
    float V = i / (float)stacks;
    float phi = V * glm::pi<float>();

    for (int j = 0; j <= slices; j++) {
      float U = j / (float)slices;
      float theta = U * glm::two_pi<float>();

      float x = cos(theta) * sin(phi);
      float y = cos(phi);
      float z = sin(theta) * sin(phi);

      vertices.push_back(glm::vec3(x, y, z));
    }
  }

  for (int i = 0; i < stacks; i++) {
    for (int j = 0; j < slices; j++) {
      int first = (i * (slices + 1)) + j;
      int second = first + slices + 1;

      indices.push_back(first);
      indices.push_back(second);
      indices.push_back(first + 1);

      indices.push_back(second);
      indices.push_back(second + 1);
      indices.push_back(first + 1);
    }
  }

  mesh.indexCount = indices.size();

  glGenVertexArrays(1, &mesh.vao);
  glGenBuffers(1, &mesh.vbo);
  glGenBuffers(1, &mesh.ebo);

  glBindVertexArray(mesh.vao);

  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

  glBindVertexArray(0);

  return mesh;
}

#endif
