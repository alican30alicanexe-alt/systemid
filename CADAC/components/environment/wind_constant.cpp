///////////////////////////////////////////////////////////////////////////////
// COMPONENT: wind_constant
// CATEGORY: Environment
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Constant horizontal wind from specified direction
//   Optional vertical wind component
//   Use for: Wind effects on trajectory, crosswind landings
//
// INPUTS (from vehicle array):
//   vehicle[21] - VBEL - Matrix(3x1) - Velocity in Earth frame m/s
//
// OUTPUTS (to vehicle array):
//   vehicle[74] - VAEL - Matrix(3x1) - Wind velocity in local coords m/s
//   vehicle[75] - dvba - double - Speed relative to air m/s
//   vehicle[76] - VBAL - Matrix(3x1) - Velocity relative to air m/s
//
// PARAMETERS (from input.asc):
//   vehicle[64] - dvae - double - Wind speed m/s
//   vehicle[70] - psiwdx - double - Wind direction from north deg
//   vehicle[69] - vaed3 - double - Vertical wind (positive down) m/s
//
// DEPENDENCIES:
//   - Requires: Kinematics for VBEL
//   - Provides: Wind velocity and airspeed for aerodynamics
//
// REFERENCE:
//   Standard wind model in local level coordinates
//   Wind direction: 0° = from north, 90° = from east
//
// USAGE:
//   Include in class header:
//     virtual void def_environment();
//     virtual void environment(double int_step);
//
//   In input.asc MODULES section:
//     environment   def,exec
//
//   In input.asc DATA section:
//     dvae = 10.0      // Wind speed, m/s
//     psiwdx = 90.0    // Wind from east, deg
//     vaed3 = 0.0      // No vertical wind, m/s
//
// NOTES:
//   - Wind direction is meteorological convention (from north)
//   - Local level frame: X=East, Y=North, Z=Down
//   - Positive vaed3 is downward wind
//   - For no wind, set dvae=0
//   - For wind shear or turbulence, use wind_table or wind_turbulence
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of environment module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_environment()
{
	//Definition of module-variables
	//input data
	vehicle[64].init("dvae",0,"Wind speed - m/s","environment","data","");
	vehicle[69].init("vaed3",0,"Vertical wind (down positive) - m/s","environment","data","");
	vehicle[70].init("psiwdx",0,"Wind direction from north - deg","environment","data","");
	//output to other modules
	vehicle[74].init("VAEL",0,0,0,"Wind velocity in local coords - m/s","environment","out","");
	vehicle[75].init("dvba",0,"Speed relative to air - m/s","environment","out","");
	vehicle[76].init("VBAL",0,0,0,"Velocity relative to air - m/s","environment","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Environment module
//Member function of class 'Vehicle'
//
// Constant wind model
// Computes airspeed and velocity relative to airmass
//
// 011127 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::environment(double int_step)
{
	//local module-variables
	double dvba(0);
	Matrix VAEL(3,1);
	Matrix VBAL(3,1);

	//localizing module-variables
	//input data
	double dvae=vehicle[64].real();
	double psiwdx=vehicle[70].real();
	double vaed3=vehicle[69].real();
	//input from other modules
	Matrix VBEL=vehicle[21].vec();

	//-------------------------------------------------------------------------
	//Wind components in local level coordinates
	//Local frame: X=East, Y=North, Z=Down
	//Wind direction is FROM north (meteorological convention)
	//psiwdx = 0° means wind from north (blowing toward south)
	//psiwdx = 90° means wind from east (blowing toward west)

	VAEL[0] = -dvae * cos(psiwdx * RAD);  // East component
	VAEL[1] = -dvae * sin(psiwdx * RAD);  // North component
	VAEL[2] = vaed3;                       // Down component

	//Velocity relative to airmass
	//VBAL = vehicle velocity - wind velocity
	VBAL = VBEL - VAEL;

	//Airspeed magnitude
	dvba = VBAL.absolute();

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[74].gets_vec(VAEL);
	vehicle[75].gets(dvba);
	vehicle[76].gets_vec(VBAL);
}
