#include "led-matrix.h"
#include "graphics.h"
#include <signal.h>
#include <unistd.h>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
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
  matrix_options.rows = 32;
  matrix_options.cols = 64;
  matrix_options.show_refresh_rate = true;
  runtime_options.drop_privileges = 1;

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
  delete offscreen_canvas;
  delete matrix;

  return 0;
}