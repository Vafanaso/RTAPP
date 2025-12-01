#ifndef SHADER_H
#define SHADER_H

#include <cstdio>
#include <glad/glad.h>

inline GLuint compileSingleShader(GLenum type, const char *src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, NULL);
  glCompileShader(s);

  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(s, 512, NULL, log);
    printf("Shader compile error:\n%s\n", log);
  }
  return s;
}

inline GLuint createShaderProgram(const char *vs, const char *fs) {
  GLuint vert = compileSingleShader(GL_VERTEX_SHADER, vs);
  GLuint frag = compileSingleShader(GL_FRAGMENT_SHADER, fs);

  GLuint prog = glCreateProgram();
  glAttachShader(prog, vert);
  glAttachShader(prog, frag);
  glLinkProgram(prog);

  glDeleteShader(vert);
  glDeleteShader(frag);

  return prog;
}

#endif
