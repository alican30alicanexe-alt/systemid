///////////////////////////////////////////////////////////////////////////////
// COMPONENT: gravity_constant
// CATEGORY: Environment
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Simple constant gravity model (9.81 m/s² downward)
//   Use for: Ballistics, low-altitude flight, simple simulations
//
// INPUTS (from vehicle array):
//   None
//
// OUTPUTS (to vehicle array):
//   ball[0] - grav - m/s^2 - Gravity acceleration magnitude
//
// PARAMETERS (from input.asc):
//   None (hardcoded to 9.81 m/s²)
//
// DEPENDENCIES:
//   - Requires: None
//   - Provides: Gravity acceleration for forces module
//
// REFERENCE:
//   Standard gravity at sea level
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
//     Define ball[0] for gravity in def_environment()
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of environment module-variables
//Member function of class 'Ball'
///////////////////////////////////////////////////////////////////////////////
void Ball::def_environment()
{
	//Definition of module-variables
	//output data
	ball[0].init("grav","D",0,"Gravity acceleration - m/s^2","environment","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Environment module
//Member function of class 'Ball'
//
//Provides constant gravity acceleration
//
//030424 Created by Peter Zipfel
//xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Ball::environment(double int_step)
{
	//localizing module-variables
	//output data
	double grav=ball[0].real();

	//Simple constant gravity
	grav = 9.81; // m/s^2 (standard gravity at sea level)

	//loading module-variables
	//output data
	ball[0].gets(grav);
}
