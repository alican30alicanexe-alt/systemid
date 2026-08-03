///////////////////////////////////////////////////////////////////////////////
// COMPONENT: target_const_velocity
// CATEGORY: Navigation
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Target moving with constant velocity
//   Integrates position from initial conditions
//   Use for: Aircraft, ships, vehicles
//
// INPUTS (from vehicle array):
//   None
//
// OUTPUTS (to vehicle array):
//   vehicle[200] - STEL - Matrix(3x1) - Target position in local frame m
//   vehicle[201] - VTEL - Matrix(3x1) - Target velocity in local frame m/s
//
// PARAMETERS (from input.asc):
//   vehicle[202] - stel1 - double - Initial target X position (east) m
//   vehicle[203] - stel2 - double - Initial target Y position (north) m
//   vehicle[204] - stel3 - double - Initial target Z position (down) m
//   vehicle[205] - vtel1 - double - Target X velocity (east) m/s
//   vehicle[206] - vtel2 - double - Target Y velocity (north) m/s
//   vehicle[207] - vtel3 - double - Target Z velocity (down) m/s
//
// DEPENDENCIES:
//   - Requires: None
//   - Provides: Target state for seeker and guidance
//
// REFERENCE:
//   Simple kinematic propagation: s = s0 + v*t
//
// USAGE:
//   Include in class header:
//     virtual void def_target();
//     virtual void init_target();
//     virtual void target(double int_step);
//
//   In input.asc MODULES section:
//     target   def,init,exec
//
//   In input.asc DATA section:
//     stel1 = 5000     // Initial 5km east
//     stel2 = 10000    // Initial 10km north
//     stel3 = -3000    // 3km altitude (Z is down)
//     vtel1 = 100      // 100 m/s east
//     vtel2 = 50       // 50 m/s north
//     vtel3 = 0        // Level flight
//
// NOTES:
//   - Local level frame: X=East, Y=North, Z=Down
//   - Velocity is constant (no acceleration)
//   - For maneuvering targets, use target_accel or target_waypoint
//   - For stationary targets, use target_fixed
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of target module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_target()
{
	//Definition of module-variables
	//input data
	vehicle[202].init("stel1",0,"Target initial X position (east) - m","target","data","");
	vehicle[203].init("stel2",0,"Target initial Y position (north) - m","target","data","");
	vehicle[204].init("stel3",0,"Target initial Z position (down) - m","target","data","");
	vehicle[205].init("vtel1",0,"Target X velocity (east) - m/s","target","data","");
	vehicle[206].init("vtel2",0,"Target Y velocity (north) - m/s","target","data","");
	vehicle[207].init("vtel3",0,"Target Z velocity (down) - m/s","target","data","");
	//state variables
	vehicle[208].init("STEL",0,0,0,"Target position - m","target","state","");
	//output to other modules
	vehicle[200].init("STEL",0,0,0,"Target position in local frame - m","target","out","");
	vehicle[201].init("VTEL",0,0,0,"Target velocity in local frame - m/s","target","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of target module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_target()
{
	//local module-variables
	Matrix STEL(3,1);
	Matrix VTEL(3,1);

	//input data
	double stel1=vehicle[202].real();
	double stel2=vehicle[203].real();
	double stel3=vehicle[204].real();
	double vtel1=vehicle[205].real();
	double vtel2=vehicle[206].real();
	double vtel3=vehicle[207].real();

	//-------------------------------------------------------------------------
	//Build initial target position and velocity vectors
	STEL.build_vec3(stel1, stel2, stel3);
	VTEL.build_vec3(vtel1, vtel2, vtel3);

	//-------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[208].gets_vec(STEL);
	//output
	vehicle[200].gets_vec(STEL);
	vehicle[201].gets_vec(VTEL);
}

///////////////////////////////////////////////////////////////////////////////
//Target module
//Member function of class 'Vehicle'
//
// Constant velocity target motion
// Simple kinematic integration
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::target(double int_step)
{
	//local module-variables
	Matrix STEL(3,1);
	Matrix VTEL(3,1);

	//localizing module-variables
	//input data
	double vtel1=vehicle[205].real();
	double vtel2=vehicle[206].real();
	double vtel3=vehicle[207].real();
	//state variables
	STEL=vehicle[208].vec();

	//-------------------------------------------------------------------------
	//Target velocity (constant)
	VTEL.build_vec3(vtel1, vtel2, vtel3);

	//Integrate position: s = s0 + v*dt
	Matrix STELD = VTEL;
	STEL = integrate(STELD, Matrix(3,1), STEL, int_step);

	//-------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[208].gets_vec(STEL);
	//output to other modules
	vehicle[200].gets_vec(STEL);
	vehicle[201].gets_vec(VTEL);
}
