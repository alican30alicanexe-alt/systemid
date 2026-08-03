///////////////////////////////////////////////////////////////////////////////
// COMPONENT: gps_perfect
// CATEGORY: Sensors
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Perfect GPS sensor (no errors)
//   Provides exact position and velocity
//   Use for: Initial navigation development, performance bounds
//
// INPUTS (from vehicle array):
//   vehicle[20] - SBEL - Matrix(3x1) - True position m
//   vehicle[21] - VBEL - Matrix(3x1) - True velocity m/s
//   vehicle[22] - altitude - double - True altitude m
//
// OUTPUTS (to vehicle array):
//   vehicle[180] - SBELG - Matrix(3x1) - GPS position m
//   vehicle[181] - VBELG - Matrix(3x1) - GPS velocity m/s
//   vehicle[182] - altg - double - GPS altitude m
//
// PARAMETERS (from input.asc):
//   None (perfect sensor, no errors)
//
// DEPENDENCIES:
//   - Requires: Kinematics for true state
//   - Provides: Position/velocity for navigation
//
// REFERENCE:
//   Ideal GPS with perfect measurements
//
// USAGE:
//   Include in class header:
//     virtual void def_gps();
//     virtual void gps(double int_step);
//
//   In input.asc MODULES section:
//     gps   def,exec
//
// NOTES:
//   - This is an idealized sensor for testing
//   - Real GPS has: noise, lag, dropouts, multipath
//   - For realistic GPS, use gps_noise component
//   - Update rate is every timestep (unrealistic)
//   - No satellite visibility modeling
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of GPS module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_gps()
{
	//Definition of module-variables
	//output to other modules
	vehicle[180].init("SBELG",0,0,0,"GPS position - m","gps","out","");
	vehicle[181].init("VBELG",0,0,0,"GPS velocity - m/s","gps","out","");
	vehicle[182].init("altg",0,"GPS altitude - m","gps","out","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//GPS module
//Member function of class 'Vehicle'
//
// Perfect GPS measurements (truth data)
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::gps(double int_step)
{
	//local module-variables
	Matrix SBELG(3,1);
	Matrix VBELG(3,1);
	double altg(0);

	//localizing module-variables
	//input from other modules (truth)
	Matrix SBEL=vehicle[20].vec();
	Matrix VBEL=vehicle[21].vec();
	double altitude=vehicle[22].real();

	//-------------------------------------------------------------------------
	//Perfect GPS (no errors)
	SBELG = SBEL;
	VBELG = VBEL;
	altg = altitude;

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[180].gets_vec(SBELG);
	vehicle[181].gets_vec(VBELG);
	vehicle[182].gets(altg);
}
