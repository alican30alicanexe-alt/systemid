# Component Index: gps_perfect

```yaml
component:
  name: gps_perfect
  category: Sensors
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 20
    name: SBEL
    type: Matrix(3x1)
    unit: m
    description: True position
    source: unknown

  - index: 21
    name: VBEL
    type: Matrix(3x1)
    unit: m/s
    description: True velocity
    source: unknown

  - index: 22
    name: altitude
    type: double
    unit: m
    description: True altitude
    source: unknown

outputs:
  - index: 180
    name: SBELG
    type: Matrix(3x1)
    unit: m
    description: GPS position
    scope: inter-module

  - index: 181
    name: VBELG
    type: Matrix(3x1)
    unit: m/s
    description: GPS velocity
    scope: inter-module

  - index: 182
    name: altg
    type: double
    unit: m
    description: GPS altitude
    scope: inter-module

parameters: []

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  gps   def,exec

notes: []
```

---

# Component Index: seeker_perfect

```yaml
component:
  name: seeker_perfect
  category: Sensors
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 20
    name: SAEL
    type: Matrix(3x1)
    unit: m
    description: Missile position in local frame
    source: unknown

  - index: 21
    name: VAEL
    type: Matrix(3x1)
    unit: m/s
    description: Missile velocity in local frame
    source: unknown

  - index: 200
    name: STEL
    type: Matrix(3x1)
    unit: m
    description: Target position in local frame
    source: unknown

  - index: 201
    name: VTEL
    type: Matrix(3x1)
    unit: m/s
    description: Target velocity in local frame
    source: unknown

outputs:
  - index: 80
    name: dta
    type: double
    unit: m
    description: Range to target
    scope: inter-module

  - index: 81
    name: dvta
    type: double
    unit: m/s
    description: Closing velocity
    scope: inter-module

  - index: 82
    name: tgo
    type: double
    unit: dimensionless
    description: Time-to-go sec
    scope: inter-module

  - index: 87
    name: UTAA
    type: Matrix(3x1)
    unit: dimensionless
    description: LOS unit vector in missile body
    scope: inter-module

  - index: 88
    name: WOEA
    type: Matrix(3x1)
    unit: rad/s
    description: LOS rate vector in missile body
    scope: inter-module

  - index: 89
    name: STAL
    type: Matrix(3x1)
    unit: m
    description: Relative position vector
    scope: inter-module

parameters: []

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  seeker   def,exec

notes: []
```