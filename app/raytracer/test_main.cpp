#include "material.h"
#include "rt_engine.h"
#include "rtweekend.h"
#include "sphere.h"
#include <fstream>
#include <vector>

int main() {
  int width = 400;
  int height = 225;

  std::vector<uint32_t> pixels(width * height);

  // Create scene
  RtScene scene;

  // Ground
  auto ground_mat = make_shared<lambertian>(color(0.5, 0.5, 0.5));
  scene.world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_mat));

  // Main sphere
  auto mat = make_shared<lambertian>(color(0.7, 0.3, 0.3));
  scene.world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, mat));

  // Camera config
  RtCameraConfig cam;
  cam.aspect_ratio = double(width) / height;
  cam.samples_per_pixel = 10;
  cam.max_depth = 20;

  cam.vfov = 20;
  cam.lookfrom = point3(13, 2, 3);
  cam.lookat = point3(0, 0, 0);
  cam.vup = vec3(0, 1, 0);

  cam.defocus_angle = 0.0;
  cam.focus_dist = 10.0;

  // Render
  render_rt_scene(scene, cam, pixels.data(), width, height);

  // Save PPM
  std::ofstream out("test.ppm");
  out << "P3\n" << width << " " << height << "\n255\n";
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      uint32_t c = pixels[j * width + i];
      int r = (c >> 16) & 255;
      int g = (c >> 8) & 255;
      int b = (c)&255;
      out << r << " " << g << " " << b << "\n";
    }
  }

  return 0;
}
