///////////////////////////////////////////////////////////////////////////////
// COMPONENT: atmosphere_constant
// CATEGORY: Environment
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Constant atmospheric density (sea level conditions)
//   Use for: Space flight (rho=0), low altitude (rho=1.225), simple sims
//
// INPUTS (from vehicle array):
//   None
//
// OUTPUTS (to vehicle array):
//   vehicle[12] - rho - kg/m^3 - Air density
//
// PARAMETERS (from input.asc):
//   None (hardcoded to 1.225 kg/m³ at sea level)
//   Can be modified for vacuum (0.0) or custom density
//
// DEPENDENCIES:
//   - Requires: None
//   - Provides: Air density for aerodynamics module
//
// REFERENCE:
//   Standard sea level density from US Standard Atmosphere 1976
//
// USAGE:
//   Include in class header:
//     virtual void def_environment();
//     virtual void environment(double int_step);
//
//   In input.asc MODULES section:
//     environment   def,exec
//
//   Variable index allocation:
//     Define vehicle[12] for density in def_environment()
//
// NOTES:
//   - For vacuum/space, change rho = 0.0
//   - For custom altitude, lookup density from tables
//   - Consider atmosphere_us76 for altitude-varying density
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of environment module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_environment()
{
	//Definition of module-variables
	//output data
	vehicle[12].init("rho",0,"Air density - kg/m^3","out","","");
}

///////////////////////////////////////////////////////////////////////////////
//Environment module
//Member function of class 'Vehicle'
//
// Provides constant atmospheric density
//
// 030424 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::environment(double int_step)
{
	//localizing module-variables
	//output data
	double rho=vehicle[12].real();

	//Constant air density at sea level
	rho = 1.225; // kg/m^3 (standard sea level density)

	//loading module-variables
	//output data
	vehicle[12].gets(rho);
}
