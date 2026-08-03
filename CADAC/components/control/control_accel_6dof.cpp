///////////////////////////////////////////////////////////////////////////////
// COMPONENT: control_accel_6dof
// CATEGORY: Control
// DoF: 6DoF
//
// DESCRIPTION:
//   Dual-channel acceleration autopilot for pitch and yaw control
//   Uses pole placement technique with rate and acceleration feedback
//   Generates TVC or control surface deflection commands
//   Use for: Rockets, missiles with acceleration command tracking
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[145] - dla - double - Lift slope derivative m/s²
//   vehicle[147] - dma - double - Pitch moment derivative 1/s²
//   vehicle[148] - dmq - double - Pitch damping derivative 1/s
//   vehicle[149] - dmde - double - Pitch control derivative 1/s²
//   vehicle[150] - dyb - double - Side force derivative m/s²
//   vehicle[152] - dnb - double - Yaw moment derivative 1/s²
//   vehicle[153] - dnr - double - Yaw damping derivative 1/s
//   vehicle[154] - dndr - double - Yaw control derivative 1/s²
//   vehicle[225] - dvbe - double - Geographic speed m/s
//   vehicle[321] - qqcx - double - Pitch rate deg/s
//   vehicle[322] - rrcx - double - Yaw rate deg/s
//   vehicle[334] - FSPCB - Matrix(3x1) - Specific force in body m/s²
//   vehicle[10] - mprop - int - Propulsion mode flag
//   vehicle[191] - gnmax - double - Max g available (pitch) g
//   vehicle[192] - gymax - double - Max g available (yaw) g
//   vehicle[575] - alcomx - double - Lateral accel command g
//   vehicle[576] - ancomx - double - Normal accel command g
//
// OUTPUTS (to vehicle array):
//   vehicle[520] - delecx - double - Pitch command deg
//   vehicle[521] - delrcx - double - Yaw command deg
//   vehicle[514] - yyd - double - Yaw feed-forward derivative m/s² (state)
//   vehicle[515] - yy - double - Yaw feed-forward integral m/s (state)
//   vehicle[516] - zzd - double - Pitch feed-forward derivative m/s² (state)
//   vehicle[517] - zz - double - Pitch feed-forward integral m/s (state)
//
// PARAMETERS (from input.asc):
//   vehicle[500] - maut - int - Autopilot mode (maut=|mauty|mautp|)
//   vehicle[504] - waclp - double - Pitch nat freq rad/s
//   vehicle[505] - zaclp - double - Pitch damping
//   vehicle[506] - paclp - double - Pitch real pole 1/s
//   vehicle[509] - delimx - double - Pitch command limiter deg
//   vehicle[510] - drlimx - double - Yaw command limiter deg
//   vehicle[525] - gainp - double - Pitch prop gain s²/m
//   vehicle[568] - wacly - double - Yaw nat freq rad/s
//   vehicle[569] - zacly - double - Yaw damping
//   vehicle[570] - pacly - double - Yaw real pole 1/s
//   vehicle[571] - gainy - double - Yaw prop gain s²/m
//   vehicle[575] - alcomx - double - Lateral accel command g
//   vehicle[576] - ancomx - double - Normal accel command g
//
// DEPENDENCIES:
//   - Requires: Aerodynamics module for dimensional derivatives
//   - Requires: Kinematics module for rates (qqcx, rrcx)
//   - Requires: Newton module for specific forces (FSPCB)
//   - Provides: Control commands for TVC or actuator module
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 13: Autopilot (p.416)
//
//   Pole placement acceleration autopilot:
//     Gains computed from desired closed-loop poles
//     Three gains: angle feedback, rate feedback, feed-forward
//
//   Control law:
//     δ = -K1*(a_sensed) - K2*(q) + K3*∫(a_cmd - a_sensed)dt + Kp*(a_cmd)
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
//     maut = 53               // Pitch and yaw accel control
//     waclp = 2.0             // Pitch bandwidth, rad/s
//     zaclp = 0.7             // Pitch damping
//     paclp = 1.5             // Pitch real pole, 1/s
//     wacly = 2.0             // Yaw bandwidth, rad/s
//     zacly = 0.7             // Yaw damping
//     pacly = 1.5             // Yaw real pole, 1/s
//     gainp = 0.5             // Pitch prop gain
//     gainy = 0.5             // Yaw prop gain
//     delimx = 10             // Pitch limit, deg
//     drlimx = 10             // Yaw limit, deg
//     ancomx = 2.0            // Normal accel command, g
//     alcomx = 0.0            // Lateral accel command, g
//
// NOTES:
//   - maut flag: mautp (units digit) controls pitch, mauty (tens digit) controls yaw
//     * mautp = 0: no pitch control, 3: pitch accel control
//     * mauty = 0: no yaw control, 5: yaw accel control
//     * Example: maut = 53 means pitch=3 (accel), yaw=5 (accel)
//   - Pole placement: specify desired closed-loop dynamics via poles
//   - Automatic gain scheduling with dynamic pressure (optional)
//   - Acceleration limits enforced from max g available
//   - Feed-forward integrator provides steady-state tracking
//   - Rate feedback provides damping
//   - Acceleration feedback provides fast response
//   - Typical bandwidth: 1-3 rad/s for rockets
//   - Higher bandwidth requires faster actuators
//   - Works with TVC (thrust vector control) or aerosurfaces
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of control module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_control()
{
	//Definition and initialization of module-variables
	vehicle[500].init("maut","int",0,"maut=|mauty|mautp| see table","control","data","");
	vehicle[504].init("waclp",0,"Nat freq of accel close loop complex pole - rad/s","control","data","plot");
	vehicle[505].init("zaclp",0,"Damping of accel close loop complex pole - ND","control","data","plot");
	vehicle[506].init("paclp",0,"Close loop real pole - ND","control","data","plot");
	vehicle[509].init("delimx",0,"Pitch command limiter - deg","control","data","");
	vehicle[510].init("drlimx",0,"Yaw command limiter - deg","control","data","");
	vehicle[514].init("yyd",0,"Yaw feed-forward derivative variable - m/s^2","control","state","");
	vehicle[515].init("yy",0,"Yaw feed-forward integration variable - m/s","control","state","");
	vehicle[516].init("zzd",0,"Pitch feed-forward derivative variable - m/s^2","control","state","");
	vehicle[517].init("zz",0,"Pitch feed-forward integration variable - m/s","control","state","");
	vehicle[520].init("delecx",0,"Pitch command deflection - deg","control","out","");
	vehicle[521].init("delrcx",0,"Yaw  command deflection - deg","control","out","");
	vehicle[525].init("gainp",0,"Proportional gain in pitch acceleration loop - s^2/m","control","data","");
	vehicle[568].init("wacly",0,"Nat freq of accel close loop pole, yaw - rad/s","control","data","plot");
	vehicle[569].init("zacly",0,"Damping of accel close loop pole, yaw - ND","control","data","");
	vehicle[570].init("pacly",0,"Close loop real pole, yaw - ND","control","data","");
	vehicle[571].init("gainy",0,"Gain in lateral acceleration loop - rad/g's","control","data","");
	vehicle[575].init("alcomx",0,"Lateral (horizontal) acceleration command - g's","control","data","plot");
	vehicle[576].init("ancomx",0,"Pitch (normal) acceleration command - g's","control","data","plot");
}

