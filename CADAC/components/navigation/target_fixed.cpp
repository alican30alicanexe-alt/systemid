///////////////////////////////////////////////////////////////////////////////
// COMPONENT: target_fixed
// CATEGORY: Navigation
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Stationary target at fixed coordinates
//   Use for: Ground targets, ships, buildings
//
// INPUTS (from vehicle array):
//   None
//
// OUTPUTS (to vehicle array):
//   vehicle[200] - STEL - Matrix(3x1) - Target position in local frame m
//   vehicle[201] - VTEL - Matrix(3x1) - Target velocity (zero) m/s
//
// PARAMETERS (from input.asc):
//   vehicle[202] - stel1 - double - Target X position (east) m
//   vehicle[203] - stel2 - double - Target Y position (north) m
//   vehicle[204] - stel3 - double - Target Z position (down) m
//
// DEPENDENCIES:
//   - Requires: None
//   - Provides: Target state for seeker and guidance
//
// REFERENCE:
//   Fixed target at specified coordinates
//
// USAGE:
//   Include in class header:
//     virtual void def_target();
//     virtual void init_target();
//     virtual void target(double int_step);
//
//   In input.asc MODULES section:
//     target   def,init,exec
//
//   In input.asc DATA section:
//     stel1 = 5000    // Target 5km east
//     stel2 = 10000   // Target 10km north
//     stel3 = 0       // Target at ground level
//
// NOTES:
//   - Local level frame: X=East, Y=North, Z=Down
//   - Target velocity is zero (stationary)
//   - For moving targets, use target_constant_velocity component
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of target module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_target()
{
	//Definition of module-variables
	//input data
	vehicle[202].init("stel1",0,"Target X position (east) - m","target","data","");
	vehicle[203].init("stel2",0,"Target Y position (north) - m","target","data","");
	vehicle[204].init("stel3",0,"Target Z position (down) - m","target","data","");
	//output to other modules
	vehicle[200].init("STEL",0,0,0,"Target position in local frame - m","target","out","");
	vehicle[201].init("VTEL",0,0,0,"Target velocity in local frame - m/s","target","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of target module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_target()
{
	//local module-variables
	Matrix STEL(3,1);
	Matrix VTEL(3,1);

	//input data
	double stel1=vehicle[202].real();
	double stel2=vehicle[203].real();
	double stel3=vehicle[204].real();

	//-------------------------------------------------------------------------
	//Build target position vector
	STEL.build_vec3(stel1, stel2, stel3);

	//Target velocity is zero (stationary)
	VTEL.zero();

	//-------------------------------------------------------------------------
	//loading module-variables
	//output
	vehicle[200].gets_vec(STEL);
	vehicle[201].gets_vec(VTEL);
}

///////////////////////////////////////////////////////////////////////////////
//Target module
//Member function of class 'Vehicle'
//
// Fixed target (no motion)
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::target(double int_step)
{
	//Target position and velocity are constant
	//No calculations needed each time step
	//State is set in init_target() and never changes
}
