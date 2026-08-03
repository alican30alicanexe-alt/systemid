///////////////////////////////////////////////////////////////////////////////
// COMPONENT: forces_3dof
// CATEGORY: Dynamics
// DoF: 3DoF
//
// DESCRIPTION:
//   Sums all forces acting on point mass vehicle
//   Computes total specific force (acceleration) for kinematics
//   Use for: 3DoF missiles, aircraft, projectiles
//
// INPUTS (from vehicle array):
//   vehicle[0]  - grav - double - Gravity magnitude m/s²
//   vehicle[11] - area - double - Reference area m²
//   vehicle[25] - caaim - double - Axial force coefficient
//   vehicle[26] - cyaim - double - Side force coefficient
//   vehicle[27] - cnaim - double - Normal force coefficient
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[60] - thrust - double - Thrust force N
//   vehicle[61] - mass - double - Vehicle mass kg
//
// OUTPUTS (to vehicle array):
//   vehicle[14] - FSPA - Matrix(3x1) - Specific force (accel) in body m/s²
//   vehicle[150] - aax - double - Axial acceleration g's
//   vehicle[151] - alx - double - Lateral acceleration g's
//   vehicle[152] - anx - double - Normal acceleration g's
//
// PARAMETERS (from input.asc):
//   None (uses forces from other modules)
//
// DEPENDENCIES:
//   - Requires: Aerodynamics for force coefficients
//   - Requires: Propulsion for thrust and mass
//   - Requires: Environment for dynamic pressure and gravity
//   - Provides: Specific force for kinematics integration
//
// REFERENCE:
//   Standard 3DoF force summation
//   F_specific = (F_aero + F_thrust + F_gravity) / mass
//
// USAGE:
//   Include in class header:
//     virtual void def_forces();
//     virtual void forces(double int_step);
//
//   In input.asc MODULES section:
//     forces   def,exec
//
// NOTES:
//   - Body frame: X=forward, Y=right, Z=down
//   - Specific force = acceleration in body frame
//   - Gravity is added in kinematics (already in FSPB for some sims)
//   - For 6DoF, use forces_6dof + moments_6dof components
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of forces module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_forces()
{
	//Definition of module-variables
	//output to other modules
	vehicle[14].init("FSPA",0,0,0,"Specific force in body frame - m/s^2","forces","out","");
	//diagnostics
	vehicle[150].init("aax",0,"Axial acceleration - g's","forces","diag","");
	vehicle[151].init("alx",0,"Lateral acceleration - g's","forces","diag","scrn,plot");
	vehicle[152].init("anx",0,"Normal acceleration - g's","forces","diag","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Forces module
//Member function of class 'Vehicle'
//
// Sums aerodynamic, propulsive, and gravitational forces
// Computes specific force (force per unit mass = acceleration)
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::forces(double int_step)
{
	//local module-variables
	Matrix FSPA(3,1);
	double aax(0);
	double alx(0);
	double anx(0);

	//localizing module-variables
	//input from other modules
	double grav=vehicle[0].real();
	double area=vehicle[11].real();
	double caaim=vehicle[25].real();
	double cyaim=vehicle[26].real();
	double cnaim=vehicle[27].real();
	double pdynmc=vehicle[57].real();
	double thrust=vehicle[60].real();
	double mass=vehicle[61].real();

	//-------------------------------------------------------------------------
	//Force summation in body coordinates
	//
	// Aerodynamic forces: F_aero = q * A * C
	// where: q = dynamic pressure (Pa)
	//        A = reference area (m²)
	//        C = force coefficient (dimensionless)
	//
	// Propulsive force: F_thrust (N)
	//
	// Specific force: F_sp = F_total / mass (m/s²)

	//Axial specific force (X-direction, forward)
	FSPA[0] = (thrust - caaim * pdynmc * area) / mass;

	//Lateral specific force (Y-direction, right)
	FSPA[1] = (cyaim * pdynmc * area) / mass;

	//Normal specific force (Z-direction, down)
	//Note: negative sign because positive normal force is upward
	FSPA[2] = (-cnaim * pdynmc * area) / mass;

	//Diagnostics: accelerations in g's for monitoring
	aax = FSPA[0] / grav;  // Axial g's
	alx = FSPA[1] / grav;  // Lateral g's
	anx = -FSPA[2] / grav; // Normal g's (flip sign for upward positive)

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[14].gets_vec(FSPA);
	//diagnostics
	vehicle[150].gets(aax);
	vehicle[151].gets(alx);
	vehicle[152].gets(anx);
}
