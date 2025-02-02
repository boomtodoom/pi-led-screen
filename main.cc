#include "led-matrix.h"

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RuntimeOptions;

volatile bool interrupt_received = false;

static void InterruptHandler(int signo) {
  interrupt_received = true;
}

void DrawOnCanvas(Canvas *canvas) {
  const int width = canvas->width();
  const int height = canvas->height();

  // Draw a red box in the middle
  for (int y = height / 4; y < height * 3 / 4; ++y) {
    for (int x = width / 4; x < width * 3 / 4; ++x) {
      canvas->SetPixel(x, y, 255, 0, 0);
    }
  }
}

void main(int argc, char **argv) {
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
  while (getopt()) {...}

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
}