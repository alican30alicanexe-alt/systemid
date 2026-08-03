#!/bin/bash
# Script to create Makefiles for all CADAC examples

# Change to example directory
cd /home/user/CADAC/example

# List of all example directories
EXAMPLES="ADS6 AGM6 AIM5 CRUISE5 FALCON6 GHAME3 GHAME6 SRAAM6"

for EXAMPLE in $EXAMPLES; do
    echo "Processing $EXAMPLE..."

    # Skip if Makefile already exists
    if [ -f "$EXAMPLE/Makefile" ]; then
        echo "  Makefile already exists, skipping..."
        continue
    fi

    # Get target name (lowercase)
    TARGET=$(echo $EXAMPLE | tr '[:upper:]' '[:lower:]')

    # Extract source files from .vcxproj
    SOURCES=$(grep "ClCompile Include" "$EXAMPLE/$EXAMPLE.vcxproj" 2>/dev/null | \
              sed 's/.*Include="//' | sed 's/".*//' | tr '\n' ' ')

    if [ -z "$SOURCES" ]; then
        echo "  No source files found, skipping..."
        continue
    fi

    # Create Makefile
    cat > "$EXAMPLE/Makefile" << EOF
# Makefile for CADAC $EXAMPLE Simulation
# Auto-generated makefile

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 -Wno-write-strings
LDFLAGS =

# Target executable
TARGET = $TARGET

# Source files
SOURCES = $SOURCES

# Object files
OBJECTS = \$(SOURCES:.cpp=.o)

# Header dependencies
HEADERS = class_hierarchy.hpp \\
          global_header.hpp \\
          global_constants.hpp \\
          utility_header.hpp

# Default target
all: \$(TARGET)

# Link object files to create executable
\$(TARGET): \$(OBJECTS)
	\$(CXX) \$(LDFLAGS) -o \$@ \$^
	@echo "Build complete! Executable: \$(TARGET)"

# Compile source files to object files
%.o: %.cpp \$(HEADERS)
	\$(CXX) \$(CXXFLAGS) -c \$< -o \$@

# Clean build artifacts
clean:
	rm -f \$(OBJECTS) \$(TARGET)
	rm -f tabout.asc doc.asc traj.asc plot*.asc input_copy.asc
	@echo "Clean complete!"

# Clean only output files, keep executable
cleanout:
	rm -f tabout.asc doc.asc traj.asc plot*.asc input_copy.asc
	@echo "Output files cleaned!"

# Run the simulation with default input
run: \$(TARGET)
	./\$(TARGET)

# Display help
help:
	@echo "CADAC $EXAMPLE Simulation - Makefile targets:"
	@echo "  make           - Build the simulation executable"
	@echo "  make clean     - Remove all build artifacts and output files"
	@echo "  make cleanout  - Remove only output files"
	@echo "  make run       - Build and run the simulation"
	@echo "  make help      - Display this help message"

.PHONY: all clean cleanout run help
EOF

    echo "  Makefile created for $EXAMPLE"

    # Fix missing <cstring> include in global_header.hpp if needed
    if ! grep -q "#include <cstring>" "$EXAMPLE/global_header.hpp" 2>/dev/null; then
        if grep -q "#include <string>" "$EXAMPLE/global_header.hpp" 2>/dev/null; then
            sed -i '/#include <string>/a #include <cstring>' "$EXAMPLE/global_header.hpp"
            echo "  Added #include <cstring> to global_header.hpp"
        fi
    fi

    # Fix utility_header.hpp if it has the Matrix:: issue
    if grep -q "Matrix::" "$EXAMPLE/utility_header.hpp" 2>/dev/null; then
        sed -i 's/Matrix Matrix::/Matrix /g' "$EXAMPLE/utility_header.hpp"
        sed -i 's/void Matrix::/void /g' "$EXAMPLE/utility_header.hpp"
        sed -i 's/double & Matrix::/double \& /g' "$EXAMPLE/utility_header.hpp"
        echo "  Fixed Matrix:: qualifications in utility_header.hpp"
    fi

done

echo "Done!"
