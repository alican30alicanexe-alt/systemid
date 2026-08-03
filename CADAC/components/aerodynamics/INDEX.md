# Component Index: aero_3dof_table

```yaml
component:
  name: aero_3dof_table
  category: Aerodynamics
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 56
    name: vmach
    type: double
    unit: dimensionless
    description: Mach number
    source: unknown

  - index: 57
    name: pdynmc
    type: double
    unit: Pa
    description: Dynamic pressure
    source: unknown

  - index: 61
    name: mass
    type: double
    unit: kg
    description: Vehicle mass
    source: unknown

  - index: 143
    name: alphax
    type: double
    unit: deg
    description: Angle of attack
    source: unknown

  - index: 144
    name: betax
    type: double
    unit: deg
    description: Sideslip angle
    source: unknown

outputs:
  - index: 11
    name: area
    type: double
    unit: dimensionless
    description: Reference area m²
    scope: inter-module

  - index: 20
    name: alppx
    type: double
    unit: deg
    description: Total angle of attack
    scope: inter-module

  - index: 21
    name: phipx
    type: double
    unit: deg
    description: Aerodynamic roll angle
    scope: inter-module

  - index: 23
    name: claim
    type: double
    unit: dimensionless
    description: Lift coefficient
    scope: inter-module

  - index: 24
    name: cdaim
    type: double
    unit: dimensionless
    description: Drag coefficient
    scope: inter-module

  - index: 25
    name: caaim
    type: double
    unit: dimensionless
    description: Axial force coefficient
    scope: inter-module

  - index: 26
    name: cyaim
    type: double
    unit: dimensionless
    description: Side force coefficient
    scope: inter-module

  - index: 27
    name: cnaim
    type: double
    unit: dimensionless
    description: Normal force coefficient
    scope: inter-module

  - index: 28
    name: cnalp
    type: double
    unit: 1/rad
    description: Normal force derivative
    scope: inter-module

  - index: 29
    name: cybet
    type: double
    unit: 1/rad
    description: Side force derivative
    scope: inter-module

  - index: 30
    name: gmax
    type: double
    unit: dimensionless
    description: Maximum g's available
    scope: inter-module

parameters:
  - index: 11
    name: area
    type: double
    unit: dimensionless
    description: Reference area m²
    required: true

  - index: 14
    name: alpmax
    type: double
    unit: deg
    description: Maximum angle of attack
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  aerodynamics   def,exec
  # In input.asc DATA section:
  area = 0.0314      // Reference area, m²
  alpmax = 30        // Max angle of attack, deg
  Create aero_deck.asc with tables

notes: []
```

---

# Component Index: aerodynamics_6dof

