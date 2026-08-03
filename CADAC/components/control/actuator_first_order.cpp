///////////////////////////////////////////////////////////////////////////////
// COMPONENT: actuator_first_order
// CATEGORY: Control
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Simple first-order actuator lag with position limits
//   Models control surface or TVC dynamics
//   Use for: Missile fins, aircraft control surfaces, TVC nozzles
//
// INPUTS (from vehicle array):
//   vehicle[519] - delacx - double - Commanded aileron deflection deg
//   vehicle[520] - delecx - double - Commanded elevator deflection deg
//   vehicle[521] - delrcx - double - Commanded rudder deflection deg
//
// OUTPUTS (to vehicle array):
//   vehicle[619] - delax - double - Actual aileron deflection deg
//   vehicle[620] - delex - double - Actual elevator deflection deg
//   vehicle[621] - delrx - double - Actual rudder deflection deg
//
// PARAMETERS (from input.asc):
//   vehicle[602] - dlimx - double - Position limit deg
//   vehicle[606] - tauact - double - Time constant sec
//
// DEPENDENCIES:
//   - Requires: Control for commanded deflections
//   - Provides: Actual deflections for aerodynamics
//
// REFERENCE:
//   First-order lag: tau * ddel/dt + del = del_cmd
//   Transfer function: G(s) = 1 / (tau*s + 1)
//
// USAGE:
//   Include in class header:
//     virtual void def_actuator();
//     virtual void init_actuator();
//     virtual void actuator(double int_step);
//
//   In input.asc MODULES section:
//     actuator   def,init,exec
//
//   In input.asc DATA section:
//     dlimx = 20.0      // Position limit, deg
//     tauact = 0.05     // Time constant, sec (0.05 = 50ms)
//
// NOTES:
//   - Separate first-order lag for each axis (roll, pitch, yaw)
//   - Position limits prevent over-deflection
//   - No rate limits (for rate limits, use actuator_second_order)
//   - Typical time constants: 0.02-0.1 sec for missiles
//   - Smaller tau = faster response
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of actuator module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_actuator()
{
	//Definition of module-variables
	//input data
	vehicle[602].init("dlimx",0,"Position limit - deg","actuator","data","");
	vehicle[606].init("tauact",0,"Actuator time constant - sec","actuator","data","");
	//state variables
	vehicle[615].init("dela",0,"Aileron position - deg","actuator","state","");
	vehicle[616].init("dele",0,"Elevator position - deg","actuator","state","");
	vehicle[617].init("delr",0,"Rudder position - deg","actuator","state","");
	//output to other modules
	vehicle[619].init("delax",0,"Aileron deflection - deg","actuator","out","scrn,plot");
	vehicle[620].init("delex",0,"Elevator deflection - deg","actuator","out","scrn,plot");
	vehicle[621].init("delrx",0,"Rudder deflection - deg","actuator","out","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of actuator module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_actuator()
{
	//Initialize actuator positions to zero (neutral)
	vehicle[615].gets(0.0);  // dela
	vehicle[616].gets(0.0);  // dele
	vehicle[617].gets(0.0);  // delr
	vehicle[619].gets(0.0);  // delax
	vehicle[620].gets(0.0);  // delex
	vehicle[621].gets(0.0);  // delrx
}

///////////////////////////////////////////////////////////////////////////////
//Actuator module
//Member function of class 'Vehicle'
//
// First-order actuator dynamics with position limits
//
// 030515 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::actuator(double int_step)
{
	//local module-variables
	double delax(0);
	double delex(0);
	double delrx(0);

	//localizing module-variables
	//input data
	double dlimx=vehicle[602].real();
	double tauact=vehicle[606].real();
	//input from other modules
	double delacx=vehicle[519].real();  // Commanded
	double delecx=vehicle[520].real();
	double delrcx=vehicle[521].real();
	//state variables
	double dela=vehicle[615].real();
	double dele=vehicle[616].real();
	double delr=vehicle[617].real();

	//-------------------------------------------------------------------------
	//First-order lag dynamics: tau * ddel/dt + del = del_cmd
	//Rearranged: ddel/dt = (del_cmd - del) / tau

	//Aileron channel
	double delad_new = (delacx - dela) / tauact;
	dela = integrate(delad_new, 0, dela, int_step);
	//Position limit
	if(dela > dlimx) dela = dlimx;
	if(dela < -dlimx) dela = -dlimx;
	delax = dela;

	//Elevator channel
	double deled_new = (delecx - dele) / tauact;
	dele = integrate(deled_new, 0, dele, int_step);
	//Position limit
	if(dele > dlimx) dele = dlimx;
	if(dele < -dlimx) dele = -dlimx;
	delex = dele;

	//Rudder channel
	double delrd_new = (delrcx - delr) / tauact;
	delr = integrate(delrd_new, 0, delr, int_step);
	//Position limit
	if(delr > dlimx) delr = dlimx;
	if(delr < -dlimx) delr = -dlimx;
	delrx = delr;

	//-------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[615].gets(dela);
	vehicle[616].gets(dele);
	vehicle[617].gets(delr);
	//output to other modules
	vehicle[619].gets(delax);
	vehicle[620].gets(delex);
	vehicle[621].gets(delrx);
}
