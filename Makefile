# Define the compiler
CXX = g++

# Define the flags for the compiler
CXXFLAGS = -Wall -Wextra -std=c++11 -Iinclude

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
    $(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile the source files into object files
%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up the build files
clean:
    rm -f $(OBJS) $(TARGET)

.PHONY: all clean