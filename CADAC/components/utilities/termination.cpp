///////////////////////////////////////////////////////////////////////////////
// COMPONENT: termination
// CATEGORY: Utilities
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Simulation termination conditions
//   Stops simulation when conditions are met
//   Use for: All simulations
//
// INPUTS (from vehicle array):
//   vehicle[0]  - time - double - Simulation time sec
//   vehicle[22] - altitude - double - Altitude m
//   vehicle[80] - dta - double - Range to target m (optional)
//   vehicle[162] - intercept_flag - int - Intercept detected (optional)
//
// OUTPUTS (to vehicle array):
//   vehicle[5] - stop - int - Stop flag (1=stop)
//   vehicle[6] - lconv - int - Convergence flag
//
// PARAMETERS (from input.asc):
//   vehicle[7] - endtime - double - Maximum sim time sec
//   vehicle[8] - min_alt - double - Minimum altitude m
//   vehicle[9] - min_range - double - Minimum target range m
//
// DEPENDENCIES:
//   - Requires: Various modules for termination criteria
//   - Provides: Stop signal to executive
//
// REFERENCE:
//   Standard termination logic for CADAC simulations
//
// USAGE:
//   Include in class header:
//     virtual void def_termination();
//     virtual void termination(double int_step);
//
//   In input.asc MODULES section:
//     termination   def,exec
//
//   In input.asc DATA section:
//     endtime = 100     // Max simulation time, sec
//     min_alt = -10     // Stop if altitude < -10m (ground)
//     min_range = 10    // Stop if range < 10m (intercept)
//
// NOTES:
//   - Multiple termination criteria (OR logic)
//   - Ground impact: altitude <= min_alt
//   - Time limit: time >= endtime
//   - Intercept: range <= min_range
//   - lconv flag indicates reason for stop
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of termination module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_termination()
{
	//Definition of module-variables
	//input data
	vehicle[7].init("endtime",0,"Maximum simulation time - sec","termination","data","");
	vehicle[8].init("min_alt",0,"Minimum altitude - m","termination","data","");
	vehicle[9].init("min_range",0,"Minimum target range - m","termination","data","");
	//output to executive
	vehicle[5].init("stop","int",0,"Stop flag (1=stop) - ND","termination","exec","");
	vehicle[6].init("lconv","int",0,"Convergence flag - ND","termination","exec","");
}

///////////////////////////////////////////////////////////////////////////////
//Termination module
//Member function of class 'Vehicle'
//
// Checks termination conditions
// Sets stop flag when any condition is met
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::termination(double int_step)
{
	//local module-variables
	int stop(0);
	int lconv(0);

	//localizing module-variables
	//input data
	double endtime=vehicle[7].real();
	double min_alt=vehicle[8].real();
	double min_range=vehicle[9].real();
	//input from other modules
	double time=vehicle[0].real();
	double altitude=vehicle[22].real();
	double dta=vehicle[80].real();
	int intercept_flag=vehicle[162].integer();

	//-------------------------------------------------------------------------
	//Termination criteria (OR logic - any condition triggers stop)

	//1. Time limit exceeded
	if(time >= endtime) {
		stop = 1;
		lconv = 1;  // Normal termination
	}

	//2. Ground impact (altitude below minimum)
	if(altitude <= min_alt) {
		stop = 1;
		lconv = 2;  // Ground impact
	}

	//3. Target proximity (range below minimum)
	if(dta <= min_range && dta > 0) {
		stop = 1;
		lconv = 3;  // Intercept
	}

	//4. Intercept detected (from intercept module)
	if(intercept_flag == 1) {
		stop = 1;
		lconv = 4;  // Confirmed intercept
	}

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to executive
	vehicle[5].gets_int(stop);
	vehicle[6].gets_int(lconv);
}
