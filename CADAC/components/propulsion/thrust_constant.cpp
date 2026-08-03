///////////////////////////////////////////////////////////////////////////////
// COMPONENT: thrust_constant
// CATEGORY: Propulsion
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Constant thrust magnitude (no variation with time or altitude)
//   Simple propulsion model for initial trajectory studies
//   Use for: Simple rocket, constant acceleration, preliminary analysis
//
// INPUTS (from vehicle array):
//   None
//
// OUTPUTS (to vehicle array):
//   vehicle[80] - thrust - double - Thrust force N
//   vehicle[81] - mass - double - Vehicle mass kg
//
// PARAMETERS (from input.asc):
//   vehicle[82] - thrust_sl - double - Thrust magnitude N
//   vehicle[83] - mass0 - double - Initial mass kg
//
// DEPENDENCIES:
//   - Requires: None
//   - Provides: Thrust for forces module
//
// REFERENCE:
//   Simplified propulsion for preliminary analysis
//
// USAGE:
//   Include in class header:
//     virtual void def_propulsion();
//     virtual void init_propulsion();
//     virtual void propulsion(double int_step);
//
//   In input.asc MODULES section:
//     propulsion   def,init,exec
//
//   In input.asc DATA section:
//     thrust_sl = 5000  // Thrust, N
//     mass0 = 100       // Initial mass, kg
//
// NOTES:
//   - No altitude compensation (thrust is constant)
//   - No fuel depletion (mass is constant)
//   - For more realism, use thrust_table or rocket_motor components
//   - Consider adding mass depletion: mass = mass0 - mdot*time
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of propulsion module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_propulsion()
{
	//Definition of module-variables
	//input data
	vehicle[82].init("thrust_sl",0,"Thrust magnitude - N","propulsion","data","");
	vehicle[83].init("mass0",0,"Initial mass - kg","propulsion","data","");
	//output to other modules
	vehicle[80].init("thrust",0,"Thrust force - N","propulsion","out","scrn,plot");
	vehicle[81].init("mass",0,"Vehicle mass - kg","propulsion","out","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of propulsion module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_propulsion()
{
	//localizing module-variables
	//input data
	double mass0=vehicle[83].real();

	//Initialize mass
	vehicle[81].gets(mass0);
}

///////////////////////////////////////////////////////////////////////////////
//Propulsion module
//Member function of class 'Vehicle'
//
// Provides constant thrust (no altitude or time variation)
//
// 030424 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::propulsion(double int_step)
{
	//localizing module-variables
	//input data
	double thrust_sl=vehicle[82].real();
	double mass0=vehicle[83].real();

	//Simple constant thrust and mass
	double thrust = thrust_sl;
	double mass = mass0;

	//loading module-variables
	//output to other modules
	vehicle[80].gets(thrust);
	vehicle[81].gets(mass);
}
