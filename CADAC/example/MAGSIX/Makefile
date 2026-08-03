# Makefile for CADAC MAGSIX Simulation
# Adapted from Peter Zipfel's Visual Studio project to cross-platform build

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Wno-write-strings
LDFLAGS =

# Target executable
TARGET = magsix

# Source files
SOURCES = attitude.cpp \
          class_functions.cpp \
          environment.cpp \
          execution.cpp \
          global_functions.cpp \
          rotor_functions.cpp \
          trajectory.cpp \
          utility_functions.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Header dependencies
HEADERS = class_hierarchy.hpp \
          global_header.hpp \
          global_constants.hpp \
          utility_header.hpp

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "Build complete! Executable: $(TARGET)"

# Compile source files to object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f tabout.asc doc.asc traj.asc plot*.asc input_copy.asc
	@echo "Clean complete!"

# Clean only output files, keep executable
cleanout:
	rm -f tabout.asc doc.asc traj.asc plot*.asc input_copy.asc
	@echo "Output files cleaned!"

# Run the simulation with default input
run: $(TARGET)
	./$(TARGET)

# Display help
help:
	@echo "CADAC MAGSIX Simulation - Makefile targets:"
	@echo "  make           - Build the simulation executable"
	@echo "  make clean     - Remove all build artifacts and output files"
	@echo "  make cleanout  - Remove only output files"
	@echo "  make run       - Build and run the simulation"
	@echo "  make help      - Display this help message"

.PHONY: all clean cleanout run help