///////////////////////////////////////////////////////////////////////////////
//Control module
//Member function of class 'Vehicle'
//
// Dual-channel acceleration autopilot using pole placement
//
// 030520 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::control(double int_step)
{
	//local module-variables
	double delecx(0), delrcx(0);

	//localizing module-variables
	//input data
	int maut=vehicle[500].integer();
	double delimx=vehicle[509].real();
	double drlimx=vehicle[510].real();
	//input from other modules
	double time=vehicle[0].real();
	int mprop=vehicle[10].integer();
	double gnmax=vehicle[191].real();
	double gymax=vehicle[192].real();
	double alcomx=vehicle[575].real();
	double ancomx=vehicle[576].real();
	//----------------------------------------------------------------------------
	//Decode control flag: maut = |mauty|mautp|
	int mauty = maut / 10;   // Tens digit: yaw control mode
	int mautp = maut % 10;   // Units digit: pitch control mode

	//**YAW ACCELERATION CONTROLLER**
	if(mauty == 5) {
		// Limit lateral acceleration by max g capability
		if(alcomx > gymax) alcomx = gymax;
		if(alcomx < -gymax) alcomx = -gymax;

		// TVC control while thrusting
		if(mprop)
			delrcx = control_yaw_accel(alcomx, int_step);
	}

	//**PITCH ACCELERATION CONTROLLER**
	if(mautp == 3) {
		// Limit normal acceleration by max g capability
		if(ancomx > gnmax) ancomx = gnmax;
		if(ancomx < -gnmax) ancomx = -gnmax;

		// TVC control while thrusting
		if(mprop)
			delecx = control_normal_accel(ancomx, int_step);
	}

	//**APPLY CONTROL LIMITERS**
	if(fabs(delecx) > delimx) delecx = delimx * sign(delecx);
	if(fabs(delrcx) > drlimx) delrcx = drlimx * sign(delrcx);
	//----------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[520].gets(delecx);  // To TVC module
	vehicle[521].gets(delrcx);  // To TVC module
}

