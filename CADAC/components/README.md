# CADAC Component Library

## Purpose

This directory contains **reusable aerospace simulation components** that you can mix-and-match to build custom simulations without copying entire examples.

## Philosophy

Instead of:
```
"Which example is closest to what I want?"
→ Copy entire example
→ Delete modules I don't need
→ Modify what remains
```

Do this:
```
"What physics do I need?"
→ Pick components from library
→ Assemble simulation
→ Configure with input.asc
```

## Directory Structure

```
components/
├── environment/          # Gravity, atmosphere, wind models
│   ├── gravity_constant.cpp
│   ├── gravity_wgs84.cpp
│   ├── atmosphere_constant.cpp
│   ├── atmosphere_us76.cpp
│   └── wind_constant.cpp
├── kinematics/           # Position/velocity integration
│   ├── kinematics_3dof.cpp
│   ├── kinematics_6dof.cpp
│   └── kinematics_inertial.cpp
├── dynamics/             # Force/moment equations
│   ├── newton_3dof.cpp
│   ├── newton_6dof.cpp
│   └── euler_integration.cpp
├── aerodynamics/         # Aero forces and moments
│   ├── drag_pointmass.cpp
│   ├── aero_3dof.cpp
│   ├── aero_6dof.cpp
│   └── aero_hypersonic.cpp
├── propulsion/           # Thrust and fuel
│   ├── thrust_constant.cpp
│   ├── thrust_curve.cpp
│   ├── rocket_motor.cpp
│   └── jet_engine.cpp
├── guidance/             # Navigation and targeting
│   ├── guidance_none.cpp
│   ├── guidance_proportional_nav.cpp
│   ├── guidance_pure_pursuit.cpp
│   └── guidance_orbital.cpp
├── control/              # Autopilot and stability
│   ├── control_none.cpp
│   ├── control_pid.cpp
│   ├── control_lqr.cpp
│   └── actuator_dynamics.cpp
├── sensors/              # Sensor models
│   ├── gps_simple.cpp
│   ├── gps_realistic.cpp
│   ├── ins_ideal.cpp
│   ├── ins_errors.cpp
│   └── seeker_ir.cpp
└── navigation/           # State estimation
    ├── navigation_ideal.cpp
    ├── navigation_gps_only.cpp
    └── navigation_gps_ins.cpp
```

## Component Categories

### 1. Environment (`environment/`)

**Purpose**: External forces and environmental conditions

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `gravity_constant` | Simple g=9.81 m/s² | Ballistics, low altitude |
| `gravity_wgs84` | Full WGS84 with J2 | Orbital mechanics |
| `atmosphere_constant` | Fixed density | Space, vacuum |
| `atmosphere_us76` | US Standard Atmosphere | Atmospheric flight |
| `wind_constant` | Constant wind vector | Simple dispersion |

### 2. Kinematics (`kinematics/`)

**Purpose**: Integrate position and velocity

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `kinematics_3dof` | Point mass motion | Missiles, ballistics |
| `kinematics_6dof` | Position + attitude | Aircraft, spinning projectiles |
| `kinematics_inertial` | ECI frame integration | Orbital mechanics |

### 3. Dynamics (`dynamics/`)

**Purpose**: Apply forces and moments

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `newton_3dof` | F = ma (point mass) | Simple vehicles |
| `newton_6dof` | F = ma, M = Iα | Full rigid body |
| `euler_integration` | Attitude integration | Spinning objects |

### 4. Aerodynamics (`aerodynamics/`)

**Purpose**: Calculate aerodynamic forces

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `drag_pointmass` | Simple C_d * v² | Cannonball, simple projectile |
| `aero_3dof` | Lift + Drag | Missiles, simple aircraft |
| `aero_6dof` | Full forces + moments | Aircraft, complex missiles |
| `aero_hypersonic` | High-speed effects | Re-entry, hypersonics |

### 5. Propulsion (`propulsion/`)

**Purpose**: Generate thrust

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `thrust_constant` | Fixed thrust | Simple rocket |
| `thrust_curve` | Thrust vs time table | Solid rocket motor |
| `rocket_motor` | Multi-stage with staging | Launch vehicles |
| `jet_engine` | Thrust vs altitude/Mach | Aircraft, cruise missiles |

### 6. Guidance (`guidance/`)

**Purpose**: Compute commanded acceleration/attitude

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `guidance_none` | Ballistic (no guidance) | Unguided projectiles |
| `guidance_proportional_nav` | PNG, APN, TPN | Missiles |
| `guidance_pure_pursuit` | Point at target | Simple homing |
| `guidance_orbital` | Orbital insertion | Launch vehicles |

### 7. Control (`control/`)

**Purpose**: Track commanded values

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `control_none` | No autopilot | Ballistic |
| `control_pid` | PID controller | Simple aircraft/missiles |
| `control_lqr` | Linear Quadratic Regulator | Advanced control |
| `actuator_dynamics` | Fin/TVC lag | Realistic response |

### 8. Sensors (`sensors/`)

**Purpose**: Measure state (with errors)

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `gps_simple` | Perfect position | Testing |
| `gps_realistic` | Position with errors | Realistic navigation |
| `ins_ideal` | Perfect inertial | Truth data |
| `ins_errors` | Gyro/accel drift | Realistic INS |
| `seeker_ir` | IR seeker model | Heat-seeking missiles |

