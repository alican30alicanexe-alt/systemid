# Makefile for CADAC GHAME6 Simulation
# Auto-generated makefile

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Wno-write-strings
LDFLAGS =

# Target executable
TARGET = ghame6

# Source files
SOURCES = actuator.cpp aerodynamics.cpp class_functions.cpp control.cpp datalink.cpp environment.cpp euler.cpp execution.cpp forces.cpp global_functions.cpp gps.cpp ground0_modules.cpp guidance.cpp hyper_functions.cpp ins.cpp intercept.cpp kinematics.cpp newton.cpp propulsion.cpp radar_functions.cpp radar_modules.cpp rcs.cpp round3_modules.cpp satellite_functions.cpp satellite_modules.cpp seeker.cpp startrack.cpp utility_functions.cpp 

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
	@echo "CADAC GHAME6 Simulation - Makefile targets:"
	@echo "  make           - Build the simulation executable"
	@echo "  make clean     - Remove all build artifacts and output files"
	@echo "  make cleanout  - Remove only output files"
	@echo "  make run       - Build and run the simulation"
	@echo "  make help      - Display this help message"

.PHONY: all clean cleanout run help
