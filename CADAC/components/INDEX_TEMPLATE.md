# Component Index Template

This template defines the structure for component INDEX.md files.

## Format Specification

Each component should have an INDEX.md file with the following YAML structure:

```yaml
component:
  name: component_name
  category: Category
  dof: 3  # or 6 or "3/6"
  description: Brief description

lifecycle:
  def: true      # Has def_module() method
  init: true     # Has init_module() method
  exec: true     # Has module() execution method

inputs:
  - index: 0
    name: time
    type: double
    unit: sec
    description: Simulation time
    source: kinematics

  - index: 12
    name: rho
    type: double
    unit: kg/m³
    description: Air density
    source: environment

outputs:
  - index: 13
    name: dvbe
    type: double
    unit: m/s
    description: Speed
    scope: diagnostic

  - index: 14
    name: FSPB
    type: Matrix(3x1)
    unit: m/s²
    description: Specific force in body frame
    scope: inter-module

parameters:
  - index: 10
    name: cd
    type: double
    unit: dimensionless
    description: Drag coefficient
    default: 0.47
    typical_range: [0.1, 2.0]

  - index: 11
    name: area
    type: double
    unit: m²
    description: Reference area
    required: true

dependencies:
  required_before:
    - environment  # Must execute before this module
    - kinematics
  required_after: []  # Must execute after this module
  optional:
    - wind  # Can use if available

usage_example: |
  # In input.asc MODULES section:
  forces   def,exec

  # In input.asc DATA section:
  cd = 0.47
  area = 0.0314

notes:
  - Combines drag and gravity into specific force
  - Handles zero-velocity case
  - Point mass assumption (no lift)
```

## Field Descriptions

### Component Section
- **name**: Component filename (without .cpp extension)
- **category**: One of: Actuators, Aerodynamics, Control, Dynamics, Environment, Guidance, Kinematics, Navigation, Propulsion, Sensors, Utilities
- **dof**: Degrees of freedom (3, 6, or "3/6" for both)
- **description**: One-line summary

### Lifecycle Section
- **def**: Has definition method (usually true)
- **init**: Has initialization method (false for modules that don't need setup)
- **exec**: Has execution method (usually true)

### Inputs Array
Variables read from vehicle array:
- **index**: Vehicle array index or range (e.g., "10-12")
- **name**: Variable name
- **type**: double, int, Matrix(3x1), Matrix(3x3), etc.
- **unit**: Physical unit or "dimensionless"
- **description**: What this variable represents
- **source**: Which module provides this (for dependency tracking)

### Outputs Array
Variables written to vehicle array:
- **index**: Vehicle array index or range
- **name**: Variable name
- **type**: Data type
- **unit**: Physical unit
- **description**: What this variable represents
- **scope**: "diagnostic" (for display), "inter-module" (used by others), "state" (integrated)

### Parameters Array
User-configurable values set in input.asc:
- **index**: Vehicle array index
- **name**: Variable name as used in input.asc
- **type**: Data type
- **unit**: Physical unit
- **description**: What to configure
- **default**: Default value (optional)
- **typical_range**: Reasonable value range (optional)
- **required**: true if must be set by user

### Dependencies Section
- **required_before**: Modules that must execute before this one
- **required_after**: Modules that must execute after this one
- **optional**: Modules that enhance functionality but aren't required

### Usage Example
Literal example showing how to configure in input.asc

### Notes
Array of important usage notes, warnings, or tips
