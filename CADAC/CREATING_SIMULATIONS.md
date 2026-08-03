# Creating New CADAC Simulations

## Overview

The CADAC framework is designed for creating custom simulations. You're not limited to the 10 examples - they're templates to learn from and modify.

## Step-by-Step Process

### 1. Choose Your Simulation Type

Decide on the degrees of freedom:
- **3DoF** - Point mass (position only, no rotation) - simplest
- **5DoF** - Skid-to-turn missiles (3 position + 2 angles)
- **6DoF** - Full rigid body (3 position + 3 rotation) - most complex

### 2. Define Required Modules

Common modules:
- `environment` - Gravity, atmosphere, wind
- `kinematics` - Position and velocity integration
- `dynamics` - Forces and accelerations (3DoF) or moments (6DoF)
- `aerodynamics` - Lift, drag forces
- `propulsion` - Thrust (if powered)
- `guidance` - Navigation, targeting
- `control` - Autopilot, stability

### 3. Create Class Hierarchy

Define your vehicle class inheriting from `Cadac`:

```cpp
class Ball : public Cadac {
    // Your vehicle-specific implementation
};
```

### 4. Implement Module Functions

Each module has up to 4 stages:
- `def` - Define variables (allocate memory)
- `init` - Initialize values (before simulation)
- `exec` - Execute (called every time step)
- `term` - Terminate (cleanup after simulation)

### 5. Create Required Files

Minimum files needed:
- `class_hierarchy.hpp` - Class definitions
- `global_header.hpp` - Global structures (copy from example)
- `global_constants.hpp` - Constants (copy from example)
- `global_functions.cpp` - I/O functions (copy from example)
- `utility_header.hpp` - Matrix class (copy from example)
- `utility_functions.cpp` - Math utilities (copy from example)
- `class_functions.cpp` - Your class implementation
- `your_modules.cpp` - Module implementations
- `execution.cpp` - Main program (modify from example)
- `Makefile` - Build system

### 6. Create Input File

Define `input.asc` with your parameters.

### 7. Build and Test

```bash
make
./your_simulation
```

## Example: BALL3 (3DoF Ballistic Projectile)

Let's create the simplest possible simulation - a cannonball with gravity and drag.

### Files Structure

```
BALL3/
├── Makefile
├── class_hierarchy.hpp
├── class_functions.cpp
├── environment.cpp
├── forces.cpp
├── kinematics.cpp
├── execution.cpp
├── global_*.{hpp,cpp}      (copied from MAGSIX)
├── utility_*.{hpp,cpp}      (copied from MAGSIX)
└── input.asc
```

### Minimum Code Required

The key is understanding what each file does:

**class_hierarchy.hpp** - Defines your vehicle class
**class_functions.cpp** - Implements class methods
**environment.cpp** - Gravity and atmosphere
**forces.cpp** - Calculates drag force
**kinematics.cpp** - Integrates position/velocity
**execution.cpp** - Main program (mostly standard)

## Practical Approach

### Option 1: Start from Scratch (Advanced)
- Requires understanding all framework components
- Copy utility files from an existing example
- Write only vehicle-specific code

### Option 2: Modify Existing Example (Recommended)
- Copy closest matching example (e.g., MAGSIX for 3DoF)
- Modify modules for your needs
- Change input parameters
- Rename classes and variables

### Option 3: Simplify Existing Example (Easiest)
- Copy an example
- Remove modules you don't need
- Simplify remaining modules
- Modify input file

## Key Concepts

### Variables

All variables are stored in arrays:
```cpp
ball[0].init("time", 0.0, "Simulation time - s", "exec", "scrn", "plot");
ball[1].init("mass", 1.0, "Projectile mass - kg", "data", "", "");
```

Format: `init(name, initial_value, description, role, output_flags)`

### Module Execution

In `execution.cpp`, modules are called each time step:
```cpp
// Each iteration:
for (all vehicles) {
    vehicle->environment(int_step);
    vehicle->forces(int_step);
    vehicle->kinematics(int_step);
}
```

### Integration

Use utility functions:
```cpp
// In kinematics module:
SBEL = integrate(VBEL, SBEL, VEL_old, int_step);  // position
VBEL = integrate(ABEL, VBEL, ABEL_old, int_step); // velocity
```

## Tips for Success

1. **Start Simple**: 3DoF with minimal modules
2. **Copy Liberally**: Use utility files from examples unchanged
3. **Test Incrementally**: Add one module at a time
4. **Use Existing Modules**: Copy environment.cpp from MAGSIX if you need gravity
5. **Debug with Output**: Add variables to plot output to understand behavior
6. **Read Examples**: MAGSIX (simplest) → ROCKET6G (complex)

## Common Patterns

### Module Template

```cpp
void YourClass::def_module_name() {
    // Define variables
    your_array[0].init("var_name", 0, "Description - units",
                       "data", "scrn", "plot");
}

void YourClass::init_module_name() {
    // Initialize from input or calculations
    double var = your_array[0].real();
    // ... calculations ...
    your_array[1].gets(calculated_value);
}

void YourClass::module_name(double int_step) {
    // Execute each time step
    // Get current values
    double old_val = your_array[0].real();

    // Calculate new values
    double new_val = old_val + int_step * rate;

    // Store results
    your_array[0].gets(new_val);
}
```

### Input File Template

```
TITLE Your Simulation Description
OPTIONS y_scrn y_plot y_doc
MODULES
    environment   def,exec
    kinematics    def,init,exec
    forces        def,exec
END
TIMING
    scrn_step 1.0
    plot_step 0.1
    int_step 0.01
END
VEHICLES 1
    YOUR_TYPE NAME
        param1  value1    // Description
        param2  value2    // Description
    END
ENDTIME 100
STOP
```

## Debugging New Simulations

1. **Compilation errors**: Check includes and class definitions
2. **Runtime crashes**: Verify array dimensions and initialization
3. **Wrong results**: Add debug output, check units, verify physics
4. **No output**: Check OPTIONS flags in input.asc

## Next Steps

After creating BALL3 example:
1. Try modifying it (different initial conditions, add wind)
2. Add a module (e.g., thrust for a rocket)
3. Create a 5DoF or 6DoF version
4. Study how ROCKET6G implements complex features

The framework is yours to extend!
