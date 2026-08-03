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
//   ball[0]  - grav - double - Gravity magnitude m/s²
//   ball[11] - area - double - Reference area m²
//   ball[25] - caaim - double - Axial force coefficient
//   ball[26] - cyaim - double - Side force coefficient
//   ball[27] - cnaim - double - Normal force coefficient
//   ball[57] - pdynmc - double - Dynamic pressure Pa
//   ball[60] - thrust - double - Thrust force N
//   ball[61] - mass - double - Vehicle mass kg
//
// OUTPUTS (to vehicle array):
//   ball[14] - FSPA - Matrix(3x1) - Specific force (accel) in body m/s²
//   ball[150] - aax - double - Axial acceleration g's
//   ball[151] - alx - double - Lateral acceleration g's
//   ball[152] - anx - double - Normal acceleration g's
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
//Member function of class 'Ball'
///////////////////////////////////////////////////////////////////////////////
void Ball::def_forces()
{
	//Definition of module-variables
	//output to other modules
	ball[14].init("FSPA",0,0,0,"Specific force in body frame - m/s^2","forces","out","");
	//diagnostics
	ball[150].init("aax",0,"Axial acceleration - g's","forces","diag","");
	ball[151].init("alx",0,"Lateral acceleration - g's","forces","diag","scrn,plot");
	ball[152].init("anx",0,"Normal acceleration - g's","forces","diag","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Forces module
//Member function of class 'Ball'
//
// Sums aerodynamic, propulsive, and gravitational forces
// Computes specific force (force per unit mass = acceleration)
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Ball::forces(double int_step)
{
	//local module-variables
	Matrix FSPA(3,1);
	double aax(0);
	double alx(0);
	double anx(0);

	//localizing module-variables
	//input from other modules
	double grav=ball[0].real();
	double area=ball[11].real();
	double caaim=ball[25].real();
	double cyaim=ball[26].real();
	double cnaim=ball[27].real();
	double pdynmc=ball[57].real();
	double thrust=ball[60].real();
	double mass=ball[61].real();

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
	ball[14].gets_vec(FSPA);
	//diagnostics
	ball[150].gets(aax);
	ball[151].gets(alx);
	ball[152].gets(anx);
}
