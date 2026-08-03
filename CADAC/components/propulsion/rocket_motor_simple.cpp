///////////////////////////////////////////////////////////////////////////////
// COMPONENT: rocket_motor_simple
// CATEGORY: Propulsion
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Simple rocket motor with constant thrust and burn time
//   Mass depletion during burn, constant after burnout
//   Use for: Solid rocket motors, boosters, simple missiles
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//
// OUTPUTS (to vehicle array):
//   vehicle[50] - mprop - int - Motor status (0=off, 1=on)
//   vehicle[60] - thrust - double - Thrust force N
//   vehicle[61] - mass - double - Vehicle mass kg
//
// PARAMETERS (from input.asc):
//   vehicle[62] - thrust_SL - double - Sea level thrust N
//   vehicle[63] - burn_time - double - Motor burn duration sec
//   vehicle[64] - mass_init - double - Initial total mass kg
//   vehicle[65] - mass_prop - double - Propellant mass kg
//
// DEPENDENCIES:
//   - Requires: None (uses time from simulation)
//   - Provides: Thrust and mass for forces module
//
// REFERENCE:
//   Simple constant-thrust rocket motor
//   Linear mass depletion: m(t) = m0 - mdot*t
//   where mdot = mprop / tburn
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
//     thrust_SL = 5000    // Thrust, N
//     burn_time = 3.5     // Burn duration, sec
//     mass_init = 100     // Total initial mass, kg
//     mass_prop = 20      // Propellant mass, kg
//
// NOTES:
//   - Thrust is constant during burn (no altitude compensation)
//   - Mass decreases linearly during burn
//   - Motor shuts off at burn_time
//   - For altitude-varying thrust, use rocket_motor_altitude component
//   - For table-based thrust profile, use rocket_motor_table component
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
	vehicle[62].init("thrust_SL",0,"Sea level thrust - N","propulsion","data","");
	vehicle[63].init("burn_time",0,"Motor burn duration - sec","propulsion","data","");
	vehicle[64].init("mass_init",0,"Initial total mass - kg","propulsion","data","");
	vehicle[65].init("mass_prop",0,"Propellant mass - kg","propulsion","data","");
	//output to other modules
	vehicle[50].init("mprop","int",0,"Motor status (0=off, 1=on) - ND","propulsion","diag","scrn");
	vehicle[60].init("thrust",0,"Thrust force - N","propulsion","out","scrn,plot");
	vehicle[61].init("mass",0,"Vehicle mass - kg","propulsion","out","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of propulsion module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_propulsion()
{
	//localizing module-variables
	//input data
	double mass_init=vehicle[64].real();

	//Initialize mass and motor status
	vehicle[61].gets(mass_init);
	vehicle[50].gets_int(1);  // Motor starts ON
}

///////////////////////////////////////////////////////////////////////////////
//Propulsion module
//Member function of class 'Vehicle'
//
// Simple rocket motor with constant thrust and linear mass depletion
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::propulsion(double int_step)
{
	//local module-variables
	double thrust(0);
	double mass(0);
	int mprop(0);

	//localizing module-variables
	//input data
	double thrust_SL=vehicle[62].real();
	double burn_time=vehicle[63].real();
	double mass_init=vehicle[64].real();
	double mass_prop=vehicle[65].real();
	//input from other modules
	double time=vehicle[0].real();

	//-------------------------------------------------------------------------
	//Motor status and thrust
	if(time < burn_time) {
		//Motor burning
		mprop = 1;
		thrust = thrust_SL;

		//Mass depletion (linear)
		double mdot = mass_prop / burn_time;  // Mass flow rate kg/s
		mass = mass_init - mdot * time;

	} else {
		//Motor burned out
		mprop = 0;
		thrust = 0;

		//Mass is constant after burnout
		double mass_burnout = mass_init - mass_prop;
		mass = mass_burnout;
	}

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[50].gets_int(mprop);
	vehicle[60].gets(thrust);
	vehicle[61].gets(mass);
}
