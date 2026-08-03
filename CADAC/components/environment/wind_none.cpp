///////////////////////////////////////////////////////////////////////////////
// COMPONENT: wind_none
// CATEGORY: Environment
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   No wind (calm air conditions)
//   Airspeed equals ground speed
//   Use for: Indoor, vacuum, or calm-air simulations
//
// INPUTS (from vehicle array):
//   vehicle[21] - VBEL - Matrix(3x1) - Velocity in Earth frame m/s
//
// OUTPUTS (to vehicle array):
//   vehicle[74] - VAEL - Matrix(3x1) - Wind velocity (zero) m/s
//   vehicle[75] - dvba - double - Speed relative to air m/s
//   vehicle[76] - VBAL - Matrix(3x1) - Velocity relative to air m/s
//
// PARAMETERS (from input.asc):
//   None
//
// DEPENDENCIES:
//   - Requires: Kinematics for VBEL
//   - Provides: Airspeed for aerodynamics (equals ground speed)
//
// REFERENCE:
//   Calm air assumption: VBAL = VBEL
//
// USAGE:
//   Include in class header:
//     virtual void def_environment();
//     virtual void environment(double int_step);
//
//   In input.asc MODULES section:
//     environment   def,exec
//
// NOTES:
//   - Wind velocity is zero
//   - Airspeed equals ground speed
//   - Simplest wind model
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of environment module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_environment()
{
	//Definition of module-variables
	//output to other modules
	vehicle[74].init("VAEL",0,0,0,"Wind velocity (zero) - m/s","environment","out","");
	vehicle[75].init("dvba",0,"Speed relative to air - m/s","environment","out","");
	vehicle[76].init("VBAL",0,0,0,"Velocity relative to air - m/s","environment","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Environment module
//Member function of class 'Vehicle'
//
// No wind (calm air)
// Airspeed equals ground speed
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::environment(double int_step)
{
	//local module-variables
	double dvba(0);
	Matrix VAEL(3,1);
	Matrix VBAL(3,1);

	//localizing module-variables
	//input from other modules
	Matrix VBEL=vehicle[21].vec();

	//-------------------------------------------------------------------------
	//No wind
	VAEL.zero();  // Wind velocity = 0

	//Velocity relative to air = velocity relative to Earth
	VBAL = VBEL;

	//Airspeed = ground speed
	dvba = VBEL.absolute();

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[74].gets_vec(VAEL);
	vehicle[75].gets(dvba);
	vehicle[76].gets_vec(VBAL);
}
