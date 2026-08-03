# Component Index: actuator_first_order

```yaml
component:
  name: actuator_first_order
  category: Control
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 519
    name: delacx
    type: double
    unit: deg
    description: Commanded aileron deflection
    source: unknown

  - index: 520
    name: delecx
    type: double
    unit: deg
    description: Commanded elevator deflection
    source: unknown

  - index: 521
    name: delrcx
    type: double
    unit: deg
    description: Commanded rudder deflection
    source: unknown

outputs:
  - index: 619
    name: delax
    type: double
    unit: deg
    description: Actual aileron deflection
    scope: inter-module

  - index: 620
    name: delex
    type: double
    unit: deg
    description: Actual elevator deflection
    scope: inter-module

  - index: 621
    name: delrx
    type: double
    unit: deg
    description: Actual rudder deflection
    scope: inter-module

parameters:
  - index: 602
    name: dlimx
    type: double
    unit: deg
    description: Position limit
    required: true

  - index: 606
    name: tauact
    type: double
    unit: dimensionless
    description: Time constant sec
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  actuator   def,init,exec
  # In input.asc DATA section:
  dlimx = 20.0      // Position limit, deg
  tauact = 0.05     // Time constant, sec (0.05 = 50ms)

notes: []
```

---

# Component Index: control_accel_6dof

```yaml
component:
  name: control_accel_6dof
  category: Control
  dof: 6DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 0
    name: time
    type: double
    unit: dimensionless
    description: Simulation time sec
    source: kinematics

  - index: 57
    name: pdynmc
    type: double
    unit: Pa
    description: Dynamic pressure
    source: unknown

  - index: 145
    name: dla
    type: double
    unit: m/s²
    description: Lift slope derivative
    source: unknown

  - index: 147
    name: dma
    type: double
    unit: 1/s²
    description: Pitch moment derivative
    source: unknown

  - index: 148
    name: dmq
    type: double
    unit: 1/s
    description: Pitch damping derivative
    source: unknown

  - index: 149
    name: dmde
    type: double
    unit: 1/s²
    description: Pitch control derivative
    source: unknown

  - index: 150
    name: dyb
    type: double
    unit: m/s²
    description: Side force derivative
    source: unknown

  - index: 152
    name: dnb
    type: double
    unit: 1/s²
    description: Yaw moment derivative
    source: unknown

  - index: 153
    name: dnr
    type: double
    unit: 1/s
    description: Yaw damping derivative
    source: unknown

  - index: 154
    name: dndr
    type: double
    unit: 1/s²
    description: Yaw control derivative
    source: unknown

  - index: 225
    name: dvbe
    type: double
    unit: m/s
    description: Geographic speed
    source: unknown

  - index: 321
    name: qqcx
    type: double
    unit: deg/s
    description: Pitch rate
    source: unknown

  - index: 322
    name: rrcx
    type: double
    unit: deg/s
    description: Yaw rate
    source: unknown

  - index: 334
    name: FSPCB
    type: Matrix(3x1)
    unit: m/s²
    description: Specific force in body
    source: unknown

  - index: 10
    name: mprop
    type: int
    unit: dimensionless
    description: Propulsion mode flag
    source: unknown

  - index: 191
    name: gnmax
    type: double
    unit: dimensionless
    description: Max g available (pitch) g
    source: unknown

  - index: 192
    name: gymax
    type: double
    unit: dimensionless
    description: Max g available (yaw) g
    source: unknown

  - index: 575
    name: alcomx
    type: double
    unit: dimensionless
    description: Lateral accel command g
    source: unknown

  - index: 576
    name: ancomx
    type: double
    unit: dimensionless
    description: Normal accel command g
    source: unknown

outputs:
  - index: 520
    name: delecx
    type: double
    unit: deg
    description: Pitch command
    scope: inter-module

  - index: 521
    name: delrcx
    type: double
    unit: deg
    description: Yaw command
    scope: inter-module

  - index: 514
    name: yyd
    type: double
    unit: dimensionless
    description: Yaw feed-forward derivative m/s² (state)
    scope: state

  - index: 515
    name: yy
    type: double
    unit: dimensionless
    description: Yaw feed-forward integral m/s (state)
    scope: state

  - index: 516
    name: zzd
    type: double
    unit: dimensionless
    description: Pitch feed-forward derivative m/s² (state)
    scope: state

  - index: 517
    name: zz
    type: double
    unit: dimensionless
    description: Pitch feed-forward integral m/s (state)
    scope: state

parameters:
  - index: 500
    name: maut
    type: int
    unit: dimensionless
    description: Autopilot mode (maut=|mauty|mautp|)
    required: true

  - index: 504
    name: waclp
    type: double
    unit: rad/s
    description: Pitch nat freq
    required: true

  - index: 505
    name: zaclp
    type: double
    unit: dimensionless
    description: Pitch damping
    required: true

  - index: 506
    name: paclp
    type: double
    unit: 1/s
    description: Pitch real pole
    required: true

  - index: 509
    name: delimx
    type: double
    unit: deg
    description: Pitch command limiter
    required: true

  - index: 510
    name: drlimx
    type: double
    unit: deg
    description: Yaw command limiter
    required: true

  - index: 525
    name: gainp
    type: double
    unit: s²/m
    description: Pitch prop gain
    required: true

  - index: 568
    name: wacly
    type: double
    unit: rad/s
    description: Yaw nat freq
    required: true

  - index: 569
    name: zacly
    type: double
    unit: dimensionless
    description: Yaw damping
    required: true

  - index: 570
    name: pacly
    type: double
    unit: 1/s
    description: Yaw real pole
    required: true

  - index: 571
    name: gainy
    type: double
    unit: s²/m
    description: Yaw prop gain
    required: true

  - index: 575
    name: alcomx
    type: double
    unit: dimensionless
    description: Lateral accel command g
    required: true

  - index: 576
    name: ancomx
    type: double
    unit: dimensionless
    description: Normal accel command g
    required: true

dependencies:
  required_before: ['aerodynamics', 'kinematics', 'newton']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  control   def,exec
  # In input.asc DATA section:
  maut = 53               // Pitch and yaw accel control
  waclp = 2.0             // Pitch bandwidth, rad/s
  zaclp = 0.7             // Pitch damping
  paclp = 1.5             // Pitch real pole, 1/s
  wacly = 2.0             // Yaw bandwidth, rad/s
  zacly = 0.7             // Yaw damping
  pacly = 1.5             // Yaw real pole, 1/s
  gainp = 0.5             // Pitch prop gain
  gainy = 0.5             // Yaw prop gain
  delimx = 10             // Pitch limit, deg
  drlimx = 10             // Yaw limit, deg
  ancomx = 2.0            // Normal accel command, g
  alcomx = 0.0            // Lateral accel command, g

notes: []
```

