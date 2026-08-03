# Component Index: kinematics_3dof_flat

```yaml
component:
  name: kinematics_3dof_flat
  category: Kinematics
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 14
    name: FSPB
    type: Matrix(3x1)
    unit: m/s^2
    description: Specific force (acceleration)
    source: unknown

outputs:
  - index: 20
    name: SBEL
    type: Matrix(3x1)
    unit: m
    description: Position in Earth frame
    scope: inter-module

  - index: 21
    name: VBEL
    type: Matrix(3x1)
    unit: m/s
    description: Velocity in Earth frame
    scope: inter-module

  - index: 22
    name: altitude
    type: double
    unit: m
    description: Altitude above ground
    scope: inter-module

parameters:
  - index: 23
    name: sbel1
    type: double
    unit: m
    description: Initial X position
    required: true

  - index: 24
    name: sbel2
    type: double
    unit: m
    description: Initial Y position
    required: true

  - index: 25
    name: sbel3
    type: double
    unit: dimensionless
    description: Initial Z position m (down is positive)
    required: true

  - index: 26
    name: vbel1
    type: double
    unit: m/s
    description: Initial X velocity
    required: true

  - index: 27
    name: vbel2
    type: double
    unit: m/s
    description: Initial Y velocity
    required: true

  - index: 28
    name: vbel3
    type: double
    unit: m/s
    description: Initial Z velocity
    required: true

dependencies:
  required_before: ['dynamics']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  kinematics   def,init,exec
  # In input.asc DATA section:
  sbel1 = 0      // Initial position X, m
  sbel2 = 0      // Initial position Y, m
  sbel3 = 0      // Initial position Z (negative altitude), m
  vbel1 = 35.36  // Initial velocity X, m/s
  vbel2 = 0      // Initial velocity Y, m/s
  vbel3 = -35.36 // Initial velocity Z (upward), m/s

notes: []
```

---

# Component Index: kinematics_6dof

```yaml
component:
  name: kinematics_6dof
  category: Kinematics
  dof: 6DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 75
    name: dvba
    type: double
    unit: m/s
    description: Airspeed
    source: unknown

  - index: 164
    name: WBIB
    type: Matrix(3x1)
    unit: dimensionless
    description: Body rates rad/s (p,q,r)
    source: euler

  - index: 219
    name: lonx
    type: double
    unit: deg
    description: Longitude
    source: newton

  - index: 220
    name: latx
    type: double
    unit: deg
    description: Latitude
    source: newton

  - index: 221
    name: alt
    type: double
    unit: m
    description: Altitude
    source: newton

  - index: 232
    name: VBED
    type: Matrix(3x1)
    unit: m/s
    description: Velocity in geodetic frame
    source: newton

  - index: 236
    name: VBII
    type: Matrix(3x1)
    unit: m/s
    description: Velocity in inertial frame
    source: newton

  - index: 72
    name: VAED
    type: Matrix(3x1)
    unit: dimensionless
    description: Wind velocity m/s (optional)
    source: unknown

outputs:
  - index: 0
    name: time
    type: double
    unit: dimensionless
    description: Simulation time sec
    scope: inter-module

  - index: 120
    name: TBD
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM body to geodetic
    scope: inter-module

  - index: 121
    name: TBI
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM body to inertial (state)
    scope: state

  - index: 137
    name: psibdx
    type: double
    unit: deg
    description: Yaw angle
    scope: inter-module

  - index: 138
    name: thtbdx
    type: double
    unit: deg
    description: Pitch angle
    scope: inter-module

  - index: 139
    name: phibdx
    type: double
    unit: deg
    description: Roll angle
    scope: inter-module

  - index: 140
    name: alppx
    type: double
    unit: deg
    description: Total angle of attack
    scope: inter-module

  - index: 141
    name: phipx
    type: double
    unit: deg
    description: Aerodynamic roll angle
    scope: inter-module

  - index: 144
    name: alphax
    type: double
    unit: deg
    description: Angle of attack
    scope: inter-module

  - index: 145
    name: betax
    type: double
    unit: deg
    description: Sideslip angle
    scope: inter-module

  - index: 123
    name: ortho_error
    type: double
    unit: dimensionless
    description: DCM orthogonality error
    scope: inter-module

parameters:
  - index: 137
    name: psibdx
    type: double
    unit: deg
    description: Initial yaw
    required: true

  - index: 138
    name: thtbdx
    type: double
    unit: deg
    description: Initial pitch
    required: true

  - index: 139
    name: phibdx
    type: double
    unit: deg
    description: Initial roll
    required: true

dependencies:
  required_before: ['euler', 'newton']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  kinematics   def,init,exec
  # In input.asc DATA section:
  psibdx = 0    // Initial yaw, deg
  thtbdx = 90   // Initial pitch, deg (vertical)
  phibdx = 0    // Initial roll, deg

notes: []
```