# CADAC Regression Tests

This directory contains regression tests for validating CADAC simulations built with the Python API.

## Directory Structure

```
regression/
├── README.md                      # This file
├── reference/                     # Reference trajectories for comparison
│   └── ball3_reference.asc        # Ground truth BALL3 trajectory
├── BALL3/                         # BALL3 full test workspace (WIP)
├── BALL3_simple/                  # BALL3 simplified test workspace
├── test_ball3_regression.py       # Full BALL3 regression test (WIP)
└── test_ball3_simple.py           # Simplified BALL3 validation test ✅
```

## Available Tests

### test_ball3_simple.py ✅ WORKING

**Purpose**: Validates that the CADAC Builder Python API generates correct input.asc files

**Approach**:
1. Uses Python API to build BALL3 simulation configuration
2. Generates input.asc file
3. Uses existing BALL3 example to run simulation
4. Validates trajectory output is generated

**Usage**:
```bash
cd /home/user/CADAC
python3 tests/regression/test_ball3_simple.py
```

**Expected Output**:
```
✅ TEST COMPLETED
✓ Configuration valid
✓ Generated input file
✓ Simulation completed
✓ Generated trajectory with ~405 data points
```

**What It Validates**:
- Python API component construction
- Input file generation
- Module configuration
- Initial conditions setup
- Simulation parameters

### test_ball3_regression.py 🚧 WORK IN PROGRESS

**Purpose**: Full end-to-end test building BALL3 from modular components

**Planned Steps**:
1. Build BALL3 using Python API
2. Copy CADAC framework files
3. Copy component files
4. Compile simulation from components
5. Run simulation
6. Compare trajectory with reference
7. Report RMS and max errors

**Status**:
- ⏸️ Requires framework integration to compile modular components
- Current blocker: CADAC framework expects monolithic class structure
- Solution needed: Adapter layer between modular components and framework

## Reference Trajectories

### ball3_reference.asc

Ground truth trajectory from the original BALL3 example.

**Simulation Parameters**:
- Initial position: (0, 0, 0) meters
- Initial velocity: (35.36, 0, -35.36) m/s (50 m/s at 45°)
- Drag coefficient: 0.47 (sphere)
- Reference area: 0.01 m² (~10cm diameter)
- Integration timestep: 0.01 sec
- Duration: 10 seconds
- Output interval: 0.05 sec

**Expected Behavior**:
- Ballistic trajectory with drag
- Maximum altitude: ~25 meters
- Flight time: ~7.2 seconds
- Range: ~176 meters

## Test Development Guidelines

### Adding New Regression Tests

1. **Create test script** in this directory
2. **Use Python API** to build simulation
3. **Generate reference** trajectory by running validated example
4. **Save reference** to `reference/` directory
5. **Implement comparison** using TrajectoryComparator
6. **Define pass criteria** (RMS thresholds for key variables)
7. **Document** test in this README

### Pass Criteria Guidelines

For ballistic trajectories:
- **Position (SBEL)**: RMS < 0.1 m (10 cm)
- **Velocity (VBEL)**: RMS < 0.01 m/s (1 cm/s)
- **Altitude**: RMS < 0.1 m
- **Speed**: RMS < 0.01 m/s

For controlled trajectories:
- More lenient criteria due to guidance/control variations
- Focus on terminal conditions (final position, impact point)

## Future Tests

Planned regression tests:

1. **ROCKET6G** - Space launch vehicle
   - 6DoF dynamics
   - Multi-stage propulsion
   - Autopilot control
   - TVC actuation

2. **AIM5** - Air-to-air missile
   - Proportional navigation
   - Target tracking
   - Intercept geometry

3. **Monte Carlo** - Statistical validation
   - Multiple runs with parameter variations
   - Statistical comparison of distributions

## Continuous Integration

These tests can be integrated into CI/CD pipelines:

```bash
# Run all regression tests
python3 tests/regression/test_ball3_simple.py
python3 tests/regression/test_rocket6g.py  # When ready

# Or use pytest
pytest tests/regression/
```

## Troubleshooting

### Test Fails to Compile

**Issue**: Component files missing or framework files not found

**Solution**:
- Ensure components are extracted to `components/` directory
- Verify example BALL3 exists in `example/BALL3/`
- Check that framework headers are present

### Trajectory Mismatch

**Issue**: RMS errors exceed threshold

**Possible Causes**:
1. Initial conditions don't match reference
2. Integration timestep differs
3. Component parameters incorrect
4. Numerical precision differences

**Debug Steps**:
1. Compare input.asc files (generated vs reference)
2. Plot trajectories side-by-side
3. Check variable names and units
4. Verify module execution order

### Simulation Doesn't Run

**Issue**: Executable runs but no output generated

**Solution**:
- Check input.asc format matches expected CADAC format
- Verify MODULES section lists all required modules
- Ensure TIMING section is correct
- Check termination conditions aren't too restrictive

## References

- CADAC Framework Documentation
- CADAC Builder Python API: `../cadac_builder/README.md`
- Component Catalog: `../components/CATALOG.md`
- Example Simulations: `../example/`
