///////////////////////////////////////////////////////////////////////////////
// COMPONENT: drag_simple
// CATEGORY: Aerodynamics
// DoF: 3DoF
//
// DESCRIPTION:
//   Simple point mass drag force calculation
//   Uses constant drag coefficient and reference area
//   F_drag = 0.5 * rho * v^2 * Cd * A
//   Use for: Ballistics, simple projectiles, cannonballs
//
// INPUTS (from vehicle array):
//   vehicle[0]  - grav - double - Gravity acceleration m/s^2
//   vehicle[12] - rho - double - Air density kg/m^3
//   vehicle[21] - VBEL - Matrix(3x1) - Velocity in Earth frame m/s
//
// OUTPUTS (to vehicle array):
//   vehicle[13] - dvbe - double - Speed m/s
//   vehicle[14] - FSPB - Matrix(3x1) - Specific force m/s^2
//
// PARAMETERS (from input.asc):
//   vehicle[10] - cd - double - Drag coefficient (dimensionless)
//   vehicle[11] - area - double - Reference area m^2
//
// DEPENDENCIES:
//   - Requires: Environment module for gravity (vehicle[0]) and density (vehicle[12])
//   - Requires: Kinematics module for velocity (vehicle[21])
//   - Provides: Specific force for dynamics/kinematics integration
//
// REFERENCE:
//   Standard drag equation for incompressible flow
//   Valid for subsonic, low-altitude flight
//
// USAGE:
//   Include in class header:
//     virtual void def_forces();
//     virtual void forces(double int_step);
//
//   In input.asc MODULES section:
//     forces   def,exec
//
//   In input.asc DATA section:
//     cd = 0.47      // Drag coefficient (sphere = 0.47)
//     area = 0.0314  // Reference area (10cm radius sphere), m^2
//
// NOTES:
//   - Combines drag and gravity into specific force (F/m = a)
//   - Drag opposes velocity direction
//   - Gravity acts downward (positive Z in Earth frame)
//   - No lift force (point mass assumption)
//   - Handles zero-velocity case to avoid division by zero
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of forces module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_forces()
{
	//Definition of module-variables
	//input data
	vehicle[10].init("cd",0,"Drag coefficient - ND","data","","");
	vehicle[11].init("area",0,"Reference area - m^2","data","","");
	//diagnostic output
	vehicle[13].init("dvbe",0,"Speed - m/s","out","scrn","plot");
	//output to other modules
	vehicle[14].init("FSPB",0,0,0,"Specific force in body frame - m/s^2","out","","");
}

///////////////////////////////////////////////////////////////////////////////
//Forces module
//Calculates drag force for point mass
//Member function of class 'Vehicle'
//
// Computes aerodynamic drag opposing velocity
// Combines with gravity to produce total specific force
//
// 030424 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::forces(double int_step)
{
	//local module-variables
	Matrix FSPB(3,1);

	//localizing module-variables
	//input data
	double cd=vehicle[10].real();
	double area=vehicle[11].real();
	//input from other modules
	double grav=vehicle[0].real();
	double rho=vehicle[12].real();
	Matrix VBEL=vehicle[21].vec();

	//-------------------------------------------------------------------------
	//Speed
	double dvbe = VBEL.absolute();

	//Drag force per unit mass (specific force)
	//F_drag = 0.5 * rho * v^2 * cd * area
	//a_drag = F_drag / mass
	//Since we track specific force, and drag opposes velocity:
	double drag_mag = 0.5 * rho * dvbe * dvbe * cd * area;

	//Unit vector in velocity direction
	Matrix VBEL_unit(3,1);
	if(dvbe > 0.1) {
		VBEL_unit = VBEL * (1.0/dvbe); //unit vector in velocity direction
	} else {
		VBEL_unit.zero();
	}

	//Specific force (includes both gravity and drag)
	//Note: In Earth frame, gravity is downward (negative Z)
	//Drag opposes velocity
	FSPB[0] = -drag_mag * VBEL_unit[0]; // Drag in X
	FSPB[1] = -drag_mag * VBEL_unit[1]; // Drag in Y
	FSPB[2] = -drag_mag * VBEL_unit[2] - grav; // Drag in Z + gravity (down)

	//loading module-variables
	//output to other modules
	vehicle[13].gets(dvbe);
	vehicle[14].gets_vec(FSPB);
}
