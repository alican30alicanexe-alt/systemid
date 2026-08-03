#!/usr/bin/env python3
"""
Test BALL3 - Ballistic Cannonball Simulation

This script demonstrates using the CADAC Builder Python API to construct
the BALL3 simulation from modular components.
"""

import sys
from pathlib import Path

# Add cadac_builder to path
sys.path.insert(0, str(Path(__file__).parent))

from pycas import Simulation, Component


def build_ball3():
    """Build BALL3 simulation using Python API"""

    print("Building BALL3 - Ballistic Cannonball Simulation")
    print("=" * 60)

    # Create simulation
    sim = Simulation("BALL3_test")

    # Add required components
    print("\nAdding components...")

    # Time management (required)
    sim.add_component(Component.time_management())

    # Kinematics - 3DoF on flat Earth
    sim.add_component(Component.kinematics_3dof_flat())

    # Dynamics - 3DoF force summation
    sim.add_component(Component.forces_3dof())

    # Aerodynamics - simple drag
    sim.add_component(Component.drag_simple(
        cd=0.47,      # Drag coefficient (sphere)
        area=0.01     # Reference area (10cm diameter)
    ))

    # Environment - constant gravity
    sim.add_component(Component.gravity_constant(grav=9.81))

    # Environment - constant atmosphere
    sim.add_component(Component.atmosphere_constant(rho=1.225))

    # Termination conditions
    sim.add_component(Component.termination(
        altitude_min=0.0,   # Stop when hitting ground
        time_max=10.0       # Or after 10 seconds
    ))

    # Set initial conditions
    # CADAC uses specific variable names from components
    print("\nSetting initial conditions...")
    sim.set_initial_state(
        # Position components (meters in Earth frame)
        sbel1=0.0,     # X position (East)
        sbel2=0.0,     # Y position (North)
        sbel3=0.0,     # Z position (Down)
        # Velocity components (m/s in Earth frame)
        vbel1=35.36,   # X velocity (East)
        vbel2=0.0,     # Y velocity (North)
        vbel3=-35.36,  # Z velocity (Down, negative is up)
    )

    # Set simulation parameters
    print("\nSetting simulation parameters...")
    sim.set_config(
        duration=10.0,     # 10 seconds
        dt=0.01,           # Integration timestep
        output_step=0.05   # Plot output interval
    )

    # Validate configuration
    print("\nValidating configuration...")
    errors = sim.validate()
    if errors:
        print("Validation errors:")
        for error in errors:
            print(f"  - {error}")
        return None

    print("✓ Configuration valid")

    return sim


def main():
    """Main test function"""

    # Build simulation
    sim = build_ball3()
    if not sim:
        print("Failed to build simulation")
        return 1

    # Display simulation info
    print("\n" + str(sim))

    # Generate input file (without compiling)
    print("\nGenerating input file...")
    input_file = sim.generate_input_file()
    print(f"✓ Input file generated: {input_file}")

    # Display input file contents
    print("\n" + "=" * 60)
    print("Generated input.asc:")
    print("=" * 60)
    print(input_file.read_text())

    # TODO: Compilation and running would happen here
    # For now, just show that the API works
    print("\n" + "=" * 60)
    print("BALL3 simulation successfully built using Python API!")
    print("\nNext steps:")
    print("  1. Compile: sim.compile()")
    print("  2. Run: results = sim.run()")
    print("  3. Compare: sim.compare(results, 'BALL3/traj.asc')")
    print("=" * 60)

    # Note about compilation
    print("\n" + "=" * 60)
    print("NOTE: Full compilation requires framework adaptation")
    print("=" * 60)
    print("\nThis demo shows the pyCAS Python API for building simulations.")
    print("For full compilation and execution from modular components, see:")
    print("  tests/regression/test_ball3_regression.py")
    print("\nThe regression test demonstrates:")
    print("  - Framework adaptation (Vehicle → Ball)")
    print("  - Module merging (environment, forces)")
    print("  - Complete C++ compilation")
    print("  - Simulation execution")
    print("  - Trajectory validation")
    print("\n" + "=" * 60)

    # Compile and run the simulation
    print("\nCompiling and running the simulation...")
    sim.compile()
    print(f"✓ Simulation compiled: {sim.executable_path}")
    results = sim.run(duration=10.0, dt=0.01)
    print(f"✓ Simulation results: {results}")

    return 0


if __name__ == '__main__':
    sys.exit(main())
