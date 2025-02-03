#include "led-matrix.h"
#include "graphics.h"
#include <signal.h>
#include <unistd.h>
#include <Magick++.h>
#include <magick/image.h>
#include <glob.h>

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
  const int imageHeight = image.rows();
  const int imageWidth = image.columns();

  for (int x = 0; x < width; ++x){
    for (int y = 0; y < height; ++y){
      if (x < imageWidth && y < imageHeight){
        Magick::ColorRGB color = image.pixelColor(x, y);
        canvas->SetPixel(x, y, color.red(), color.green(), color.blue());
      } else {
        canvas->SetPixel(x, y, 0, 0, 0);
      }
    }
  }
}

void load_images_from_dir(const std::string &dir, std::vector<Magick::Image> &images){
  Magick::InitializeMagick(NULL);
  Magick::Image image;
  Magick::Blob blob;
  std::vector<std::string> files;
  std::string path = dir + "/*";
  glob_t glob_result;
  glob(path.c_str(), GLOB_TILDE, NULL, &glob_result);

  for (unsigned int i = 0; i < glob_result.gl_pathc; ++i){
    files.push_back(std::string(glob_result.gl_pathv[i]));
  }

  for (unsigned int i = 0; i < files.size(); ++i){
    image.read(files[i]);
    images.push_back(image);
  }
}

void DrawOnCanvas(FrameCanvas *canvas) {
  // Set the color to red
  Color red(255, 0, 0);

  // Draw a red box across the screen
  for (int x = 0; x < 128; ++x) {
    for (int y = 0; y < 64; ++y) {
      canvas->SetPixel(x, y, red.r, red.g, red.b);
    }
  }
}

int main(int argc, char **argv) {
  RGBMatrix::Options matrix_options;
  RuntimeOptions runtime_options;

  // Set defaults
  matrix_options.chain_length = 3;
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

  // Do your own command line handling with the remaining flags.
  // while (getopt()) {...}

  // Looks like we're ready to start
  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_options);
  if (matrix == NULL) {
    return 1;
  }

  FrameCanvas *offscreen_canvas = matrix->CreateFrameCanvas();
  if (offscreen_canvas == NULL) {
    delete matrix;
    return 1;
  }

  // Install signal handler to gracefully shutdown when requested.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  // Draw something on the canvas
  DrawOnCanvas(offscreen_canvas);

  // Swap the offscreen canvas with the onscreen canvas
  matrix->SwapOnVSync(offscreen_canvas);

  // Wait for a signal to gracefully shutdown
  while (!interrupt_received) {
    usleep(100000);
  }

  // Done. Shut everything down.
  delete matrix;

  return 0;
}