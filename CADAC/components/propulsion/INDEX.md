# Component Index: propulsion_staging

```yaml
component:
  name: propulsion_staging
  category: Propulsion
  dof: 6DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 0
    name: time
    type: double
    unit: dimensionless
    description: Simulation time sec
    source: kinematics

  - index: 52
    name: press
    type: double
    unit: Pa
    description: Ambient pressure
    source: environment

  - index: 503
    name: mfreeze
    type: int
    unit: dimensionless
    description: Freeze flag for autopilot testing
    source: unknown

outputs:
  - index: 10
    name: mprop
    type: int
    unit: dimensionless
    description: Propulsion mode
    scope: inter-module

  - index: 15
    name: vmass
    type: double
    unit: kg
    description: Vehicle mass
    scope: inter-module

  - index: 17
    name: xcg
    type: double
    unit: m
    description: CG location from nose
    scope: inter-module

  - index: 18
    name: IBBB
    type: Matrix(3x3)
    unit: dimensionless
    description: Inertia tensor kg·m²
    scope: inter-module

  - index: 22
    name: fmasse
    type: double
    unit: dimensionless
    description: Fuel mass expended kg (state)
    scope: state

  - index: 23
    name: fmassd
    type: double
    unit: dimensionless
    description: Fuel mass rate kg/s (state)
    scope: state

  - index: 26
    name: thrust
    type: double
    unit: N
    description: Thrust
    scope: inter-module

  - index: 27
    name: fmassr
    type: double
    unit: kg
    description: Remaining fuel mass
    scope: inter-module

parameters:
  - index: 10
    name: mprop
    type: int
    unit: dimensionless
    description: Propulsion mode (0=off, 3=const thrust, 4=LTG)
    required: true

  - index: 11
    name: acowl
    type: double
    unit: dimensionless
    description: Cowl area m² (reserved)
    required: true

  - index: 16
    name: vmass0
    type: double
    unit: kg
    description: Initial gross mass
    required: true

  - index: 21
    name: fmass0
    type: double
    unit: kg
    description: Initial fuel mass
    required: true

  - index: 24
    name: aexit
    type: double
    unit: dimensionless
    description: Nozzle exit area m²
    required: true

  - index: 25
    name: spi
    type: double
    unit: dimensionless
    description: Specific impulse sec
    required: true

  - index: 28
    name: xcg_0
    type: double
    unit: m
    description: Initial CG from nose
    required: true

  - index: 29
    name: xcg_1
    type: double
    unit: m
    description: Final CG from nose
    required: true

  - index: 33
    name: fuel_flow_rate
    type: double
    unit: kg/s
    description: Fuel flow rate
    required: true

  - index: 38
    name: moi_roll_0
    type: double
    unit: dimensionless
    description: Initial roll MOI kg·m²
    required: true

  - index: 39
    name: moi_roll_1
    type: double
    unit: dimensionless
    description: Burnout roll MOI kg·m²
    required: true

  - index: 40
    name: moi_trans_0
    type: double
    unit: dimensionless
    description: Initial transverse MOI kg·m²
    required: true

  - index: 41
    name: moi_trans_1
    type: double
    unit: dimensionless
    description: Burnout transverse MOI kg·m²
    required: true

dependencies:
  required_before: ['environment']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  propulsion   def,init,exec
  # In input.asc DATA section:
  mprop = 3               // Constant thrust mode
  vmass0 = 23474          // Initial mass, kg
  fmass0 = 18500          // Fuel mass, kg
  spi = 290               // Specific impulse, sec
  fuel_flow_rate = 150    // Flow rate, kg/s
  aexit = 0.6             // Exit area, m²
  xcg_0 = 3.5             // Initial CG, m
  xcg_1 = 4.2             // Burnout CG, m
  moi_roll_0 = 1500       // Initial roll MOI, kg·m²
  moi_roll_1 = 300        // Burnout roll MOI, kg·m²
  moi_trans_0 = 150000    // Initial transverse MOI, kg·m²
  moi_trans_1 = 30000     // Burnout transverse MOI, kg·m²

notes: []
```

---

# Component Index: rocket_motor_simple

```yaml
component:
  name: rocket_motor_simple
  category: Propulsion
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 0
    name: time
    type: double
    unit: dimensionless
    description: Simulation time sec
    source: kinematics

outputs:
  - index: 50
    name: mprop
    type: int
    unit: dimensionless
    description: Motor status (0=off, 1=on)
    scope: inter-module

  - index: 60
    name: thrust
    type: double
    unit: N
    description: Thrust force
    scope: inter-module

  - index: 61
    name: mass
    type: double
    unit: kg
    description: Vehicle mass
    scope: inter-module

parameters:
  - index: 62
    name: thrust_SL
    type: double
    unit: N
    description: Sea level thrust
    required: true

  - index: 63
    name: burn_time
    type: double
    unit: dimensionless
    description: Motor burn duration sec
    required: true

  - index: 64
    name: mass_init
    type: double
    unit: kg
    description: Initial total mass
    required: true

  - index: 65
    name: mass_prop
    type: double
    unit: kg
    description: Propellant mass
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  propulsion   def,init,exec
  # In input.asc DATA section:
  thrust_SL = 5000    // Thrust, N
  burn_time = 3.5     // Burn duration, sec
  mass_init = 100     // Total initial mass, kg
  mass_prop = 20      // Propellant mass, kg

notes: []
```

---

# Component Index: thrust_constant

```yaml
component:
  name: thrust_constant
  category: Propulsion
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs: []

outputs:
  - index: 80
    name: thrust
    type: double
    unit: N
    description: Thrust force
    scope: inter-module

  - index: 81
    name: mass
    type: double
    unit: kg
    description: Vehicle mass
    scope: inter-module

parameters:
  - index: 82
    name: thrust_sl
    type: double
    unit: N
    description: Thrust magnitude
    required: true

  - index: 83
    name: mass0
    type: double
    unit: kg
    description: Initial mass
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  propulsion   def,init,exec
  # In input.asc DATA section:
  thrust_sl = 5000  // Thrust, N
  mass0 = 100       // Initial mass, kg

notes: []
```

---

# Component Index: thrust_table

```yaml
component:
  name: thrust_table
  category: Propulsion
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 0
    name: time
    type: double
    unit: dimensionless
    description: Simulation time sec
    source: kinematics

outputs:
  - index: 50
    name: mprop
    type: int
    unit: dimensionless
    description: Motor status (0=off, 1=on)
    scope: inter-module

  - index: 60
    name: thrust
    type: double
    unit: N
    description: Thrust force
    scope: inter-module

  - index: 61
    name: mass
    type: double
    unit: kg
    description: Vehicle mass
    scope: inter-module

parameters:
  - index: 64
    name: mass_init
    type: double
    unit: kg
    description: Initial total mass
    required: true

  - index: 65
    name: mass_prop
    type: double
    unit: kg
    description: Propellant mass
    required: true

  - index: 66
    name: burn_time
    type: double
    unit: dimensionless
    description: Total burn duration sec
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  propulsion   def,init,exec
  # In input.asc DATA section:
  mass_init = 100      // Initial mass, kg
  mass_prop = 20       // Propellant mass, kg
  burn_time = 5.0      // Burn duration, sec
  Create thrust_deck.asc with table:
  THRUST_DECK
  thrust_vs_time
  0.0   5000
  1.0   4800
  2.0   4500
  ...
  END

notes: []
```