///////////////////////////////////////////////////////////////////////////////
// COMPONENT: time_management
// CATEGORY: Utilities
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Simulation time tracking and step size management
//   Provides time for all modules
//   Use for: All simulations
//
// INPUTS (from vehicle array):
//   None (receives sim_time from executive)
//
// OUTPUTS (to vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[1] - event_time - double - Time since last event sec
//
// PARAMETERS (from input.asc):
//   None
//
// DEPENDENCIES:
//   - Requires: Executive for sim_time
//   - Provides: Time for all other modules
//
// REFERENCE:
//   Standard time management for CADAC simulations
//
// USAGE:
//   Include in class header:
//     virtual void def_time();
//     virtual void init_time(double sim_time);
//     virtual void time_management(double sim_time, double event_time);
//
//   In input.asc MODULES section:
//     time   def,init,exec
//
// NOTES:
//   - This component is essential for all simulations
//   - time = absolute simulation time since t=0
//   - event_time = time since last event (useful for triggers)
//   - Typically the first module to execute
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of time module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_time()
{
	//Definition of module-variables
	vehicle[0].init("time",0,"Simulation time - sec","time","exec","scrn,plot,com");
	vehicle[1].init("event_time",0,"Time since last event - sec","time","exec","");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of time module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_time(double sim_time)
{
	//Initialize time to simulation time
	vehicle[0].gets(sim_time);
	vehicle[1].gets(0.0);
}

///////////////////////////////////////////////////////////////////////////////
//Time management module
//Member function of class 'Vehicle'
//
// Updates simulation time and event time
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::time_management(double sim_time, double event_time)
{
	//Update time variables
	vehicle[0].gets(sim_time);
	vehicle[1].gets(event_time);
}
