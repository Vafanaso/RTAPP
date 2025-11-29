#include "rt_engine.h"
#include "color.h"
#include "material.h"
#include "rtweekend.h"
#include "sphere.h"
#include <algorithm>

inline double clamp_double(double x, double min_val, double max_val) {
  if (x < min_val)
    return min_val;
  if (x > max_val)
    return max_val;
  return x;
}

// Renders RTIOW image into ARGB pixel buffer
void render_rt_scene(const RtScene &scene, const RtCameraConfig &camCfg,
                     std::uint32_t *pixels, int width, int height) {
  camera cam;

  // Copy settings
  cam.aspect_ratio = camCfg.aspect_ratio;
  cam.image_width = width;
  cam.samples_per_pixel = camCfg.samples_per_pixel;
  cam.max_depth = camCfg.max_depth;

  cam.vfov = camCfg.vfov;
  cam.lookfrom = camCfg.lookfrom;
  cam.lookat = camCfg.lookat;
  cam.vup = camCfg.vup;

  cam.defocus_angle = camCfg.defocus_angle;
  cam.focus_dist = camCfg.focus_dist;

  // MUST be public (otherwise make it public)
  cam.initialize();

  int image_height = int(width / cam.aspect_ratio);
  if (image_height < 1)
    image_height = 1;

  // MAIN RENDER LOOP
  for (int j = 0; j < image_height; j++) {
    for (int i = 0; i < width; i++) {

      color pixel_color(0, 0, 0);
      for (int s = 0; s < cam.samples_per_pixel; s++) {
        ray r = cam.get_ray(i, j);
        pixel_color += cam.ray_color(r, cam.max_depth, scene.world);
      }

      // Gamma correct and scale by sample count
      double scale = 1.0 / cam.samples_per_pixel;

      double r = sqrt(scale * pixel_color.x());
      double g = sqrt(scale * pixel_color.y());
      double b = sqrt(scale * pixel_color.z());

      int ir = static_cast<int>(256 * clamp_double(r, 0.0, 0.999));
      int ig = static_cast<int>(256 * clamp_double(g, 0.0, 0.999));
      int ib = static_cast<int>(256 * clamp_double(b, 0.0, 0.999));

      pixels[j * width + i] = (255u << 24) | (ir << 16) | (ig << 8) | ib;
    }
  }
}
