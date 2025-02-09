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
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
    interrupt_received = true;
}

/* Draws an image to the screen pixel by pixel, and pixels outside the 
image width but within the screen width are set to black to prevent shadowing issues. */
void drawImage(const Magick::Image &image, FrameCanvas *canvas) {
    const int imageWidth = image.columns();
    const int imageHeight = image.rows();
    
    // Get pixel data as a raw pointer
    const Magick::PixelPacket *pixels = image.getConstPixels(0, 0, imageWidth, imageHeight);
    if (!pixels) return; // Fail-safe check

    const Magick::PixelPacket *pixelPtr = pixels; // Pointer to pixel data
    for (int y = 0; y < imageHeight; ++y) {
        for (int x = 0; x < imageWidth; ++x, ++pixelPtr) {
            canvas->SetPixel(x, y, pixelPtr->red >> 8, pixelPtr->green >> 8, pixelPtr->blue >> 8);
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
            if (file_name.find(".bmp") != std::string::npos || 
                file_name.find(".jpg") != std::string::npos || 
                file_name.find(".jpeg") != std::string::npos || 
                file_name.find(".png") != std::string::npos || 
                file_name.find(".webp") != std::string::npos)
            {
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

std::queue<std::unique_ptr<Magick::Image>> image_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;

void image_loader(const std::vector<std::string> &image_files) {
    int preloaded = 0;
    while (!interrupt_received) {
        for (const auto &file_path : image_files) {
            if (interrupt_received) break;
            try {
                auto image = std::make_unique<Magick::Image>();
                image->read(file_path);
                image->resize(Magick::Geometry(128, 64));

                std::unique_lock<std::mutex> lock(queue_mutex);
                
                // If queue is empty, don't wait, allow preloading
                if (preloaded < 3) {  // Preload first 3 images
                    preloaded++;
                } else {
                    queue_cv.wait(lock, [] { return image_queue.size() < 5 || interrupt_received; });
                }

                image_queue.push(std::move(image));
                lock.unlock();
                queue_cv.notify_one();
            } catch (Magick::Exception &error) {
                std::cerr << "Error loading image " << file_path << ": " << error.what() << std::endl;
            }
        }
    }
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
    matrix_options.chain_length = 2;
    matrix_options.parallel = 1;
    matrix_options.hardware_mapping = "regular";
    matrix_options.rows = 64;
    matrix_options.cols = 64;
    matrix_options.disable_hardware_pulsing = false;
    matrix_options.brightness = 100;
    matrix_options.pwm_bits = 7;
    runtime_options.drop_privileges = 1;
    runtime_options.gpio_slowdown = 4;
    matrix_options.show_refresh_rate = true;

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

    std::thread loader_thread(image_loader, std::ref(image_files));

    while (!interrupt_received) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        queue_cv.wait(lock, []{ return !image_queue.empty() || interrupt_received; });

        if (interrupt_received) break;

        std::unique_ptr<Magick::Image> image = std::move(image_queue.front());
        image_queue.pop();
        lock.unlock();
        queue_cv.notify_one();

        auto draw_start = std::chrono::high_resolution_clock::now();
        drawImage(*image, offscreen_canvas);
        auto draw_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> draw_duration = draw_end - draw_start;
//        std::cout << "Image draw time: " << draw_duration.count() << " ms" << std::endl;

        auto swap_start = std::chrono::high_resolution_clock::now();
        offscreen_canvas = matrix->SwapOnVSync(offscreen_canvas);
        auto swap_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> swap_duration = swap_end - swap_start;
//        std::cout << "Canvas swap time: " << swap_duration.count() << " ms" << std::endl;

//        std::cout << "Displayed image" << std::endl;

        //usleep(25000000); // 25ms delay
    }

    loader_thread.join();

    delete matrix;
    return 0;
}
