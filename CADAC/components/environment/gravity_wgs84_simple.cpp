///////////////////////////////////////////////////////////////////////////////
// COMPONENT: gravity_wgs84_simple
// CATEGORY: Environment
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Gravity model based on altitude above Earth surface
//   Simple inverse-square law: g = g0 * (Re / (Re + h))^2
//   Use for: High-altitude flight, orbital mechanics, space launch
//
// INPUTS (from vehicle array):
//   vehicle[22] - altitude - double - Altitude above sea level m
//
// OUTPUTS (to vehicle array):
//   vehicle[0] - grav - double - Gravity magnitude m/s^2
//
// PARAMETERS (from input.asc):
//   None (uses WGS84 constants)
//
// DEPENDENCIES:
//   - Requires: Kinematics module for altitude
//   - Provides: Gravity for dynamics
//
// REFERENCE:
//   WGS84 Earth model
//   g = g0 * (Re / (Re + h))^2
//   where: g0 = 9.80665 m/s² (standard gravity at sea level)
//          Re = 6371000 m (mean Earth radius)
//          h = altitude above sea level
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
//   - More accurate than constant gravity for high altitude
//   - Simple spherical Earth approximation
//   - For full WGS84 ellipsoid with J2, use gravity_wgs84_full component
//   - Negligible difference from constant gravity below 10 km
//   - Important for altitudes > 50 km
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
	vehicle[0].init("grav",0,"Gravity acceleration - m/s^2","environment","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Environment module
//Member function of class 'Vehicle'
//
// Gravity varies with altitude using inverse-square law
//
// 130523 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::environment(double int_step)
{
	//localizing module-variables
	//input from other modules
	double altitude=vehicle[22].real();

	//-------------------------------------------------------------------------
	//Gravity calculation with altitude variation
	//WGS84 constants
	const double EARTH_MASS = 5.973332e24;  // kg
	const double REARTH = 6371000;          // m (mean radius)
	const double G = 6.673e-11;             // gravitational constant

	//Distance from Earth center
	double rad = REARTH + altitude;

	//Inverse-square law
	double grav = G * EARTH_MASS / (rad * rad);

	//-------------------------------------------------------------------------
	//loading module-variables
	//output data
	vehicle[0].gets(grav);
}
