///////////////////////////////////////////////////////////////////////////////
// COMPONENT: control_rate_damping
// CATEGORY: Control
// DoF: 5DoF / 6DoF
//
// DESCRIPTION:
//   Simple rate damping autopilot
//   Provides stability by damping body rates
//   Use for: Basic stabilization, rockets, simple missiles
//
// INPUTS (from vehicle array):
//   vehicle[164] - WBIB - Matrix(3x1) - Body rates rad/s
//
// OUTPUTS (to vehicle array):
//   vehicle[519] - delacx - double - Aileron command deg
//   vehicle[520] - delecx - double - Elevator command deg
//   vehicle[521] - delrcx - double - Rudder command deg
//
// PARAMETERS (from input.asc):
//   vehicle[522] - kp_roll - double - Roll rate gain
//   vehicle[523] - kp_pitch - double - Pitch rate gain
//   vehicle[524] - kp_yaw - double - Yaw rate gain
//
// DEPENDENCIES:
//   - Requires: Euler/attitude module for body rates
//   - Provides: Control deflections for actuators/aerodynamics
//
// REFERENCE:
//   Simple proportional rate feedback
//   delta = -K * omega
//
// USAGE:
//   Include in class header:
//     virtual void def_control();
//     virtual void control(double int_step);
//
//   In input.asc MODULES section:
//     control   def,exec
//
//   In input.asc DATA section:
//     kp_roll = 10.0    // Roll damping gain, deg/(rad/s)
//     kp_pitch = 5.0    // Pitch damping gain
//     kp_yaw = 5.0      // Yaw damping gain
//
// NOTES:
//   - Pure damping (no commanded rates)
//   - Negative feedback for stability
//   - Gain units: deg deflection per rad/s rate
//   - For full autopilot, use control_accel_autopilot
//   - Simple but effective for basic stabilization
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of control module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_control()
{
	//Definition of module-variables
	//input data
	vehicle[522].init("kp_roll",0,"Roll rate gain - deg/(rad/s)","control","data","");
	vehicle[523].init("kp_pitch",0,"Pitch rate gain - deg/(rad/s)","control","data","");
	vehicle[524].init("kp_yaw",0,"Yaw rate gain - deg/(rad/s)","control","data","");
	//output to other modules
	vehicle[519].init("delacx",0,"Aileron command - deg","control","out","scrn,plot");
	vehicle[520].init("delecx",0,"Elevator command - deg","control","out","scrn,plot");
	vehicle[521].init("delrcx",0,"Rudder command - deg","control","out","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Control module
//Member function of class 'Vehicle'
//
// Rate damping autopilot
// Provides basic stability through rate feedback
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::control(double int_step)
{
	//local module-variables
	double delacx(0);
	double delecx(0);
	double delrcx(0);

	//localizing module-variables
	//input data
	double kp_roll=vehicle[522].real();
	double kp_pitch=vehicle[523].real();
	double kp_yaw=vehicle[524].real();
	//input from other modules
	Matrix WBIB=vehicle[164].vec();

	//-------------------------------------------------------------------------
	//Body rates (rad/s)
	double wroll = WBIB[0];   // Roll rate (p)
	double wpitch = WBIB[1];  // Pitch rate (q)
	double wyaw = WBIB[2];    // Yaw rate (r)

	//Rate damping control law: delta = -K * omega
	//Negative feedback provides damping

	//Roll channel: aileron opposes roll rate
	delacx = -kp_roll * wroll;

	//Pitch channel: elevator opposes pitch rate
	delecx = -kp_pitch * wpitch;

	//Yaw channel: rudder opposes yaw rate
	delrcx = -kp_yaw * wyaw;

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[519].gets(delacx);
	vehicle[520].gets(delecx);
	vehicle[521].gets(delrcx);
}
