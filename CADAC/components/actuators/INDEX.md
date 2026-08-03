# Component Index: rcs_simple

```yaml
component:
  name: rcs_simple
  category: Actuators
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

  - index: 18
    name: IBBB
    type: Matrix(3x3)
    unit: dimensionless
    description: Inertia tensor kg·m²
    source: propulsion

  - index: 320
    name: ppcx
    type: double
    unit: deg/s
    description: Roll rate
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

  - index: 338
    name: phibdcx
    type: double
    unit: deg
    description: Roll angle
    source: unknown

  - index: 339
    name: thtbdcx
    type: double
    unit: deg
    description: Pitch angle
    source: unknown

  - index: 340
    name: psibdcx
    type: double
    unit: deg
    description: Yaw angle
    source: unknown

outputs:
  - index: 64
    name: FMRCS
    type: Matrix(3x1)
    unit: N·m
    description: RCS moments
    scope: inter-module

  - index: 84
    name: FARCS
    type: Matrix(3x1)
    unit: dimensionless
    description: RCS forces N (optional)
    scope: inter-module

parameters:
  - index: 50
    name: mrcs_moment
    type: int
    unit: dimensionless
    description: RCS moment mode (=|type||mode|)
    required: true

  - index: 51
    name: mrcs_force
    type: int
    unit: dimensionless
    description: RCS force mode (0=off, 1=prop)
    required: true

  - index: 55
    name: roll_mom_max
    type: double
    unit: N·m
    description: Max roll moment
    required: true

  - index: 56
    name: pitch_mom_max
    type: double
    unit: N·m
    description: Max pitch moment
    required: true

  - index: 57
    name: yaw_mom_max
    type: double
    unit: N·m
    description: Max yaw moment
    required: true

  - index: 58
    name: rcs_zeta
    type: double
    unit: dimensionless
    description: RCS damping
    required: true

  - index: 59
    name: rcs_freq
    type: double
    unit: rad/s
    description: RCS natural frequency
    required: true

  - index: 70
    name: phibdcomx
    type: double
    unit: deg
    description: Roll angle command
    required: true

  - index: 71
    name: thtbdcomx
    type: double
    unit: deg
    description: Pitch angle command
    required: true

  - index: 72
    name: psibdcomx
    type: double
    unit: deg
    description: Yaw angle command
    required: true

dependencies:
  required_before: ['kinematics', 'propulsion']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  rcs   def,exec
  # In input.asc DATA section:
  mrcs_moment = 11       // Proportional, Euler angle mode
  roll_mom_max = 100     // Max roll moment, N·m
  pitch_mom_max = 500    // Max pitch moment, N·m
  yaw_mom_max = 500      // Max yaw moment, N·m
  rcs_zeta = 0.7         // RCS damping
  rcs_freq = 1.0         // RCS bandwidth, rad/s
  phibdcomx = 0          // Roll command, deg
  thtbdcomx = 0          // Pitch command, deg
  psibdcomx = 0          // Yaw command, deg

notes: []
```

---

# Component Index: tvc_simple

```yaml
component:
  name: tvc_simple
  category: Actuators
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

  - index: 17
    name: xcg
    type: double
    unit: m
    description: CG location from nose
    source: unknown

  - index: 26
    name: thrust
    type: double
    unit: N
    description: Thrust
    source: propulsion

  - index: 520
    name: delecx
    type: double
    unit: deg
    description: Pitch command
    source: unknown

  - index: 521
    name: delrcx
    type: double
    unit: deg
    description: Yaw command
    source: unknown

  - index: 57
    name: pdynmc
    type: double
    unit: dimensionless
    description: Dynamic pressure Pa (for variable gain)
    source: unknown

outputs:
  - index: 910
    name: FPB
    type: Matrix(3x1)
    unit: N
    description: Thrust vector in body
    scope: inter-module

  - index: 911
    name: FMPB
    type: Matrix(3x1)
    unit: N·m
    description: Thrust moments in body
    scope: inter-module

  - index: 912
    name: etax
    type: double
    unit: deg
    description: Nozzle pitch deflection
    scope: inter-module

  - index: 913
    name: zetx
    type: double
    unit: deg
    description: Nozzle yaw deflection
    scope: inter-module

parameters:
  - index: 900
    name: mtvc
    type: int
    unit: dimensionless
    description: TVC mode (0=off, 1=no dynamics, 2=2nd order)
    required: true

  - index: 902
    name: tvclimx
    type: double
    unit: deg
    description: Nozzle deflection limit
    required: true

  - index: 904
    name: dtvclimx
    type: double
    unit: deg/s
    description: Nozzle rate limit
    required: true

  - index: 905
    name: wntvc
    type: double
    unit: rad/s
    description: TVC natural frequency
    required: true

  - index: 906
    name: zettvc
    type: double
    unit: dimensionless
    description: TVC damping
    required: true

  - index: 908
    name: gtvc
    type: double
    unit: dimensionless
    description: TVC gain (deflection per command)
    required: true

  - index: 909
    name: parm
    type: double
    unit: m
    description: Propulsion moment arm from nose
    required: true

dependencies:
  required_before: ['propulsion', 'control']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  tvc   def,exec
  # In input.asc DATA section:
  mtvc = 2            // Second-order dynamics
  gtvc = 1.0          // Unity gain
  tvclimx = 10        // ±10° limit
  dtvclimx = 50       // 50°/s rate limit
  wntvc = 20          // 20 rad/s bandwidth
  zettvc = 0.7        // 0.7 damping
  parm = 5.0          // Moment arm, m from nose

notes: []
```