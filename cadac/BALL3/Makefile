# Makefile for CADAC BALL3 Simulation
# 3DoF Ballistic Projectile - Tutorial Example

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Wno-write-strings
LDFLAGS =

# Target executable
TARGET = ball3

# Source files
SOURCES = ball_functions.cpp \
          class_functions.cpp \
          environment.cpp \
          execution.cpp \
          forces.cpp \
          global_functions.cpp \
          kinematics.cpp \
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
	@echo ""
	@echo "This is a custom simulation created to demonstrate the CADAC framework."
	@echo "Run with: ./ball3"

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
	@echo "CADAC BALL3 Simulation - Custom Tutorial Example"
	@echo "================================================"
	@echo ""
	@echo "This is a 3DoF ballistic projectile simulation created to demonstrate"
	@echo "how to build new CADAC simulations from scratch."
	@echo ""
	@echo "Makefile targets:"
	@echo "  make           - Build the simulation executable"
	@echo "  make clean     - Remove all build artifacts and output files"
	@echo "  make cleanout  - Remove only output files"
	@echo "  make run       - Build and run the simulation"
	@echo "  make help      - Display this help message"
	@echo ""
	@echo "The simulation models a cannonball with:"
	@echo "  - 45 degree launch angle"
	@echo "  - 50 m/s initial velocity"
	@echo "  - Gravity (9.81 m/s²)"
	@echo "  - Aerodynamic drag"

.PHONY: all clean cleanout run help
