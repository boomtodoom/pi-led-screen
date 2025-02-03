# Define the compiler
CXX = g++

# Define the flags for the compiler
CFLAGS = -Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS = $(CFLAGS) -std=c++11 -Iinclude -I/usr/local/include -I/usr/include/ImageMagick-6
LDFLAGS = -Llib -L/usr/local/lib -lrgbmatrix -lMagick++-6.Q16HDRI -lMagickCore-6.Q16HDRI -lMagickWand-6.Q16HDRI

# Define the source files
SRCS = main.cpp

# Define the output executable
TARGET = pi-rgb-led-matrix

# Define the object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Rule to link the object files into the final executable
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(TARGET)

# Rule to compile the source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up the build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean