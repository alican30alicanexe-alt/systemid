# Component Index: euler_6dof

```yaml
component:
  name: euler_6dof
  category: Dynamics
  dof: 6DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs:
  - index: 121
    name: TBI
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM body to inertial
    source: kinematics

  - index: 201
    name: FMB
    type: Matrix(3x1)
    unit: N·m
    description: Applied moment in body frame
    source: forces

  - index: 18
    name: IBBB
    type: Matrix(3x3)
    unit: dimensionless
    description: Inertia tensor body frame kg·m²
    source: propulsion

outputs:
  - index: 160
    name: ppx
    type: double
    unit: deg/s
    description: Roll rate
    scope: inter-module

  - index: 161
    name: qqx
    type: double
    unit: deg/s
    description: Pitch rate
    scope: inter-module

  - index: 162
    name: rrx
    type: double
    unit: deg/s
    description: Yaw rate
    scope: inter-module

  - index: 163
    name: WBEB
    type: Matrix(3x1)
    unit: rad/s
    description: Body rates wrt Earth
    scope: inter-module

  - index: 164
    name: WBIB
    type: Matrix(3x1)
    unit: dimensionless
    description: Body rates wrt inertial rad/s (state)
    scope: state

  - index: 165
    name: WBIBD
    type: Matrix(3x1)
    unit: dimensionless
    description: Angular acceleration rad/s² (state)
    scope: state

  - index: 166
    name: WBII
    type: Matrix(3x1)
    unit: rad/s
    description: Angular velocity in inertial frame
    scope: inter-module

parameters:
  - index: 160
    name: ppx
    type: double
    unit: deg/s
    description: Initial roll rate
    required: true

  - index: 161
    name: qqx
    type: double
    unit: deg/s
    description: Initial pitch rate
    required: true

  - index: 162
    name: rrx
    type: double
    unit: deg/s
    description: Initial yaw rate
    required: true

dependencies:
  required_before: ['forces', 'kinematics', 'propulsion']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  euler   def,init,exec
  # In input.asc DATA section:
  ppx = 0   // Initial roll rate, deg/s
  qqx = 0   // Initial pitch rate, deg/s
  rrx = 0   // Initial yaw rate, deg/s

notes: []
```

---

# Component Index: forces_3dof

```yaml
component:
  name: forces_3dof
  category: Dynamics
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
    unit: m/s²
    description: Gravity magnitude
    source: environment

  - index: 11
    name: area
    type: double
    unit: dimensionless
    description: Reference area m²
    source: unknown

  - index: 25
    name: caaim
    type: double
    unit: dimensionless
    description: Axial force coefficient
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
    description: Thrust force
    source: propulsion

  - index: 61
    name: mass
    type: double
    unit: kg
    description: Vehicle mass
    source: unknown

outputs:
  - index: 14
    name: FSPA
    type: Matrix(3x1)
    unit: m/s²
    description: Specific force (accel) in body
    scope: inter-module

  - index: 150
    name: aax
    type: double
    unit: dimensionless
    description: Axial acceleration g's
    scope: inter-module

  - index: 151
    name: alx
    type: double
    unit: dimensionless
    description: Lateral acceleration g's
    scope: inter-module

  - index: 152
    name: anx
    type: double
    unit: dimensionless
    description: Normal acceleration g's
    scope: inter-module

parameters: []

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  forces   def,exec

notes: []
```

---

# Component Index: forces_6dof

```yaml
component:
  name: forces_6dof
  category: Dynamics
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

  - index: 10
    name: mprop
    type: int
    unit: dimensionless
    description: Propulsion mode flag
    source: unknown

  - index: 26
    name: thrust
    type: double
    unit: N
    description: Thrust force
    source: propulsion

  - index: 50
    name: mrcs_moment
    type: int
    unit: dimensionless
    description: RCS moment mode (0-23)
    source: unknown

  - index: 51
    name: mrcs_force
    type: int
    unit: dimensionless
    description: RCS force mode (0,1,2)
    source: unknown

  - index: 64
    name: FMRCS
    type: Matrix(3x1)
    unit: N·m
    description: RCS moments
    source: actuators

  - index: 84
    name: FARCS
    type: Matrix(3x1)
    unit: N
    description: RCS forces
    source: unknown

  - index: 104
    name: refa
    type: double
    unit: dimensionless
    description: Reference area m²
    source: unknown

  - index: 105
    name: refd
    type: double
    unit: m
    description: Reference length
    source: unknown

  - index: 112
    name: cy
    type: double
    unit: dimensionless
    description: Side force coefficient
    source: unknown

  - index: 113
    name: cll
    type: double
    unit: dimensionless
    description: Roll moment coefficient
    source: unknown

  - index: 114
    name: clm
    type: double
    unit: dimensionless
    description: Pitch moment coefficient
    source: unknown

  - index: 115
    name: cln
    type: double
    unit: dimensionless
    description: Yaw moment coefficient
    source: unknown

  - index: 116
    name: cx
    type: double
    unit: dimensionless
    description: Axial force coefficient
    source: unknown

  - index: 117
    name: cz
    type: double
    unit: dimensionless
    description: Normal force coefficient
    source: unknown

  - index: 900
    name: mtvc
    type: int
    unit: dimensionless
    description: TVC mode flag (0,1,2,3)
    source: unknown

  - index: 910
    name: FPB
    type: Matrix(3x1)
    unit: N
    description: TVC thrust vector
    source: actuators

  - index: 911
    name: FMPB
    type: Matrix(3x1)
    unit: N·m
    description: TVC moments
    source: actuators

outputs:
  - index: 200
    name: FAPB
    type: Matrix(3x1)
    unit: N
    description: Applied force body frame
    scope: inter-module

  - index: 201
    name: FMB
    type: Matrix(3x1)
    unit: N·m
    description: Applied moment body frame
    scope: inter-module

parameters: []

dependencies:
  required_before: ['aerodynamics', 'propulsion', 'environment']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  forces   def,exec
  # In input.asc DATA section:
  refa = 0.1767   // Reference area, m²
  refd = 0.476    // Reference diameter, m

notes: []
```

---

# Component Index: newton_6dof

```yaml
component:
  name: newton_6dof
  category: Dynamics
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

  - index: 62
    name: GRAVG
    type: Matrix(3x1)
    unit: m/s²
    description: Gravity in geocentric frame
    source: environment

  - index: 121
    name: TBI
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM body to inertial
    source: kinematics

  - index: 200
    name: FAPB
    type: Matrix(3x1)
    unit: N
    description: Applied force in body frame
    source: forces

  - index: 15
    name: vmass
    type: double
    unit: kg
    description: Vehicle mass
    source: propulsion

  - index: 503
    name: mfreeze
    type: int
    unit: dimensionless
    description: Freeze flag for autopilot testing
    source: unknown

outputs:
  - index: 219
    name: lonx
    type: double
    unit: deg
    description: Longitude
    scope: inter-module

  - index: 220
    name: latx
    type: double
    unit: deg
    description: Latitude
    scope: inter-module

  - index: 221
    name: alt
    type: double
    unit: m
    description: Altitude
    scope: inter-module

  - index: 222
    name: TVD
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM velocity to geodetic
    scope: inter-module

  - index: 223
    name: TDI
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM geodetic to inertial
    scope: inter-module

  - index: 225
    name: dvbe
    type: double
    unit: m/s
    description: Geographic speed
    scope: inter-module

  - index: 226
    name: dvbi
    type: double
    unit: m/s
    description: Inertial speed
    scope: inter-module

  - index: 227
    name: WEII
    type: Matrix(3x3)
    unit: rad/s
    description: Earth angular velocity (skew-sym)
    scope: inter-module

  - index: 228
    name: psivdx
    type: double
    unit: deg
    description: Heading angle
    scope: inter-module

  - index: 229
    name: thtvdx
    type: double
    unit: deg
    description: Flight path angle
    scope: inter-module

  - index: 230
    name: dbi
    type: double
    unit: m
    description: Distance from Earth center
    scope: inter-module

  - index: 231
    name: TGI
    type: Matrix(3x3)
    unit: dimensionless
    description: DCM geocentric to inertial
    scope: inter-module

  - index: 232
    name: VBED
    type: Matrix(3x1)
    unit: m/s
    description: Geographic velocity in geodetic
    scope: inter-module

  - index: 234
    name: altx
    type: double
    unit: dimensionless
    description: Altitude kft (diagnostic)
    scope: diagnostic

  - index: 235
    name: SBII
    type: Matrix(3x1)
    unit: dimensionless
    description: Inertial position m (state)
    scope: state

  - index: 236
    name: VBII
    type: Matrix(3x1)
    unit: dimensionless
    description: Inertial velocity m/s (state)
    scope: state

  - index: 237
    name: ABII
    type: Matrix(3x1)
    unit: dimensionless
    description: Inertial acceleration m/s² (state)
    scope: state

  - index: 238
    name: grndtrck
    type: double
    unit: m
    description: Ground track distance
    scope: inter-module

  - index: 239
    name: FSPB
    type: Matrix(3x1)
    unit: m/s²
    description: Specific force in body
    scope: inter-module

  - index: 240
    name: ayx
    type: double
    unit: dimensionless
    description: Side acceleration g
    scope: inter-module

  - index: 241
    name: anx
    type: double
    unit: dimensionless
    description: Normal acceleration g
    scope: inter-module

  - index: 242
    name: gndtrkmx
    type: double
    unit: dimensionless
    description: Ground track km
    scope: inter-module

  - index: 243
    name: gndtrnmx
    type: double
    unit: dimensionless
    description: Ground track nm
    scope: inter-module

parameters:
  - index: 217
    name: alpha0x
    type: double
    unit: deg
    description: Initial angle of attack
    required: true

  - index: 218
    name: beta0x
    type: double
    unit: deg
    description: Initial sideslip angle
    required: true

  - index: 219
    name: lonx
    type: double
    unit: deg
    description: Initial longitude
    required: true

  - index: 220
    name: latx
    type: double
    unit: deg
    description: Initial latitude
    required: true

  - index: 221
    name: alt
    type: double
    unit: m
    description: Initial altitude
    required: true

  - index: 225
    name: dvbe
    type: double
    unit: m/s
    description: Initial speed
    required: true

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
  required_before: ['forces', 'propulsion', 'kinematics', 'environment']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  newton   def,init,exec
  # In input.asc DATA section:
  lonx = -80.5   // Initial longitude, deg
  latx = 28.5    // Initial latitude (Cape Canaveral), deg
  alt = 0        // Initial altitude, m
  dvbe = 1       // Initial speed, m/s
  alpha0x = 90   // Initial AOA (vertical launch), deg
  beta0x = 0     // Initial sideslip, deg
  psibdx = 0     // Initial yaw, deg
  thtbdx = 90    // Initial pitch (vertical), deg
  phibdx = 0     // Initial roll, deg

notes: []
```