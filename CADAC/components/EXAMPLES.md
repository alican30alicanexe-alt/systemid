# Component Assembly Examples

This guide shows how to build complete simulations from components.

## Example 1: Ballistic Projectile (Recreating BALL3)

### Components Needed

```
components/environment/gravity_constant.cpp
components/environment/atmosphere_constant.cpp
components/kinematics/kinematics_3dof_flat.cpp
components/aerodynamics/drag_simple.cpp
components/guidance/guidance_none.cpp
components/control/control_none.cpp
```

### Step 1: Create Vehicle Class Header

Create `class_hierarchy.hpp`:

```cpp
#ifndef class_hierarchy__HPP
#define class_hierarchy__HPP

#include "global_header.hpp"

class Cadac; // Abstract base class

class Ball : public Cadac {
protected:
    Variable *ball;   // Vehicle module-variable array

public:
    Ball();
    virtual ~Ball();

    // Module function declarations
    virtual void def_environment();
    virtual void environment(double int_step);

    virtual void def_forces();
    virtual void forces(double int_step);

    virtual void def_kinematics();
    virtual void init_kinematics();
    virtual void kinematics(double int_step);

    // Note: guidance and control are not needed (placeholders only)
};

// Pointer to generic Ball object
extern Ball *ball_ptr;

#endif
```

### Step 2: Copy Component Functions

1. **Copy gravity_constant.cpp** → Rename functions from `Vehicle::` to `Ball::`
2. **Copy atmosphere_constant.cpp** → Merge into same file as gravity (both are environment)
3. **Copy drag_simple.cpp** → Rename to `Ball::forces()`
4. **Copy kinematics_3dof_flat.cpp** → Rename to `Ball::kinematics()`

### Step 3: Merge Environment Components

Create `environment.cpp`:

```cpp
#include "class_hierarchy.hpp"

void Ball::def_environment()
{
    // From gravity_constant
    ball[0].init("grav","D",0,"Gravity acceleration - m/s^2","environment","out","");

    // From atmosphere_constant
    ball[12].init("rho",0,"Air density - kg/m^3","environment","out","");
}

void Ball::environment(double int_step)
{
    // Gravity
    double grav = 9.81;
    ball[0].gets(grav);

    // Atmosphere
    double rho = 1.225;
    ball[12].gets(rho);
}
```

### Step 4: Create Forces Module

Copy `drag_simple.cpp` and rename:

```cpp
#include "class_hierarchy.hpp"

void Ball::def_forces()
{
    // Variable definitions from drag_simple component
    ball[10].init("cd",0,"Drag coefficient - ND","data","","");
    ball[11].init("area",0,"Reference area - m^2","data","","");
    ball[13].init("dvbe",0,"Speed - m/s","out","scrn","plot");
    ball[14].init("FSPB",0,0,0,"Specific force - m/s^2","out","","");
}

void Ball::forces(double int_step)
{
    // Implementation from drag_simple component
    // (copy entire function, change vehicle[] to ball[])
    ...
}
```

### Step 5: Create Kinematics Module

Copy `kinematics_3dof_flat.cpp` and rename:

```cpp
#include "class_hierarchy.hpp"

void Ball::def_kinematics()
{
    // From kinematics_3dof_flat component
    ball[20].init("SBEL",0,0,0,"Position in Earth frame - m","state","","plot");
    ball[21].init("VBEL",0,0,0,"Velocity in Earth frame - m/s","state","","plot");
    ball[22].init("altitude",0,"Altitude above ground - m","out","scrn","plot");
    ball[23].init("sbel1",0,"Initial X position - m","data","","");
    ball[24].init("sbel2",0,"Initial Y position - m","data","","");
    ball[25].init("sbel3",0,"Initial Z position - m","data","","");
    ball[26].init("vbel1",0,"Initial X velocity - m/s","data","","");
    ball[27].init("vbel2",0,"Initial Y velocity - m/s","data","","");
    ball[28].init("vbel3",0,"Initial Z velocity - m/s","data","","");
}

void Ball::init_kinematics()
{
    // From kinematics_3dof_flat component
    // (copy entire function, change vehicle[] to ball[])
    ...
}

void Ball::kinematics(double int_step)
{
    // From kinematics_3dof_flat component
    // (copy entire function, change vehicle[] to ball[])
    ...
}
```

### Step 6: Create Execution File

Create `execution.cpp`:

