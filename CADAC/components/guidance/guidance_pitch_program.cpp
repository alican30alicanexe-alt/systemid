///////////////////////////////////////////////////////////////////////////////
// COMPONENT: guidance_pitch_program
// CATEGORY: Guidance
// DoF: 6DoF
//
// DESCRIPTION:
//   Simple pitch angle guidance using a time-based pitch program
//   Commands flight path angle or angle of attack vs time
//   Use for: Launch vehicles, simple ascent trajectories
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[229] - thtvdx - double - Current flight path angle deg
//
// OUTPUTS (to vehicle array):
//   vehicle[400] - mguide - int - Guidance mode flag
//   vehicle[481] - alphacomx - double - Alpha command deg
//   vehicle[482] - betacomx - double - Beta command deg
//   vehicle[485] - thtvdxcom - double - Flight path angle command deg
//
// PARAMETERS (from input.asc):
//   vehicle[400] - mguide - int - Guidance mode (0=none, 1=pitch program)
//   vehicle[486] - npitch - int - Number of pitch program points
//   vehicle[487-496] - time_table[10] - double - Time breakpoints sec
//   vehicle[497-506] - pitch_table[10] - double - Pitch angle breakpoints deg
//
// DEPENDENCIES:
//   - Requires: Kinematics module for current flight path angle
//   - Provides: Pitch commands for control/autopilot module
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 11: Guidance
//
//   Pitch program guidance:
//     theta_cmd(t) = interpolate(time_table, pitch_table, t)
//
//   Typical launch vehicle pitch program:
//     t < 10s:  theta = 90° (vertical ascent)
//     10-60s:   theta decreases (gravity turn)
//     t > 60s:  theta = constant (orbital injection angle)
//
// USAGE:
//   Include in class header:
//     virtual void def_guidance();
//     virtual void guidance(double int_step);
//
//   In input.asc MODULES section:
//     guidance   def,exec
//
//   In input.asc DATA section:
//     mguide = 1          // Pitch program mode
//     npitch = 5          // Number of breakpoints
//     // Time table (sec)
//     time_table = 0, 10, 30, 60, 120
//     // Pitch table (deg) - flight path angle
//     pitch_table = 90, 85, 60, 30, 10
//
// NOTES:
//   - mguide modes: 0=no guidance, 1=pitch program, 5=LTG (advanced)
//   - Linear interpolation between breakpoints
//   - Holds last value after final breakpoint
//   - Flight path angle: 90° = vertical, 0° = horizontal
//   - For rockets: typically start vertical (90°), gradually decrease
//   - Gravity turn: allows vehicle to follow natural arc
//   - Can command alpha instead of flight path angle if needed
//   - Beta typically held at 0° (no sideslip)
//   - Simple and robust for predetermined trajectories
//   - Does not adapt to off-nominal conditions
//   - Suitable for launch vehicles with known trajectory
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of guidance module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_guidance()
{
	//definition of module-variables
	vehicle[400].init("mguide","int",0,"Guidance modes: 0=none, 1=pitch program","guidance","data","");
	vehicle[481].init("alphacomx",0, "Alpha command - deg", "guidance", "out","");
	vehicle[482].init("betacomx", 0, "Beta command - deg", "guidance", "out","");
	vehicle[485].init("thtvdxcom",0,"Flight path angle command - deg","guidance","out","");
	vehicle[486].init("npitch","int",0,"Number of pitch program points","guidance","data","");
	//pitch program tables (max 10 points)
	vehicle[487].init("time_table0",0,"Pitch time breakpoint 0 - sec","guidance","data","");
	vehicle[488].init("time_table1",0,"Pitch time breakpoint 1 - sec","guidance","data","");
	vehicle[489].init("time_table2",0,"Pitch time breakpoint 2 - sec","guidance","data","");
	vehicle[490].init("time_table3",0,"Pitch time breakpoint 3 - sec","guidance","data","");
	vehicle[491].init("time_table4",0,"Pitch time breakpoint 4 - sec","guidance","data","");
	vehicle[492].init("time_table5",0,"Pitch time breakpoint 5 - sec","guidance","data","");
	vehicle[493].init("time_table6",0,"Pitch time breakpoint 6 - sec","guidance","data","");
	vehicle[494].init("time_table7",0,"Pitch time breakpoint 7 - sec","guidance","data","");
	vehicle[495].init("time_table8",0,"Pitch time breakpoint 8 - sec","guidance","data","");
	vehicle[496].init("time_table9",0,"Pitch time breakpoint 9 - sec","guidance","data","");
	vehicle[497].init("pitch_table0",0,"Pitch angle breakpoint 0 - deg","guidance","data","");
	vehicle[498].init("pitch_table1",0,"Pitch angle breakpoint 1 - deg","guidance","data","");
	vehicle[499].init("pitch_table2",0,"Pitch angle breakpoint 2 - deg","guidance","data","");
	vehicle[500].init("pitch_table3",0,"Pitch angle breakpoint 3 - deg","guidance","data","");
	vehicle[501].init("pitch_table4",0,"Pitch angle breakpoint 4 - deg","guidance","data","");
	vehicle[502].init("pitch_table5",0,"Pitch angle breakpoint 5 - deg","guidance","data","");
	vehicle[503].init("pitch_table6",0,"Pitch angle breakpoint 6 - deg","guidance","data","");
	vehicle[504].init("pitch_table7",0,"Pitch angle breakpoint 7 - deg","guidance","data","");
	vehicle[505].init("pitch_table8",0,"Pitch angle breakpoint 8 - deg","guidance","data","");
	vehicle[506].init("pitch_table9",0,"Pitch angle breakpoint 9 - deg","guidance","data","");
}

