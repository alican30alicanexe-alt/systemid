# CADAC Simulations - Root Makefile
# Build system for Peter Zipfel's CADAC aerospace simulations
# Modernized for cross-platform compilation with GNU Make and g++

# List of all example simulations
EXAMPLES = MAGSIX ROCKET6G AIM5 ADS6 AGM6 CRUISE5 FALCON6 GHAME3 GHAME6 SRAAM6

# Default target - build all simulations
.PHONY: all
all:
	@echo "=================================="
	@echo "Building all CADAC simulations..."
	@echo "=================================="
	@for example in $(EXAMPLES); do \
		echo ""; \
		echo "Building $$example..."; \
		$(MAKE) -C example/$$example || exit 1; \
	done
	@echo ""
	@echo "=================================="
	@echo "All simulations built successfully!"
	@echo "=================================="

# Build specific simulation
.PHONY: $(EXAMPLES)
$(EXAMPLES):
	@echo "Building $@..."
	@$(MAKE) -C example/$@

# Clean all simulations
.PHONY: clean
clean:
	@echo "Cleaning all simulations..."
	@for example in $(EXAMPLES); do \
		echo "Cleaning $$example..."; \
		$(MAKE) -C example/$$example clean 2>/dev/null || true; \
	done
	@echo "Clean complete!"

# Clean only output files, keep executables
.PHONY: cleanout
cleanout:
	@echo "Cleaning output files from all simulations..."
	@for example in $(EXAMPLES); do \
		$(MAKE) -C example/$$example cleanout 2>/dev/null || true; \
	done
	@echo "Output files cleaned!"

# Test build and run MAGSIX (simplest example)
.PHONY: test
test: MAGSIX
	@echo ""
	@echo "Testing MAGSIX simulation..."
	@cd example/MAGSIX && ./magsix > /dev/null 2>&1 && echo "✓ MAGSIX test passed!"

# Help target
.PHONY: help
help:
	@echo "CADAC Simulations Build System"
	@echo "==============================="
	@echo ""
	@echo "Available targets:"
	@echo "  make           - Build all simulations"
	@echo "  make <SIM>     - Build specific simulation (e.g., make ROCKET6G)"
	@echo "  make clean     - Remove all build artifacts"
	@echo "  make cleanout  - Remove only output files, keep executables"
	@echo "  make test      - Build and test MAGSIX simulation"
	@echo "  make help      - Display this help message"
	@echo ""
	@echo "Available simulations:"
	@echo "  MAGSIX    - 6DoF Magnus rotor dynamics"
	@echo "  ROCKET6G  - 6DoF Three-stage rocket with GPS/INS"
	@echo "  AIM5      - 5DoF Short range air-to-air missile"
	@echo "  ADS6      - 6DoF Air defense simulation"
	@echo "  AGM6      - 6DoF Air-to-ground missile with IR seeker"
	@echo "  CRUISE5   - 5DoF Cruise missile"
	@echo "  FALCON6   - 6DoF Falcon (F16) aircraft"
	@echo "  GHAME3    - 3DoF NASA hypersonic vehicle"
	@echo "  GHAME6    - 6DoF NASA hypersonic vehicle"
	@echo "  SRAAM6    - 6DoF Air-to-air missile"
	@echo ""
	@echo "Example usage:"
	@echo "  make ROCKET6G          # Build only ROCKET6G"
	@echo "  make                   # Build all simulations"
	@echo "  cd example/MAGSIX && make run  # Build and run MAGSIX"
