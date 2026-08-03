# pyCAS - Python-wrapped Constructive Aerospace Simulator

> **Building aerospace simulations from fundamental, composable components**
>
> A modular Python framework built on the CADAC++ aerospace simulation framework by Peter H. Zipfel

[![Python](https://img.shields.io/badge/Python-3.8+-3776AB)]()
[![C++](https://img.shields.io/badge/C++-11-00599C)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()

## Overview

**pyCAS** extracts the core concepts from CADAC (Computer Aided Design of Aerospace Concepts) examples into a truly constructive simulation framework. Instead of working with monolithic example simulations, pyCAS enables you to build aerospace vehicles from **36+ validated, independent, reusable components**.

### Why pyCAS?

**Traditional CADAC**: Complete example simulations (ROCKET6G, FALCON6, etc.) - great for learning, but hard to adapt

**pyCAS**: Fundamental building blocks extracted from examples - build exactly what you need

```python
from pycas import Simulation, Component

# Build a custom ballistic simulation from components
sim = Simulation("MyProjectile")
sim.add_component(Component.time_management())
sim.add_component(Component.kinematics_3dof_flat())
sim.add_component(Component.forces_3dof())
sim.add_component(Component.drag_simple(cd=0.47, area=0.01))
sim.add_component(Component.gravity_constant())
sim.add_component(Component.atmosphere_constant())
sim.add_component(Component.termination(
    altitude_min=0.0,   # Stop when hitting ground
    time_max=10.0       # Or after 10 seconds
))

# Set initial conditions
sim.set_initial_state(
    sbel1=0, sbel2=0, sbel3=0,          # Position (m)
    vbel1=35.36, vbel2=0, vbel3=-35.36  # Velocity (m/s) - 45° launch
)

# Generate input, compile, and run
sim.generate_input_file()
sim.compile()
results = sim.run(duration=10, dt=0.01)
```

## Quick Start

### Installation

```bash
# Clone repository
git clone https://github.com/yakaboskic/CADAC.git
cd CADAC

# Install with uv (recommended)
uv pip install -e .

# Or with pip
pip install -e .

# For development
uv pip install -e ".[dev]"

# For plotting capabilities
uv pip install -e ".[plot]"
```

### Run Your First Simulation

```python
from pycas import Simulation, Component

# Build BALL3 - simple ballistic projectile
sim = Simulation("BALL3", working_dir="my_sim")
sim.add_component(Component.kinematics_3dof_flat())
sim.add_component(Component.forces_3dof())
sim.add_component(Component.drag_simple(cd=0.47, area=0.01))
sim.add_component(Component.gravity_constant(grav=9.81))
sim.add_component(Component.atmosphere_constant(rho=1.225))

# 45-degree launch at 50 m/s
sim.set_initial_state(
    sbel1=0.0, sbel2=0.0, sbel3=0.0,
    vbel1=35.36, vbel2=0.0, vbel3=-35.36
)

# Set simulation parameters
sim.set_config(duration=10.0, dt=0.01, output_step=0.05)

# Generate and run
input_file = sim.generate_input_file()
print(f"Generated: {input_file}")
```

## Project Organization

```
CADAC/
│
├── pycas/                          # Python framework (main package)
│   ├── __init__.py                 # Public API exports
│   ├── component.py                # Component class and factory methods
│   ├── component_registry.py      # Component catalog and metadata
│   ├── simulation.py               # Simulation builder and orchestrator
│   ├── compiler.py                 # C++ compilation management
│   ├── runner.py                   # Simulation execution
│   └── trajectory.py               # Trajectory analysis and comparison
│
├── components/                     # Modular component library (36+ components)
│   ├── kinematics/                 # Position and velocity integration
│   │   ├── kinematics_3dof_flat.cpp
│   │   ├── kinematics_6dof_sph.cpp
│   │   └── INDEX.md
│   ├── dynamics/                   # Forces and moments
│   │   ├── forces_3dof.cpp
│   │   ├── forces_6dof.cpp
│   │   └── INDEX.md
│   ├── aerodynamics/               # Aero models
│   │   ├── drag_simple.cpp
│   │   ├── drag_cd_mach.cpp
│   │   └── INDEX.md
│   ├── propulsion/                 # Thrust and fuel
│   │   ├── propulsion_rocket_3stage.cpp
│   │   └── INDEX.md
│   ├── environment/                # Gravity, atmosphere, wind
│   │   ├── gravity_constant.cpp
│   │   ├── gravity_wgs84.cpp
│   │   ├── atmosphere_constant.cpp
│   │   ├── atmosphere_us76.cpp
│   │   └── INDEX.md
│   ├── guidance/                   # Navigation and control
│   │   ├── guidance_line.cpp
│   │   ├── guidance_pro_nav.cpp
│   │   └── INDEX.md
│   ├── sensors/                    # GPS, INS, seekers
│   └── ...
│
├── framework/                      # CADAC++ core framework
│   ├── global_header.hpp           # Core data structures (Variable, Matrix)
│   ├── class_hierarchy.hpp         # Vehicle class hierarchy
│   ├── utility_functions.cpp      # Linear algebra, coordinate transforms
│   ├── execution.cpp               # Simulation executive
│   └── ...
│
├── example/                        # Original CADAC example simulations
│   ├── BALL3/                      # Simple 3DoF ballistic
│   ├── MAGSIX/                     # 6DoF Magnus rotor
│   ├── ROCKET6G/                   # Three-stage orbital rocket
│   ├── FALCON6/                    # F-16 aircraft with FCS
│   ├── SRAAM6/                     # Air-to-air missile
│   ├── AGM6/                       # Air-to-ground missile
│   ├── CRUISE5/                    # Cruise missile
│   ├── GHAME6/                     # Hypersonic vehicle
│   └── ...
│
├── tests/                          # Test suite
│   ├── regression/                 # Regression tests
│   │   ├── test_ball3_regression.py
│   │   ├── test_ball3_simple.py
│   │   └── reference/              # Ground truth trajectories
│   └── ...
│
├── docs/                           # Documentation
│   ├── COMPONENT_GUIDE.md          # Component usage guide
│   ├── EXAMPLE_PROMPTS.md          # Example simulation prompts
│   └── SESSION_LOGS.md             # Development session logs
│
├── pyproject.toml                  # Python package configuration (uv/pip)
├── Makefile                        # Build all CADAC examples
└── README.md                       # This file
```

## Key Concepts

### 1. Components Are Framework-Agnostic

Components are written generically using the `Vehicle` class and adapt to any CADAC vehicle type:

```cpp
// components/environment/gravity_constant.cpp
void Vehicle::def_environment()
{
    vehicle[0].init("grav", "D", 0, "Gravity - m/s^2", "environment", "out", "");
}

void Vehicle::environment(double int_step)
{
    double grav = 9.81;  // Standard gravity
    vehicle[0].gets(grav);
}
```

pyCAS automatically adapts this to `Ball::`, `Rocket6::`, `Missile5::`, etc. during compilation.

### 2. Modular Architecture

Each component:
- **Self-contained**: Defines its own variables and dependencies
- **Documented**: INDEX.md describes inputs, outputs, usage
- **Validated**: Extracted from working CADAC examples
- **Composable**: Works with other components via vehicle array

### 3. Automatic Dependency Resolution

pyCAS manages module dependencies automatically:

```python
sim.add_component(Component.forces_3dof())  # Needs gravity
sim.add_component(Component.gravity_constant())  # Provides gravity

# pyCAS ensures correct module order:
# MODULES
#     environment   def,exec    (gravity)
#     forces        def,exec    (uses gravity)
```

### 4. Framework Adaptation Layer

pyCAS bridges generic components to CADAC frameworks:

```
Generic Component (Vehicle::)
         ↓
  pyCAS Adapter
         ↓
Framework-Specific (Ball::, Rocket6::, etc.)
```

### 5. Full Transparency

pyCAS generates readable C++ code - you can inspect, modify, and debug:

```bash
my_sim/
├── input.asc              # Generated input file
├── kinematics.cpp         # Adapted component
├── forces.cpp             # Adapted component
├── environment.cpp        # Adapted component
└── my_sim                 # Compiled executable
```

## Component Library

### Current Status: 36 Components Extracted (72% Complete)

| Category | Components | Status |
|----------|-----------|---------|
| **Kinematics** | 3DoF flat, 6DoF spherical, 3DoF spherical | ✅ Complete |
| **Dynamics** | Forces 3DoF, Forces 6DoF, Moments 6DoF | ✅ Complete |
| **Aerodynamics** | Simple drag, CD(Mach), Full aero tables | ✅ Complete |
| **Environment** | Constant gravity/atmosphere, WGS-84, US76 | ✅ Complete |
| **Propulsion** | Rocket 3-stage, Simple thrust | ✅ Complete (10/10 from ROCKET6G) |
| **Guidance** | Line guidance, Pro-nav, LTG | 🚧 In Progress |
| **Sensors** | GPS, INS, IR seeker | 🚧 In Progress |
| **Control** | Autopilot, Stability augmentation | 🚧 In Progress |

See [docs/COMPONENT_GUIDE.md](docs/COMPONENT_GUIDE.md) for complete component documentation.

## Regression Testing

pyCAS includes a comprehensive regression testing framework:

```bash
# Run BALL3 regression test
python tests/regression/test_ball3_regression.py

# Output:
# ✅ TEST PASSED - BALL3 REGRESSION SUCCESSFUL!
# All trajectory variables within tolerance:
#   - 202 trajectory points
#   - 8 variables compared
#   - Position accuracy: ±10cm RMS, ±50cm max
#   - Velocity accuracy: ±5cm/s RMS, ±20cm/s max
```

The test:
1. Builds simulation from components using Python API
2. Compiles C++ code
3. Runs simulation
4. Compares trajectory with reference
5. Validates RMS and max errors

## Advanced Usage

### Custom Component Integration

Add your own components to `components/`:

```cpp
// components/my_category/my_component.cpp
#include "class_hierarchy.hpp"

void Vehicle::def_my_module()
{
    vehicle[100].init("my_var", 0, "My variable", "data", "", "");
}

void Vehicle::my_module(double int_step)
{
    double my_var = vehicle[100].real();
    // Your physics here
    vehicle[100].gets(my_var);
}
```

Register in `pycas/component_registry.py`:

```python
@staticmethod
def my_component(**kwargs):
    return ComponentInfo(
        name="my_component",
        category="my_category",
        file="my_category/my_component.cpp",
        parameters=kwargs
    )
```

### Trajectory Analysis

```python
from pycas import Trajectory, TrajectoryComparator

# Load trajectories
test = Trajectory.from_file("test/plot1.asc")
ref = Trajectory.from_file("reference/plot1.asc")

# Compare
comp = TrajectoryComparator(test, ref)
errors = comp.compare(['SBEL1', 'SBEL3', 'VBEL1', 'altitude'])

print(f"Position RMS error: {errors['SBEL1']['rms']:.3f} m")
print(f"Velocity max error: {errors['VBEL1']['max']:.3f} m/s")
```

### Building CADAC Example Simulations

Original CADAC examples still available:

```bash
# Build all examples
make

# Build specific example
make ROCKET6G

# Run
cd example/ROCKET6G
./rocket6g

# View output
cat plot1.asc
```

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for complete build documentation.

## Example Simulations

### BALL3 - Ballistic Projectile
```python
from pycas import Simulation, Component

sim = Simulation("BALL3")
sim.add_component(Component.kinematics_3dof_flat())
sim.add_component(Component.drag_simple(cd=0.47, area=0.01))
sim.add_component(Component.gravity_constant())
sim.set_initial_state(sbel1=0, vbel1=35.36, vbel3=-35.36)
sim.run(duration=10)
```

### ROCKET6G - Orbital Insertion
```python
sim = Simulation("ROCKET6")
sim.add_component(Component.kinematics_6dof_sph())
sim.add_component(Component.propulsion_rocket_3stage())
sim.add_component(Component.guidance_ltg())
sim.add_component(Component.gravity_wgs84())
sim.add_component(Component.atmosphere_us76())
# ... configure stages and guidance
sim.run(duration=600)
```

See [docs/EXAMPLE_PROMPTS.md](docs/EXAMPLE_PROMPTS.md) for more examples.

## Development

### Running Tests

```bash
# Install dev dependencies
uv pip install -e ".[dev]"

# Run all tests
pytest

# Run specific test
pytest tests/regression/test_ball3_regression.py -v

# Run with coverage
pytest --cov=pycas --cov-report=html
```

### Code Formatting

```bash
# Format code
black pycas/ tests/

# Lint
ruff check pycas/ tests/
```

## Architecture

### pyCAS Workflow

```
Python API (pycas)
    ↓
Component Selection & Configuration
    ↓
Framework Adaptation (Vehicle → Ball/Rocket/etc.)
    ↓
Input File Generation (input.asc)
    ↓
C++ Code Assembly & Compilation
    ↓
Simulation Execution
    ↓
Trajectory Analysis & Validation
```

### CADAC++ Framework Layers

```
┌─────────────────────────────────────┐
│  Simulation Executive               │  execution.cpp
│  (Time loop, module scheduling)     │
├─────────────────────────────────────┤
│  Vehicle Classes                    │  Ball, Rocket6, Missile5
│  (Cadac hierarchy)                  │  class_hierarchy.hpp
├─────────────────────────────────────┤
│  Modules (def, init, exec)          │  User-defined physics
│  - environment, kinematics, etc.    │  *_functions.cpp
├─────────────────────────────────────┤
│  Core Utilities                     │  utility_functions.cpp
│  - Matrix, integrators, transforms  │  global_functions.cpp
└─────────────────────────────────────┘
```

## References

### CADAC Framework
1. Zipfel, Peter H, *"Modeling and Simulation of Aerospace Vehicle Dynamics"*, AIAA Education Series, 3rd Ed. 2014
2. Zipfel, Peter H, *"Building Aerospace Simulations in C++"*, AIAA Self-Study Series, 3rd Ed. 2014
3. Zipfel, Peter H, *"Fundamentals of 6 DoF Aerospace Simulation and Analysis in C++"*, AIAA, 2nd Ed. 2014

### pyCAS Development
- Component extraction methodology: [docs/SESSION_LOGS.md](docs/SESSION_LOGS.md)
- Component documentation: [components/*/INDEX.md](components/)

## Contributing

Contributions welcome! Areas of focus:
- Extracting remaining components (guidance, sensors, control)
- Adding test coverage for more components
- Improving framework adaptation layer
- Documentation and examples

## License

MIT License - Free to use and modify without warranty.

**Original CADAC Framework**: Academic software by Peter H. Zipfel
**pyCAS Extensions**: Open source contributions

## Contact

**pyCAS Development**: https://github.com/yakaboskic/CADAC

**Original CADAC Author**: Peter H. Zipfel, Ph.D.
Modeling and Simulation Technologies (MaSTech)
mastech.zipfel@cox.net

---

**Note**: This is educational/research software. For production aerospace applications, use certified flight dynamics software with appropriate validation and verification.
