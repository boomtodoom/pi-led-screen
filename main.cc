#include <Magick++.h>
#include <magick/image.h>
#include <glob.h>
#include "led-matrix.h"
#include "graphics.h"
#include <signal.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <dirent.h>
#include <iostream>
#include <algorithm>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

/* Draws an image to the screen pixel by pixel, and pixels outside the 
image width but within the screen width are set to black to prevent shadowing issues. */
void drawImage(const Magick::Image &image, FrameCanvas *canvas){
  const int width = canvas->width();
  const int height = canvas->height();
  const int imageWidth = image.columns();
  const int imageHeight = image.rows();

  for (int x = 0; x < width; ++x){
    for (int y = 0; y < height; ++y){
      if (x < imageWidth && y < imageHeight){
        Magick::ColorRGB color = image.pixelColor(x, y);
        canvas->SetPixel(x, y, color.red() * 255, color.green() * 255, color.blue() * 255);
      } else {
        canvas->SetPixel(x, y, 0, 0, 0);
      }
    }
  }
}

std::vector<std::string> get_image_files(const std::string &folder_path) {
  std::vector<std::string> files;
  DIR *dir;
  struct dirent *ent;

  if ((dir = opendir(folder_path.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      std::string file_name = ent->d_name;
      if (file_name.find(".bmp") != std::string::npos || file_name.find(".jpg") != std::string::npos) {
        files.push_back(folder_path + "/" + file_name);
      }
    }
    closedir(dir);
  } else {
    perror("Could not open directory");
  }

  // Sort the files by their names
  std::sort(files.begin(), files.end());

  return files;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <image-directory>\n", argv[0]);
    return 1;
  }

  const char *folder_path = argv[1];
  std::vector<std::string> image_files = get_image_files(folder_path);

  if (image_files.empty()) {
    fprintf(stderr, "No images found in directory: %s\n", folder_path);
    return 1;
  }

  RGBMatrix::Options matrix_options;
  RuntimeOptions runtime_options;

  // Set defaults
  matrix_options.chain_length = 1;
  matrix_options.parallel = 1;
  matrix_options.hardware_mapping = "regular";
  matrix_options.rows = 64;
  matrix_options.cols = 128;
  matrix_options.show_refresh_rate = true;
  matrix_options.disable_hardware_pulsing = true;
  matrix_options.brightness = 100;
  matrix_options.pwm_bits = 11;
  runtime_options.drop_privileges = 1;
  runtime_options.gpio_slowdown = 5;

  if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_options)) {
    rgb_matrix::PrintMatrixFlags(stderr);
    return 1;
  }

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_options);
  if (matrix == NULL) {
    return 1;
  }

  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
  if (offscreen_canvas == NULL) {
    delete matrix;
    return 1;
  }

  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  while (!interrupt_received) {
    for (const auto &file_path : image_files) {
      try {
        Magick::Image image;
        image.read(file_path);
        drawImage(image, offscreen_canvas);
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
        std::cout << "Displayed image: " << file_path << std::endl;
        usleep(1000000); // Display each image for 1 second
      } catch (Magick::Exception &error) {
        fprintf(stderr, "Error loading image %s: %s\n", file_path.c_str(), error.what());
      }
    }
  }

  // delete offscreen_canvas;
  delete matrix;

  return 0;
}