///////////////////////////////////////////////////////////////////////////////
//Pitch acceleration controller
//Member function of class 'Vehicle'
//
// Pole placement acceleration controller
// Feedback: pitch rate and normal acceleration
//
// 021015 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
double Vehicle::control_normal_accel(double ancomx, double int_step)
{
	//localizing module-variables
	//input data
	double waclp=vehicle[504].real();
	double zaclp=vehicle[505].real();
	double paclp=vehicle[506].real();
	double gainp=vehicle[525].real();
	//input from other modules
	double pdynmc=vehicle[57].real();
	double dla=vehicle[145].real();
	double dma=vehicle[147].real();
	double dmq=vehicle[148].real();
	double dmde=vehicle[149].real();
	double dvbe=vehicle[225].real();
	double qqcx=vehicle[321].real();
	Matrix FSPCB=vehicle[334].vec();
	//state variables
	double zzd=vehicle[516].real();
	double zz=vehicle[517].real();
	//----------------------------------------------------------------------------
	//**ONLINE POLE ADJUSTMENT** (optional, based on dynamic pressure)
	// waclp = (0.1 + 0.5e-5*(pdynmc - 20e3));
	// paclp = 0.7 + 1e-5*(pdynmc - 20e3);

	//**CALCULATE FEEDBACK GAINS** (pole placement)
	// Three gains: K1 (acceleration), K2 (rate), K3 (integral)
	double gainfb3 = waclp*waclp*paclp / (dla*dmde);
	double gainfb2 = (2*zaclp*waclp + paclp + dmq - dla/dvbe) / dmde;
	double gainfb1 = (waclp*waclp + 2.*zaclp*waclp*paclp + dma + dmq*dla/dvbe
	                  - gainfb2*dmde*dla/dvbe) / (dla*dmde) - gainp;

	//**CONTROL LAW**
	// δ = -K1*(a_sensed) - K2*(q) + K3*∫(a_cmd - a_sensed)dt + Kp*(a_cmd)
	double fspb3 = FSPCB[2];  // Normal acceleration (down)

	// Feed-forward integrator
	double zzd_new = AGRAV*ancomx + fspb3;  // Command - sensed
	zz = integrate(zzd_new, zzd, zz, int_step);
	zzd = zzd_new;

	// Control command
	double dqc = -gainfb1*(-fspb3) - gainfb2*qqcx*RAD + gainfb3*zz + gainp*zzd;
	double delecx = dqc * DEG;
	//----------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[516].gets(zzd);
	vehicle[517].gets(zz);

	return delecx;
}

///////////////////////////////////////////////////////////////////////////////
//Yaw acceleration controller
//Member function of class 'Vehicle'
//
// Pole placement acceleration controller
// Feedback: yaw rate and lateral acceleration
//
// 050104 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
double Vehicle::control_yaw_accel(double alcomx, double int_step)
{
	//localizing module-variables
	//input data
	double wacly=vehicle[568].real();
	double zacly=vehicle[569].real();
	double pacly=vehicle[570].real();
	double gainy=vehicle[571].real();
	//input from other modules
	double pdynmc=vehicle[57].real();
	double dyb=vehicle[150].real();
	double dnb=vehicle[152].real();
	double dnr=vehicle[153].real();
	double dndr=vehicle[154].real();
	double dvbe=vehicle[225].real();
	double rrcx=vehicle[322].real();
	Matrix FSPCB=vehicle[334].vec();
	//state variables
	double yyd=vehicle[514].real();
	double yy=vehicle[515].real();
	//----------------------------------------------------------------------------
	//**ONLINE POLE ADJUSTMENT** (optional, based on dynamic pressure)
	// wacly = (0.1 + 0.5e-5*(pdynmc - 20e3));
	// pacly = 0.7 + 1e-5*(pdynmc - 20e3);

	//**CALCULATE FEEDBACK GAINS** (pole placement)
	double gainfb3 = -wacly*wacly*pacly / (dyb*dndr);
	double gainfb2 = (2*zacly*wacly + pacly + dnr + dyb/dvbe) / dndr;
	double gainfb1 = (-wacly*wacly - 2.*zacly*wacly*pacly + dnb + dnr*dyb/dvbe
	                  - gainfb2*dndr*dnb/dvbe) / (dyb*dndr) - gainy;

	//**CONTROL LAW**
	double fspb2 = FSPCB.get_loc(1,0);  // Lateral acceleration (right)

	// Feed-forward integrator
	double yyd_new = AGRAV*alcomx - fspb2;  // Command - sensed
	yy = integrate(yyd_new, yyd, yy, int_step);
	yyd = yyd_new;

	// Control command
	double drc = -gainfb1*fspb2 - gainfb2*rrcx*RAD + gainfb3*yy + gainy*yyd;
	double drcx = drc * DEG;
	//----------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[514].gets(yyd);
	vehicle[515].gets(yy);

	return drcx;
}