---

# Component Index: control_accel_autopilot

```yaml
component:
  name: control_accel_autopilot
  category: Control
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 0
    name: grav
    type: double
    unit: m/s²
    description: Gravity
    source: environment

  - index: 11
    name: area
    type: double
    unit: dimensionless
    description: Reference area m²
    source: unknown

  - index: 14
    name: alpmax
    type: double
    unit: deg
    description: Maximum angle of attack
    source: unknown

  - index: 26
    name: cyaim
    type: double
    unit: dimensionless
    description: Side force coefficient
    source: unknown

  - index: 27
    name: cnaim
    type: double
    unit: dimensionless
    description: Normal force coefficient
    source: unknown

  - index: 28
    name: cnalp
    type: double
    unit: 1/rad
    description: Normal force derivative
    source: unknown

  - index: 29
    name: cybet
    type: double
    unit: 1/rad
    description: Side force derivative
    source: unknown

  - index: 57
    name: pdynmc
    type: double
    unit: Pa
    description: Dynamic pressure
    source: unknown

  - index: 60
    name: thrust
    type: double
    unit: N
    description: Thrust
    source: propulsion

  - index: 61
    name: mass
    type: double
    unit: kg
    description: Vehicle mass
    source: unknown

  - index: 75
    name: dvba
    type: double
    unit: m/s
    description: Airspeed
    source: unknown

  - index: 110
    name: ancomx
    type: double
    unit: dimensionless
    description: Commanded normal accel g's
    source: unknown

  - index: 111
    name: alcomx
    type: double
    unit: dimensionless
    description: Commanded lateral accel g's
    source: unknown

outputs:
  - index: 143
    name: alphax
    type: double
    unit: deg
    description: Angle of attack
    scope: inter-module

  - index: 144
    name: betax
    type: double
    unit: deg
    description: Sideslip angle
    scope: inter-module

parameters:
  - index: 127
    name: ta
    type: double
    unit: dimensionless
    description: Ratio of prop/integral gain
    required: true

  - index: 128
    name: tr
    type: double
    unit: dimensionless
    description: Rate loop time constant sec
    required: true

  - index: 129
    name: gacp
    type: double
    unit: rad/s²
    description: Root locus gain
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  control   def,init,exec
  # In input.asc DATA section:
  ta = 2.0       // Prop/integral gain ratio
  tr = 0.1       // Rate loop time constant, sec
  gacp = 10.0    // Root locus gain, rad/s²
  alphax = 0.0   // Initial angle of attack, deg
  betax = 0.0    // Initial sideslip, deg

notes: []
```

---

# Component Index: control_none

```yaml
component:
  name: control_none
  category: Control
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs: []

outputs: []

parameters: []

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  control   def,exec

notes: []
```

---

# Component Index: control_rate_damping

```yaml
component:
  name: control_rate_damping
  category: Control
  dof: 5DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 164
    name: WBIB
    type: Matrix(3x1)
    unit: rad/s
    description: Body rates
    source: euler

outputs:
  - index: 519
    name: delacx
    type: double
    unit: deg
    description: Aileron command
    scope: inter-module

  - index: 520
    name: delecx
    type: double
    unit: deg
    description: Elevator command
    scope: inter-module

  - index: 521
    name: delrcx
    type: double
    unit: deg
    description: Rudder command
    scope: inter-module

parameters:
  - index: 522
    name: kp_roll
    type: double
    unit: dimensionless
    description: Roll rate gain
    required: true

  - index: 523
    name: kp_pitch
    type: double
    unit: dimensionless
    description: Pitch rate gain
    required: true

  - index: 524
    name: kp_yaw
    type: double
    unit: dimensionless
    description: Yaw rate gain
    required: true

dependencies:
  required_before: ['attitude']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  control   def,exec
  # In input.asc DATA section:
  kp_roll = 10.0    // Roll damping gain, deg/(rad/s)
  kp_pitch = 5.0    // Pitch damping gain
  kp_yaw = 5.0      // Yaw damping gain

notes: []
```