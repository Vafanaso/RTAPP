#pragma once
#include "rtweekend.h"

#include "camera.h"
#include "hittable_list.h"
#include "rtweekend.h"
#include <cstdint>

struct RtScene {
  hittable_list world;
};

struct RtCameraConfig {
  point3 lookfrom;
  point3 lookat;
  vec3 vup;
  double vfov;
  double aspect_ratio;

  int samples_per_pixel;
  int max_depth;
  double defocus_angle;
  double focus_dist;
};

/**
 * Renders a scene into a pixel buffer (32-bit ARGB).
 * pixels must have size width * height.
 */
void render_rt_scene(const RtScene &scene, const RtCameraConfig &camCfg,
                     std::uint32_t *pixels, int width, int height);
