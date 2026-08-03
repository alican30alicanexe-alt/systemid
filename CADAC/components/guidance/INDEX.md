# Component Index: guidance_none

```yaml
component:
  name: guidance_none
  category: Guidance
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
  guidance   def,exec

notes: []
```

---

# Component Index: guidance_pitch_program

```yaml
component:
  name: guidance_pitch_program
  category: Guidance
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

  - index: 229
    name: thtvdx
    type: double
    unit: deg
    description: Current flight path angle
    source: unknown

outputs:
  - index: 400
    name: mguide
    type: int
    unit: dimensionless
    description: Guidance mode flag
    scope: inter-module

  - index: 481
    name: alphacomx
    type: double
    unit: deg
    description: Alpha command
    scope: inter-module

  - index: 482
    name: betacomx
    type: double
    unit: deg
    description: Beta command
    scope: inter-module

  - index: 485
    name: thtvdxcom
    type: double
    unit: deg
    description: Flight path angle command
    scope: inter-module

parameters:
  - index: 400
    name: mguide
    type: int
    unit: dimensionless
    description: Guidance mode (0=none, 1=pitch program)
    required: true

  - index: 486
    name: npitch
    type: int
    unit: dimensionless
    description: Number of pitch program points
    required: true

dependencies:
  required_before: ['kinematics']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  guidance   def,exec
  # In input.asc DATA section:
  mguide = 1          // Pitch program mode
  npitch = 5          // Number of breakpoints
  // Time table (sec)
  time_table = 0, 10, 30, 60, 120
  // Pitch table (deg) - flight path angle
  pitch_table = 90, 85, 60, 30, 10

notes: []
```

---

# Component Index: guidance_proportional_nav

```yaml
component:
  name: guidance_proportional_nav
  category: Guidance
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 30
    name: gmax
    type: double
    unit: dimensionless
    description: Maximum g available g's
    source: unknown

  - index: 81
    name: dvta
    type: double
    unit: m/s
    description: Closing velocity
    source: unknown

  - index: 87
    name: UTAA
    type: Matrix(3x1)
    unit: dimensionless
    description: Unit LOS vector in missile body coords
    source: unknown

  - index: 88
    name: WOEA
    type: Matrix(3x1)
    unit: rad/s
    description: LOS rate vector
    source: unknown

outputs:
  - index: 110
    name: ancomx
    type: double
    unit: dimensionless
    description: Normal acceleration command g's
    scope: inter-module

  - index: 111
    name: alcomx
    type: double
    unit: dimensionless
    description: Lateral acceleration command g's
    scope: inter-module

parameters:
  - index: 101
    name: gnav
    type: double
    unit: dimensionless
    description: Navigation gain (typically 3-5)
    required: true

dependencies:
  required_before: ['intercept']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  guidance   def,exec
  # In input.asc DATA section:
  gnav = 3.0     // Navigation gain (3-5 typical)

notes: []
```