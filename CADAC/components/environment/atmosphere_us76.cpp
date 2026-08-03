///////////////////////////////////////////////////////////////////////////////
// COMPONENT: atmosphere_us76
// CATEGORY: Environment
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   US Standard Atmosphere 1976 model
//   Temperature, pressure, density vs altitude (0-86 km)
//   Use for: Realistic simulations, missiles, aircraft
//
// INPUTS (from vehicle array):
//   vehicle[22] - altitude - double - Altitude above sea level m
//   vehicle[75] - dvba - double - Speed relative to air m/s (for Mach, pdynmc)
//
// OUTPUTS (to vehicle array):
//   vehicle[52] - press - double - Atmospheric pressure Pa
//   vehicle[53] - rho - double - Air density kg/m³
//   vehicle[54] - vsound - double - Speed of sound m/s
//   vehicle[56] - vmach - double - Mach number (dimensionless)
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[58] - tempk - double - Temperature K
//
// PARAMETERS (from input.asc):
//   None
//
// DEPENDENCIES:
//   - Requires: Kinematics for altitude
//   - Requires: Wind module for dvba (or use dvbe if no wind)
//   - Provides: Atmosphere properties for aerodynamics
//   - Uses: atmosphere76() utility function from CADAC core
//
// REFERENCE:
//   US Standard Atmosphere, 1976
//   NOAA-S/T 76-1562, NASA-TM-X-74335
//   Valid from sea level to 86 km
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
//   - Uses CADAC utility function atmosphere76(rho, press, tempk, alt)
//   - Returns sea-level values if altitude < 0
//   - Extrapolates linearly above 86 km
//   - Assumes ideal gas law
//   - Standard gravity (9.80665 m/s²) used throughout
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
	vehicle[52].init("press",0,"Atmospheric pressure - Pa","environment","out","");
	vehicle[53].init("rho",0,"Atmospheric density - kg/m^3","environment","out","");
	vehicle[54].init("vsound",0,"Speed of sound - m/s","environment","diag","");
	vehicle[56].init("vmach",0,"Mach number - ND","environment","out","scrn,plot,com");
	vehicle[57].init("pdynmc",0,"Dynamic pressure - Pa","environment","out","scrn,plot");
	vehicle[58].init("tempk",0,"Atmospheric temperature - K","environment","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Environment module
//Member function of class 'Vehicle'
//
// US 1976 Standard Atmosphere
// Calculates atmospheric properties and flight conditions
//
// 030319 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::environment(double int_step)
{
	//local module-variables
	double press(0);
	double rho(0);
	double vsound(0);
	double vmach(0);
	double pdynmc(0);
	double tempk(0);

	//localizing module-variables
	//input from other modules
	double altitude=vehicle[22].real();
	double dvba=vehicle[75].real();  // Speed relative to air

	//-------------------------------------------------------------------------
	//US 1976 Standard Atmosphere
	//This calls the CADAC utility function (in utility_functions.cpp)
	atmosphere76(rho, press, tempk, altitude);

	//Speed of sound (assuming ideal gas)
	//gamma = 1.4 for air
	//R = 287.053 J/(kg·K) specific gas constant for dry air
	const double R = 287.053;
	vsound = sqrt(1.4 * R * tempk);

	//Mach number
	if(vsound > 0.1) {
		vmach = fabs(dvba / vsound);
	} else {
		vmach = 0;
	}

	//Dynamic pressure: q = 0.5 * rho * v^2
	pdynmc = 0.5 * rho * dvba * dvba;

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[52].gets(press);
	vehicle[53].gets(rho);
	vehicle[54].gets(vsound);
	vehicle[56].gets(vmach);
	vehicle[57].gets(pdynmc);
	vehicle[58].gets(tempk);
}
