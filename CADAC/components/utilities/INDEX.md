# Component Index: termination

```yaml
component:
  name: termination
  category: Utilities
  dof: 3DoF
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

  - index: 22
    name: altitude
    type: double
    unit: m
    description: Altitude
    source: unknown

  - index: 80
    name: dta
    type: double
    unit: dimensionless
    description: Range to target m (optional)
    source: unknown

  - index: 162
    name: intercept_flag
    type: int
    unit: dimensionless
    description: Intercept detected (optional)
    source: unknown

outputs:
  - index: 5
    name: stop
    type: int
    unit: dimensionless
    description: Stop flag (1=stop)
    scope: inter-module

  - index: 6
    name: lconv
    type: int
    unit: dimensionless
    description: Convergence flag
    scope: inter-module

parameters:
  - index: 7
    name: endtime
    type: double
    unit: dimensionless
    description: Maximum sim time sec
    required: true

  - index: 8
    name: min_alt
    type: double
    unit: m
    description: Minimum altitude
    required: true

  - index: 9
    name: min_range
    type: double
    unit: m
    description: Minimum target range
    required: true

dependencies:
  required_before: ['various']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  termination   def,exec
  # In input.asc DATA section:
  endtime = 100     // Max simulation time, sec
  min_alt = -10     // Stop if altitude < -10m (ground)
  min_range = 10    // Stop if range < 10m (intercept)

notes: []
```

---

# Component Index: time_management

```yaml
component:
  name: time_management
  category: Utilities
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs: []

outputs:
  - index: 0
    name: time
    type: double
    unit: dimensionless
    description: Simulation time sec
    scope: inter-module

  - index: 1
    name: event_time
    type: double
    unit: dimensionless
    description: Time since last event sec
    scope: inter-module

parameters: []

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  time   def,init,exec

notes: []
```