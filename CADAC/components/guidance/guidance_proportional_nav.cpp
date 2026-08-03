///////////////////////////////////////////////////////////////////////////////
// COMPONENT: guidance_proportional_nav
// CATEGORY: Guidance
// DoF: 3DoF / 5DoF
//
// DESCRIPTION:
//   Proportional Navigation Guidance (PNG)
//   Commanded acceleration proportional to line-of-sight rate
//   Classic homing guidance law for interceptors
//   Use for: Air-to-air missiles, surface-to-air missiles, guided projectiles
//
// INPUTS (from vehicle array):
//   vehicle[30] - gmax - double - Maximum g available g's
//   vehicle[81] - dvta - double - Closing velocity m/s
//   vehicle[87] - UTAA - Matrix(3x1) - Unit LOS vector in missile body coords
//   vehicle[88] - WOEA - Matrix(3x1) - LOS rate vector rad/s
//
// OUTPUTS (to vehicle array):
//   vehicle[110] - ancomx - double - Normal acceleration command g's
//   vehicle[111] - alcomx - double - Lateral acceleration command g's
//
// PARAMETERS (from input.asc):
//   vehicle[101] - gnav - double - Navigation gain (typically 3-5)
//
// DEPENDENCIES:
//   - Requires: Seeker/intercept module for LOS geometry (UTAA, WOEA, dvta)
//   - Requires: Aerodynamics for gmax
//   - Provides: Acceleration commands for autopilot
//
// REFERENCE:
//   Zarchan, P. "Tactical and Strategic Missile Guidance", 6th ed.
//   Classic PNG: a_c = N * Vc * lambda_dot
//   Where: N = navigation gain
//          Vc = closing velocity
//          lambda_dot = LOS rate
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
//     gnav = 3.0     // Navigation gain (3-5 typical)
//
// NOTES:
//   - Navigation gain N=3 is minimum for tail chase
//   - N=4 is typical for most engagements
//   - N=5 provides margin for maneuvering targets
//   - Circular limiter prevents exceeding gmax
//   - Acceleration commands in missile body frame (normal/lateral)
//   - Assumes target tracking from seeker module
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of guidance module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_guidance()
{
	//Definition of module-variables
	//input data
	vehicle[101].init("gnav",0,"Proportional navigation gain - ND","guidance","data","");
	//output to other modules
	vehicle[110].init("ancomx",0,"Normal acceleration command - g's","guidance","out","scrn,plot");
	vehicle[111].init("alcomx",0,"Lateral acceleration command - g's","guidance","out","scrn,plot");
	//diagnostics
	vehicle[106].init("annx",0,"Normal accel command, unrestricted - g's","guidance","diag","");
	vehicle[107].init("allx",0,"Lateral accel command, unrestricted - g's","guidance","diag","");
}

///////////////////////////////////////////////////////////////////////////////
//'guidance' module
//Member function of class 'Vehicle'
//
// Proportional Navigation Guidance
// Calculates acceleration commands to null line-of-sight rate
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::guidance(double int_step)
{
	//local variables
	double phi(0);

	//local module-variables
	double ancomx(0);
	double alcomx(0);
	double annx(0);
	double allx(0);

	//localizing module-variables
	//input data
	double gnav=vehicle[101].real();
	//input from other modules
	double grav=vehicle[0].real();      // Gravity for converting m/s^2 to g's
	double gmax=vehicle[30].real();      // Max g available
	double dvta=vehicle[81].real();      // Closing velocity
	Matrix UTAA=vehicle[87].vec();       // Unit LOS vector
	Matrix WOEA=vehicle[88].vec();       // LOS rate vector

	//-------------------------------------------------------------------------
	//Proportional Navigation Guidance
	//
	// PNG Law: a_c = N * Vc * omega_LOS
	// Where: a_c = commanded acceleration
	//        N = navigation gain
	//        Vc = closing velocity
	//        omega_LOS = line-of-sight rate
	//
	// In vector form: APNA = gnav * |dvta| * (WOEA x UTAA)

	//Acceleration command in missile body axes
	Matrix APNA=WOEA.skew_sym()*UTAA*gnav*fabs(dvta);

	//Extract normal (pitch) and lateral (yaw) components
	//Note: Body frame X=forward, Y=right, Z=down
	annx=-APNA.get_loc(2,0)/grav;  // Normal (pitch) accel in g's
	allx=APNA.get_loc(1,0)/grav;   // Lateral (yaw) accel in g's

	//Limiting acceleration commands by circular limiter
	//This prevents exceeding vehicle's aerodynamic capability (gmax)
	double aax=sqrt(allx*allx+annx*annx);
	if(aax>gmax) aax=gmax;

	//Preserve direction while limiting magnitude
	if((fabs(annx)<SMALL||fabs(allx)<SMALL))
		phi=0;
	else{
		phi=atan2(annx,allx);
	}
	alcomx=aax*cos(phi);
	ancomx=aax*sin(phi);

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[110].gets(ancomx);
	vehicle[111].gets(alcomx);
	//diagnostics
	vehicle[106].gets(annx);
	vehicle[107].gets(allx);
}