```yaml
component:
  name: aerodynamics_6dof
  category: Aerodynamics
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

  - index: 53
    name: rho
    type: double
    unit: kg/m³
    description: Air density
    source: environment

  - index: 56
    name: vmach
    type: double
    unit: dimensionless
    description: Mach number
    source: unknown

  - index: 57
    name: pdynmc
    type: double
    unit: Pa
    description: Dynamic pressure
    source: unknown

  - index: 75
    name: dvba
    type: double
    unit: m/s
    description: Airspeed
    source: unknown

  - index: 140
    name: alppx
    type: double
    unit: deg
    description: Total angle of attack
    source: unknown

  - index: 141
    name: phipx
    type: double
    unit: deg
    description: Aerodynamic roll angle
    source: unknown

  - index: 144
    name: alphax
    type: double
    unit: deg
    description: Angle of attack
    source: unknown

  - index: 145
    name: betax
    type: double
    unit: deg
    description: Sideslip angle
    source: unknown

  - index: 160
    name: ppx
    type: double
    unit: deg/s
    description: Roll rate
    source: euler

  - index: 161
    name: qqx
    type: double
    unit: deg/s
    description: Pitch rate
    source: euler

  - index: 162
    name: rrx
    type: double
    unit: deg/s
    description: Yaw rate
    source: euler

  - index: 10
    name: mprop
    type: int
    unit: dimensionless
    description: Propulsion mode (for base drag)
    source: unknown

  - index: 15
    name: vmass
    type: double
    unit: kg
    description: Vehicle mass
    source: propulsion

  - index: 17
    name: xcg
    type: double
    unit: m
    description: CG location from nose
    source: unknown

outputs:
  - index: 104
    name: refa
    type: double
    unit: dimensionless
    description: Reference area m²
    scope: inter-module

  - index: 105
    name: refd
    type: double
    unit: m
    description: Reference length
    scope: inter-module

  - index: 108
    name: xcg_ref
    type: double
    unit: m
    description: Reference CG location
    scope: inter-module

  - index: 112
    name: cy
    type: double
    unit: dimensionless
    description: Side force coefficient
    scope: inter-module

  - index: 113
    name: cll
    type: double
    unit: dimensionless
    description: Roll moment coefficient
    scope: inter-module

  - index: 114
    name: clm
    type: double
    unit: dimensionless
    description: Pitch moment coefficient
    scope: inter-module

  - index: 115
    name: cln
    type: double
    unit: dimensionless
    description: Yaw moment coefficient
    scope: inter-module

  - index: 116
    name: cx
    type: double
    unit: dimensionless
    description: Axial force coefficient
    scope: inter-module

  - index: 117
    name: cz
    type: double
    unit: dimensionless
    description: Normal force coefficient
    scope: inter-module

  - index: 118
    name: ca0
    type: double
    unit: dimensionless
    description: Axial force coeff(Mach)
    scope: inter-module

  - index: 119
    name: caa
    type: double
    unit: dimensionless
    description: Delta axial force due to alpha
    scope: inter-module

  - index: 120
    name: cn0
    type: double
    unit: dimensionless
    description: Normal force coeff(Mach,alpha)
    scope: inter-module

  - index: 121
    name: clm0
    type: double
    unit: dimensionless
    description: Pitch moment coeff(Mach,alpha)
    scope: inter-module

  - index: 122
    name: clmq
    type: double
    unit: 1/deg
    description: Pitch damping derivative
    scope: inter-module

  - index: 123
    name: cla
    type: double
    unit: 1/deg
    description: Lift slope derivative
    scope: inter-module

  - index: 133
    name: cma
    type: double
    unit: 1/deg
    description: Pitch moment derivative
    scope: inter-module

  - index: 191
    name: gnmax
    type: double
    unit: dimensionless
    description: Max g available (pitch) g
    scope: inter-module

  - index: 192
    name: gymax
    type: double
    unit: dimensionless
    description: Max g available (yaw) g
    scope: inter-module

parameters:
  - index: 100
    name: maero
    type: int
    unit: dimensionless
    description: Aero mode (11=1-stage, 12=2-stage, 13=3-stage)
    required: true

  - index: 104
    name: refa
    type: double
    unit: dimensionless
    description: Reference area m²
    required: true

  - index: 105
    name: refd
    type: double
    unit: m
    description: Reference length
    required: true

  - index: 108
    name: xcg_ref
    type: double
    unit: m
    description: Reference CG location
    required: true

  - index: 187
    name: alplimx
    type: double
    unit: deg
    description: Alpha limiter
    required: true

  - index: 188
    name: alimitx
    type: double
    unit: dimensionless
    description: Structural g limiter g
    required: true

dependencies:
  required_before: ['environment', 'kinematics', 'propulsion']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  aerodynamics   def,init,exec
  # In input.asc DATA section:
  maero = 13          // 3-stage configuration
  refa = 1.767        // Reference area, m²
  refd = 1.5          // Reference diameter, m
  xcg_ref = 8.5       // Reference CG, m from nose
  alplimx = 20        // Max alpha, deg
  alimitx = 5         // Structural limit, g
  AERO_DECK aero_deck_SLV.asc

notes: []
```

---

# Component Index: drag_simple

```yaml
component:
  name: drag_simple
  category: Aerodynamics
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 0
    name: grav
    type: double
    unit: m/s^2
    description: Gravity acceleration
    source: environment

  - index: 12
    name: rho
    type: double
    unit: kg/m^3
    description: Air density
    source: environment

  - index: 21
    name: VBEL
    type: Matrix(3x1)
    unit: m/s
    description: Velocity in Earth frame
    source: unknown

outputs:
  - index: 13
    name: dvbe
    type: double
    unit: m/s
    description: Speed
    scope: inter-module

  - index: 14
    name: FSPB
    type: Matrix(3x1)
    unit: m/s^2
    description: Specific force
    scope: inter-module

parameters:
  - index: 10
    name: cd
    type: double
    unit: dimensionless
    description: Drag coefficient (dimensionless)
    required: true

  - index: 11
    name: area
    type: double
    unit: m^2
    description: Reference area
    required: true

dependencies:
  required_before: ['environment', 'kinematics']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  forces   def,exec
  # In input.asc DATA section:
  cd = 0.47      // Drag coefficient (sphere = 0.47)
  area = 0.0314  // Reference area (10cm radius sphere), m^2

notes: []
```