```cpp
#include "class_hierarchy.hpp"

Ball::Ball()
{
    // Allocate module-variable array
    ball = new Variable[NBALL];
}

Ball::~Ball()
{
    delete [] ball;
}

void Ball::size_arrays()
{
    // Called by simulation
}

void Ball::initialization()
{
    // Call all init functions
    init_kinematics();
}

void Ball::execute()
{
    // Call all exec functions in order
    environment(int_step);
    forces(int_step);
    kinematics(int_step);
}
```

### Step 7: Create Input File

Create `input.asc`:

```
TITLE Ballistic Projectile from Components
MONTE 1
MODULES
    environment   def,exec
    forces        def,exec
    kinematics    def,init,exec
END

TIMING
    scrn_step 0.1
    plot_step 0.01
    int_step 0.001
END

VEHICLES 1
    BALL3 Ball
        // Initial conditions
        sbel1 = 0
        sbel2 = 0
        sbel3 = 0
        vbel1 = 35.36
        vbel2 = 0
        vbel3 = -35.36

        // Aerodynamics
        cd = 0.47
        area = 0.0314
    END
END

ENDTIME 10
STOP
```

### Step 8: Compile

```bash
g++ -c *.cpp -I../../core
g++ -o ball3.exe *.o ../../core/libcadac.a -lm
```

### Step 9: Run

```bash
./ball3.exe input.asc
```

---

## Example 2: Guided Missile (Future)

### Components Needed

```
components/environment/gravity_constant.cpp
components/environment/atmosphere_constant.cpp
components/kinematics/kinematics_3dof_flat.cpp
components/aerodynamics/drag_simple.cpp         # or aero_3dof_table
components/propulsion/thrust_constant.cpp
components/guidance/guidance_proportional_nav.cpp
components/control/control_none.cpp             # or control_pid when available
components/sensors/gps_simple.cpp               # when available
components/intercept/intercept_geometry.cpp     # when available
```

### Assembly Steps

Similar to Example 1:
1. Create vehicle class
2. Copy component functions
3. Rename from `Vehicle::` to `Missile::`
4. Change `vehicle[]` to `missile[]`
5. Create execution order
6. Configure input.asc

---

## Tips for Assembly

### Variable Index Management

Each component documents its variable indices. Ensure no conflicts:

```cpp
// From components:
vehicle[0]  = grav         (gravity_constant)
vehicle[12] = rho          (atmosphere_constant)
vehicle[10] = cd           (drag_simple)
vehicle[14] = FSPB         (drag_simple output)
vehicle[20] = SBEL         (kinematics_3dof_flat)
vehicle[21] = VBEL         (kinematics_3dof_flat)
```

If indices conflict, choose one component to use different indices.

### Module Execution Order

Critical order for physics:
```
1. environment   (provides gravity, atmosphere)
2. forces        (uses environment, provides FSPB)
3. kinematics    (uses FSPB, integrates position/velocity)
```

For guided vehicles:
```
1. environment
2. sensors       (measures state)
3. navigation    (estimates state from sensors)
4. guidance      (computes desired trajectory)
5. control       (commands to track guidance)
6. aerodynamics  (uses control commands)
7. propulsion
8. forces        (sums all forces)
9. kinematics    (integrates)
```

### Merging Multiple Components into One Module

You can merge related components into a single module file:

```cpp
// environment.cpp contains both:
void Ball::environment(double int_step)
{
    // Gravity calculation (from gravity_constant)
    double grav = 9.81;
    ball[0].gets(grav);

    // Atmosphere calculation (from atmosphere_constant)
    double rho = 1.225;
    ball[12].gets(rho);
}
```

### Function Renaming Pattern

```bash
# Search and replace pattern:
Vehicle::      →  YourClass::
vehicle[       →  yourarray[
Vehicle        →  YourClass
```

---

## Validation

Compare your assembled simulation against the original:

```bash
# Run original
cd example/BALL3
./ball3.exe input.asc

# Run your assembled version
cd your_simulation/
./your_sim.exe input.asc

# Compare trajectories
diff traj.asc ../../example/BALL3/traj.asc
```

Small differences (< 0.1%) are acceptable due to compiler/rounding.

---

## Next Steps

Once comfortable assembling from components:
1. Mix and match different components (e.g., use gravity_wgs84 instead of gravity_constant)
2. Create your own components following the standard interface
3. Build completely custom simulations for your specific problem
