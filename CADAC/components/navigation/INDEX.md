# Component Index: intercept_simple

```yaml
component:
  name: intercept_simple
  category: Navigation
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

  - index: 80
    name: dta
    type: double
    unit: m
    description: Range to target
    source: unknown

  - index: 81
    name: dvta
    type: double
    unit: m/s
    description: Closing velocity
    source: unknown

outputs:
  - index: 160
    name: miss
    type: double
    unit: m
    description: Miss distance
    scope: inter-module

  - index: 161
    name: tintercept
    type: double
    unit: dimensionless
    description: Time of closest approach sec
    scope: inter-module

  - index: 162
    name: intercept_flag
    type: int
    unit: dimensionless
    description: 1=intercept detected
    scope: inter-module

parameters:
  - index: 163
    name: detect_range
    type: double
    unit: m
    description: Detection sphere radius
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  intercept   def,init,exec
  # In input.asc DATA section:
  detect_range = 500  // Detection sphere, m

notes: []
```

---

# Component Index: target_const_velocity

```yaml
component:
  name: target_const_velocity
  category: Navigation
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs: []

outputs:
  - index: 200
    name: STEL
    type: Matrix(3x1)
    unit: m
    description: Target position in local frame
    scope: inter-module

  - index: 201
    name: VTEL
    type: Matrix(3x1)
    unit: m/s
    description: Target velocity in local frame
    scope: inter-module

parameters:
  - index: 202
    name: stel1
    type: double
    unit: m
    description: Initial target X position (east)
    required: true

  - index: 203
    name: stel2
    type: double
    unit: m
    description: Initial target Y position (north)
    required: true

  - index: 204
    name: stel3
    type: double
    unit: m
    description: Initial target Z position (down)
    required: true

  - index: 205
    name: vtel1
    type: double
    unit: m/s
    description: Target X velocity (east)
    required: true

  - index: 206
    name: vtel2
    type: double
    unit: m/s
    description: Target Y velocity (north)
    required: true

  - index: 207
    name: vtel3
    type: double
    unit: m/s
    description: Target Z velocity (down)
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  target   def,init,exec
  # In input.asc DATA section:
  stel1 = 5000     // Initial 5km east
  stel2 = 10000    // Initial 10km north
  stel3 = -3000    // 3km altitude (Z is down)
  vtel1 = 100      // 100 m/s east
  vtel2 = 50       // 50 m/s north
  vtel3 = 0        // Level flight

notes: []
```

---

# Component Index: target_fixed

```yaml
component:
  name: target_fixed
  category: Navigation
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: true
  exec: true

inputs: []

outputs:
  - index: 200
    name: STEL
    type: Matrix(3x1)
    unit: m
    description: Target position in local frame
    scope: inter-module

  - index: 201
    name: VTEL
    type: Matrix(3x1)
    unit: m/s
    description: Target velocity (zero)
    scope: inter-module

parameters:
  - index: 202
    name: stel1
    type: double
    unit: m
    description: Target X position (east)
    required: true

  - index: 203
    name: stel2
    type: double
    unit: m
    description: Target Y position (north)
    required: true

  - index: 204
    name: stel3
    type: double
    unit: m
    description: Target Z position (down)
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  target   def,init,exec
  # In input.asc DATA section:
  stel1 = 5000    // Target 5km east
  stel2 = 10000   // Target 10km north
  stel3 = 0       // Target at ground level

notes: []
```