### 9. Navigation (`navigation/`)

**Purpose**: Estimate state from sensors

| Component | What It Provides | Use Case |
|-----------|------------------|----------|
| `navigation_ideal` | Perfect state knowledge | Testing |
| `navigation_gps_only` | GPS position only | Simple systems |
| `navigation_gps_ins` | Kalman filter fusion | Modern systems |

## How to Use

### Step 1: Choose Components

Decide what physics you need:

```
I want a guided missile with:
- 3DoF motion (point mass)
- Gravity (simple)
- Atmosphere (realistic)
- Aerodynamics (lift + drag)
- Propulsion (solid rocket)
- Guidance (proportional navigation)
- Control (PID autopilot)
```

### Step 2: Create Module List

Map to components:

```cpp
// My missile simulation modules:
environment:    gravity_constant + atmosphere_us76
kinematics:     kinematics_3dof
dynamics:       newton_3dof
aerodynamics:   aero_3dof
propulsion:     rocket_motor
forces:         (sum all forces - always needed)
guidance:       guidance_proportional_nav
control:        control_pid
```

### Step 3: Copy Components

```bash
cp components/environment/gravity_constant.cpp my_missile/
cp components/environment/atmosphere_us76.cpp my_missile/
cp components/kinematics/kinematics_3dof.cpp my_missile/
cp components/dynamics/newton_3dof.cpp my_missile/
cp components/aerodynamics/aero_3dof.cpp my_missile/
cp components/propulsion/rocket_motor.cpp my_missile/
cp components/guidance/guidance_proportional_nav.cpp my_missile/
cp components/control/control_pid.cpp my_missile/
```

### Step 4: Create Forces Module

The forces module sums all contributions:

```cpp
void MyMissile::forces(double int_step) {
    // Get individual force components
    Matrix GRAV = environment_gravity();
    Matrix AERO = aerodynamics_force();
    Matrix THRUST = propulsion_thrust();

    // Sum them
    Matrix FTOTAL = GRAV + AERO + THRUST;

    // Store for dynamics module
    missile[XX].gets_vec(FTOTAL);
}
```

### Step 5: Update input.asc

Configure your chosen components:

```
MODULES
    environment   def,exec
    aerodynamics  def,exec
    propulsion    def,exec
    forces        def,exec
    guidance      def,init,exec
    control       def,init,exec
    dynamics      def,exec
    kinematics    def,init,exec
END
```

## Component Interface Standards

All components follow these rules:

### Variable Naming

- **Inputs**: Component reads from vehicle array (e.g., `vehicle[10].vec()`)
- **Outputs**: Component writes to vehicle array (e.g., `vehicle[20].gets_vec(result)`)
- **Indices**: Documented in component header

### Function Signatures

```cpp
// Definition (allocate variables)
void MyVehicle::def_module_name();

// Initialization (set initial values)
void MyVehicle::init_module_name();

// Execution (compute each timestep)
void MyVehicle::module_name(double int_step);

// Termination (cleanup)
void MyVehicle::term_module_name();
```

### Documentation Format

Each component includes:

```cpp
///////////////////////////////////////////////////////////////////////////////
// COMPONENT: component_name
// CATEGORY: category (environment, guidance, etc.)
// DoF: 3DoF or 6DoF
//
// DESCRIPTION:
//   Brief description of what it does
//
// INPUTS (from vehicle array):
//   vehicle[10] - variable_name - units - description
//   vehicle[11] - ...
//
// OUTPUTS (to vehicle array):
//   vehicle[20] - variable_name - units - description
//
// PARAMETERS (from input.asc):
//   param_name - default_value - description
//
// DEPENDENCIES:
//   - Requires: (list other modules needed)
//   - Provides: (list what this enables)
//
// REFERENCE:
//   (Book, paper, or standard where equations came from)
///////////////////////////////////////////////////////////////////////////////
```

## Example: Building a Custom Simulation

See `examples/` directory (coming soon) for complete examples of:
- Simple ballistic projectile (BALL3 - already exists)
- Guided missile from components
- Multi-stage rocket from components
- Aircraft from components

## Python Constructor (Future)

Eventually, build simulations with:

```python
from cadac import Simulation, Components

sim = Simulation(name="MyMissile", dof=3)
sim.add_component(Components.Environment.GravityConstant())
sim.add_component(Components.Environment.AtmosphereUS76())
sim.add_component(Components.Kinematics.Kinematics3DoF())
sim.add_component(Components.Aerodynamics.Aero3DoF(
    cd_file="drag_table.csv"
))
sim.add_component(Components.Propulsion.RocketMotor(
    thrust_curve="motor.csv",
    burn_time=10.0
))
sim.add_component(Components.Guidance.ProportionalNav(
    N=3.0  # Navigation constant
))

sim.generate(output_dir="my_missile_sim/")
sim.compile()
sim.run(input_file="scenario1.asc")
```

## Contributing

To add a new component:

1. Write standalone .cpp file following interface standards
2. Document inputs, outputs, parameters
3. Test in isolation
4. Add to appropriate category directory
5. Update this README

## License

Same as parent CADAC repository (Academic, free to use and modify)
