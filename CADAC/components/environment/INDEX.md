# Component Index: atmosphere_constant

```yaml
component:
  name: atmosphere_constant
  category: Environment
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
  environment   def,exec
  Variable index allocation:
  Define vehicle[12] for density in def_environment()

notes: []
```

---

# Component Index: atmosphere_us76

```yaml
component:
  name: atmosphere_us76
  category: Environment
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 22
    name: altitude
    type: double
    unit: m
    description: Altitude above sea level
    source: unknown

  - index: 75
    name: dvba
    type: double
    unit: dimensionless
    description: Speed relative to air m/s (for Mach, pdynmc)
    source: unknown

outputs:
  - index: 52
    name: press
    type: double
    unit: Pa
    description: Atmospheric pressure
    scope: inter-module

  - index: 53
    name: rho
    type: double
    unit: kg/m³
    description: Air density
    scope: inter-module

  - index: 54
    name: vsound
    type: double
    unit: m/s
    description: Speed of sound
    scope: inter-module

  - index: 56
    name: vmach
    type: double
    unit: dimensionless
    description: Mach number (dimensionless)
    scope: inter-module

  - index: 57
    name: pdynmc
    type: double
    unit: Pa
    description: Dynamic pressure
    scope: inter-module

  - index: 58
    name: tempk
    type: double
    unit: K
    description: Temperature
    scope: inter-module

parameters: []

dependencies:
  required_before: ['wind']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  environment   def,exec

notes: []
```

---

# Component Index: gravity_constant

```yaml
component:
  name: gravity_constant
  category: Environment
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

notes: []
```

---

# Component Index: gravity_wgs84_simple

```yaml
component:
  name: gravity_wgs84_simple
  category: Environment
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 22
    name: altitude
    type: double
    unit: m
    description: Altitude above sea level
    source: unknown

outputs:
  - index: 0
    name: grav
    type: double
    unit: m/s^2
    description: Gravity magnitude
    scope: inter-module

parameters: []

dependencies:
  required_before: ['kinematics']
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  environment   def,exec

notes: []
```

---

# Component Index: wind_constant

```yaml
component:
  name: wind_constant
  category: Environment
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 21
    name: VBEL
    type: Matrix(3x1)
    unit: m/s
    description: Velocity in Earth frame
    source: unknown

outputs:
  - index: 74
    name: VAEL
    type: Matrix(3x1)
    unit: m/s
    description: Wind velocity in local coords
    scope: inter-module

  - index: 75
    name: dvba
    type: double
    unit: m/s
    description: Speed relative to air
    scope: inter-module

  - index: 76
    name: VBAL
    type: Matrix(3x1)
    unit: m/s
    description: Velocity relative to air
    scope: inter-module

parameters:
  - index: 64
    name: dvae
    type: double
    unit: m/s
    description: Wind speed
    required: true

  - index: 70
    name: psiwdx
    type: double
    unit: deg
    description: Wind direction from north
    required: true

  - index: 69
    name: vaed3
    type: double
    unit: m/s
    description: Vertical wind (positive down)
    required: true

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  environment   def,exec
  # In input.asc DATA section:
  dvae = 10.0      // Wind speed, m/s
  psiwdx = 90.0    // Wind from east, deg
  vaed3 = 0.0      // No vertical wind, m/s

notes: []
```

---

# Component Index: wind_none

```yaml
component:
  name: wind_none
  category: Environment
  dof: 3DoF
  description: 

lifecycle:
  def: true
  init: false
  exec: true

inputs:
  - index: 21
    name: VBEL
    type: Matrix(3x1)
    unit: m/s
    description: Velocity in Earth frame
    source: unknown

outputs:
  - index: 74
    name: VAEL
    type: Matrix(3x1)
    unit: m/s
    description: Wind velocity (zero)
    scope: inter-module

  - index: 75
    name: dvba
    type: double
    unit: m/s
    description: Speed relative to air
    scope: inter-module

  - index: 76
    name: VBAL
    type: Matrix(3x1)
    unit: m/s
    description: Velocity relative to air
    scope: inter-module

parameters: []

dependencies:
  required_before: []
  required_after: []
  optional: []

usage_example: |
  # In input.asc MODULES section:
  environment   def,exec

notes: []
```