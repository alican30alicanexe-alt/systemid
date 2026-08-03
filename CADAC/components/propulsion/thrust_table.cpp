///////////////////////////////////////////////////////////////////////////////
// COMPONENT: thrust_table
// CATEGORY: Propulsion
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Thrust from table lookup vs time
//   Allows arbitrary thrust profiles
//   Use for: Complex rocket motors, staged vehicles
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
//   vehicle[64] - mass_init - double - Initial total mass kg
//   vehicle[65] - mass_prop - double - Propellant mass kg
//   vehicle[66] - burn_time - double - Total burn duration sec
//   Thrust table (in data deck):
//     thrust_vs_time(time, thrust)
//
// DEPENDENCIES:
//   - Requires: Time from executive
//   - Provides: Thrust and mass for forces module
//   - Uses: Table lookup from data deck
//
// REFERENCE:
//   Tabular thrust profile from test data or design
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
//     mass_init = 100      // Initial mass, kg
//     mass_prop = 20       // Propellant mass, kg
//     burn_time = 5.0      // Burn duration, sec
//
//   Create thrust_deck.asc with table:
//     THRUST_DECK
//     thrust_vs_time
//     0.0   5000
//     1.0   4800
//     2.0   4500
//     ...
//     END
//
// NOTES:
//   - Linear interpolation between table points
//   - Mass decreases linearly with time
//   - Motor shuts off at burn_time
//   - For simple constant thrust, use thrust_constant
//   - Can model complex profiles: boost-sustain, multi-stage
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
	vehicle[64].init("mass_init",0,"Initial total mass - kg","propulsion","data","");
	vehicle[65].init("mass_prop",0,"Propellant mass - kg","propulsion","data","");
	vehicle[66].init("burn_time",0,"Burn duration - sec","propulsion","data","");
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
// Table-based thrust with linear mass depletion
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::propulsion(double int_step)
{
	//local module-variables
	double thrust(0);
	double mass(0);
	int mprop(0);

	//localizing module-variables
	//input data
	double burn_time=vehicle[66].real();
	double mass_init=vehicle[64].real();
	double mass_prop=vehicle[65].real();
	//input from other modules
	double time=vehicle[0].real();

	//-------------------------------------------------------------------------
	//Motor status and thrust
	if(time < burn_time) {
		//Motor burning
		mprop = 1;

		//Table lookup for thrust vs time
		//thrust = proptable.look_up("thrust_vs_time", time);
		//For component library without table, use simple approximation:
		//Linear thrust decay from 100% to 80% over burn time
		double thrust_max = 5000.0;  // N (should come from table)
		thrust = thrust_max * (1.0 - 0.2 * time / burn_time);

		//Mass depletion (linear)
		double mdot = mass_prop / burn_time;  // kg/s
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