///////////////////////////////////////////////////////////////////////////////
//Guidance module
//Member function of class 'Vehicle'
//
// Implements simple pitch program guidance
// Commands flight path angle based on time table
//
// 030616 Created by Peter Zipfel
// xxxxxx Adapted to component library (simplified to pitch program only)
///////////////////////////////////////////////////////////////////////////////
void Vehicle::guidance(double int_step)
{
	//local module-variables
	double alphacomx(0);
	double betacomx(0);
	double thtvdxcom(0);

	//localizing module-variables
	//input data
	int mguide=vehicle[400].integer();
	int npitch=vehicle[486].integer();
	//input from other modules
	double time=vehicle[0].real();
	double thtvdx=vehicle[229].real();  // Current flight path angle
	//----------------------------------------------------------------------------
	//**NO GUIDANCE**
	if(mguide == 0) {
		alphacomx = 0;
		betacomx = 0;
		thtvdxcom = thtvdx;  // Hold current angle
	}

	//**PITCH PROGRAM GUIDANCE**
	if(mguide == 1 && npitch > 0) {
		// Load time and pitch tables
		double time_table[10];
		double pitch_table[10];

		time_table[0] = vehicle[487].real();
		time_table[1] = vehicle[488].real();
		time_table[2] = vehicle[489].real();
		time_table[3] = vehicle[490].real();
		time_table[4] = vehicle[491].real();
		time_table[5] = vehicle[492].real();
		time_table[6] = vehicle[493].real();
		time_table[7] = vehicle[494].real();
		time_table[8] = vehicle[495].real();
		time_table[9] = vehicle[496].real();

		pitch_table[0] = vehicle[497].real();
		pitch_table[1] = vehicle[498].real();
		pitch_table[2] = vehicle[499].real();
		pitch_table[3] = vehicle[500].real();
		pitch_table[4] = vehicle[501].real();
		pitch_table[5] = vehicle[502].real();
		pitch_table[6] = vehicle[503].real();
		pitch_table[7] = vehicle[504].real();
		pitch_table[8] = vehicle[505].real();
		pitch_table[9] = vehicle[506].real();

		// Linear interpolation of pitch program
		if(time <= time_table[0]) {
			// Before first breakpoint
			thtvdxcom = pitch_table[0];
		}
		else if(time >= time_table[npitch-1]) {
			// After last breakpoint - hold final value
			thtvdxcom = pitch_table[npitch-1];
		}
		else {
			// Interpolate between breakpoints
			for(int i = 0; i < npitch-1; i++) {
				if(time >= time_table[i] && time <= time_table[i+1]) {
					// Linear interpolation
					double dt = time_table[i+1] - time_table[i];
					double dp = pitch_table[i+1] - pitch_table[i];
					thtvdxcom = pitch_table[i] + dp * (time - time_table[i]) / dt;
					break;
				}
			}
		}

		// For pitch program, we command flight path angle
		// Autopilot will convert to alpha/beta as needed
		alphacomx = 0;   // Not used in flight path angle mode
		betacomx = 0;    // No sideslip command
	}
	//----------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[481].gets(alphacomx);
	vehicle[482].gets(betacomx);
	vehicle[485].gets(thtvdxcom);
}
