# Define the compiler
CXX = g++

# Compiler flags
CFLAGS = -Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS = $(CFLAGS) -std=c++11 -Iinclude -I/usr/local/include $(shell pkg-config --cflags ImageMagick++)

# Linker flags
LDFLAGS = -L/usr/local/lib -L$(CURDIR)/lib -lrgbmatrix -lMagick++-6.Q16 -lMagickWand-6.Q16 -lMagickCore-6.Q16

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
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Rule to compile the source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up the build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
