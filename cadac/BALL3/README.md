# BALL3 - Custom 3DoF Ballistic Projectile Simulation

## Purpose

This simulation was created to **demonstrate how to build a new CADAC simulation from scratch**. It's the simplest possible physics simulation: a ball with gravity and drag.

BALL3 is NOT one of the original 10 examples - it was created as a tutorial to show you the process!

## What It Simulates

- **3 Degrees of Freedom (3DoF)**: Position only (X, Y, Z), no rotation
- **Gravity**: Constant 9.81 m/s² downward
- **Aerodynamic Drag**: Simple quadratic drag model
- **Ground Impact**: Simulation stops when ball hits ground

Think of it as a cannonball, baseball, or any simple projectile.

## Files Created

To build this simulation, we created:

### Core Simulation Files (custom)
- `environment.cpp` - Provides gravity
- `forces.cpp` - Calculates drag force
- `kinematics.cpp` - Integrates position and velocity
- `class_hierarchy.hpp` - Defines the Ball class
- `ball_functions.cpp` - Framework functions (adapted from MAGSIX)
- `class_functions.cpp` - Class implementation (adapted from MAGSIX)
- `execution.cpp` - Main program (adapted from MAGSIX)

### Utility Files (copied from MAGSIX)
- `global_header.hpp`
- `global_constants.hpp`
- `global_functions.cpp`
- `utility_header.hpp`
- `utility_functions.cpp`

### Configuration
- `input.asc` - Simulation parameters
- `Makefile` - Build system

## How We Built It

### Step 1: Copy Utility Files

```bash
cp MAGSIX/global_*.{hpp,cpp} BALL3/
cp MAGSIX/utility_*.{hpp,cpp} BALL3/
```

These files provide the CADAC framework (Matrix class, I/O, etc.)

### Step 2: Define Class Hierarchy

Created `class_hierarchy.hpp`:
```cpp
class Cadac {  // Base class
    virtual void def_environment()=0;
    virtual void def_kinematics()=0;
    virtual void def_forces()=0;
    // ... framework functions ...
};

class Ball : public Cadac {  // Our vehicle class
    // Implements all virtual functions
};
```

### Step 3: Implement Modules

Each module has a def and exec function:

**environment.cpp**:
```cpp
void Ball::def_environment() {
    ball[0].init("grav", "D", 0, "Gravity - m/s^2", ...);
}

void Ball::environment(double int_step) {
    double grav = 9.81;
    ball[0].gets(grav);
}
```

**forces.cpp**:
```cpp
void Ball::forces(double int_step) {
    // Calculate drag force
    double drag = 0.5 * rho * v^2 * cd * area;
    // Apply to specific force vector
}
```

**kinematics.cpp**:
```cpp
void Ball::kinematics(double int_step) {
    // Integrate velocity
    VBEL = VBEL + ABEL * int_step;
    // Integrate position
    SBEL = SBEL + VBEL * int_step;
}
```

### Step 4: Adapt Framework Files

- Copy `execution.cpp` from MAGSIX
- Replace "trajectory" and "attitude" modules with "kinematics" and "forces"
- Copy and adapt `ball_functions.cpp` for framework methods

### Step 5: Create Input File

```
TITLE BALL3 - Simple ballistic projectile
OPTIONS y_scrn y_plot y_doc
MODULES
    environment   def,exec
    kinematics    def,init,exec
    forces        def,exec
END
TIMING
    int_step 0.01    # 10ms integration step
    plot_step 0.05   # Output every 50ms
END
VEHICLES 1
    BALL CANNONBALL
        # Initial position (at origin)
        sbel1  0
        sbel2  0
        sbel3  0

        # Initial velocity (45 deg launch at 50 m/s)
        vbel1  35.36    # Horizontal
        vbel2  0
        vbel3  -35.36   # Vertical (up is negative Z)

        # Aero properties
        cd  0.47        # Drag coefficient (sphere)
        area  0.01      # 10cm diameter ball
    END
ENDTIME 10
STOP
```

### Step 6: Create Makefile

Standard CADAC Makefile listing all source files.

### Step 7: Build and Run

```bash
make
./ball3
```

## Key Lessons

### What You MUST Create

1. **Module .cpp files** - Your physics (environment, kinematics, forces, etc.)
2. **class_hierarchy.hpp** - Your vehicle class definition
3. **input.asc** - Your simulation parameters

### What You Can Copy

1. **Utility files** - Matrix class, global functions (copy as-is)
2. **Framework files** - execution.cpp, class_functions.cpp (adapt names)
3. **Vehicle functions** - ball_functions.cpp (adapt from similar example)

### The Pattern

Every CADAC simulation follows this pattern:

```
1. Define modules (what physics do you need?)
2. Implement def_xxx() functions (allocate variables)
3. Implement init_xxx() functions (set initial values)
4. Implement xxx() exec functions (calculate each timestep)
5. Connect modules in execution.cpp
6. Define parameters in input.asc
```

## Output

After running `./ball3`, you get:

- `plot1.asc` - Time history of position, velocity, altitude
- `doc.asc` - Complete variable documentation
- Screen output - Vehicle name and progress

## Comparison with MAGSIX

| Aspect | MAGSIX (template) | BALL3 (custom) |
|--------|------------------|----------------|
| DoF | 6 (rotation too) | 3 (position only) |
| Modules | trajectory, attitude | kinematics, forces |
| Physics | Magnus rotor | Simple gravity + drag |
| Purpose | Demonstrate framework | Teach creation process |
| Code | Original example | Created from scratch |

## Next Steps

### Modify BALL3

Try changing input.asc:
- Different launch angles
- Different drag coefficients
- Add wind (modify forces.cpp)
- Add thrust (new propulsion.cpp module)

### Create Your Own Simulation

1. Copy BALL3 as template
2. Rename everything (Ball → YourVehicle)
3. Modify modules for your physics
4. Update input.asc for your parameters

### Study Complex Examples

- ROCKET6G - Multiple stages, GPS/INS
- FALCON6 - Full 6DoF with flight controls
- AIM5 - Guidance and targeting

## The Process in Summary

```
Choose complexity (3DoF, 5DoF, 6DoF)
        ↓
Copy utility files from similar example
        ↓
Define your class hierarchy
        ↓
Implement required modules
        ↓
Adapt execution framework
        ↓
Create input file
        ↓
Build and test!
```

**You are NOT limited to the 10 examples!** CADAC is a framework for building custom simulations.

## References

- [CREATING_SIMULATIONS.md](../../CREATING_SIMULATIONS.md) - Detailed guide
- [BUILD_GUIDE.md](../../BUILD_GUIDE.md) - Framework documentation
- [QUICKSTART.md](../../QUICKSTART.md) - Quick reference

---

**Created**: November 2025
**Purpose**: Tutorial / Educational Example
**Status**: Functional (basic implementation)
