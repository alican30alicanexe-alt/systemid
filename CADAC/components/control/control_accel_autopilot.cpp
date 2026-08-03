///////////////////////////////////////////////////////////////////////////////
// COMPONENT: control_accel_autopilot
// CATEGORY: Control
// DoF: 3DoF / 5DoF
//
// DESCRIPTION:
//   Acceleration autopilot with P-I control and rate feedback
//   Commands angle of attack and sideslip to achieve desired g's
//   Use for: Missiles, maneuvering aircraft
//
// INPUTS (from vehicle array):
//   vehicle[0]  - grav - double - Gravity m/s²
//   vehicle[11] - area - double - Reference area m²
//   vehicle[14] - alpmax - double - Maximum angle of attack deg
//   vehicle[26] - cyaim - double - Side force coefficient
//   vehicle[27] - cnaim - double - Normal force coefficient
//   vehicle[28] - cnalp - double - Normal force derivative 1/rad
//   vehicle[29] - cybet - double - Side force derivative 1/rad
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[60] - thrust - double - Thrust N
//   vehicle[61] - mass - double - Vehicle mass kg
//   vehicle[75] - dvba - double - Airspeed m/s
//   vehicle[110] - ancomx - double - Commanded normal accel g's
//   vehicle[111] - alcomx - double - Commanded lateral accel g's
//
// OUTPUTS (to vehicle array):
//   vehicle[143] - alphax - double - Angle of attack deg
//   vehicle[144] - betax - double - Sideslip angle deg
//
// PARAMETERS (from input.asc):
//   vehicle[127] - ta - double - Ratio of prop/integral gain
//   vehicle[128] - tr - double - Rate loop time constant sec
//   vehicle[129] - gacp - double - Root locus gain rad/s²
//
// DEPENDENCIES:
//   - Requires: Aerodynamics for force coefficients and derivatives
//   - Requires: Guidance for acceleration commands
//   - Requires: Environment for dynamic pressure
//   - Requires: Propulsion for thrust and mass
//   - Provides: Angle of attack and sideslip for aerodynamics
//
// REFERENCE:
//   Classical acceleration autopilot with:
//   - Proportional-Integral (P-I) shaping
//   - Rate feedback for stability
//   - Incidence angle lag compensation
//
// USAGE:
//   Include in class header:
//     virtual void def_control();
//     virtual void init_control();
//     virtual void control(double int_step);
//
//   In input.asc MODULES section:
//     control   def,init,exec
//
//   In input.asc DATA section:
//     ta = 2.0       // Prop/integral gain ratio
//     tr = 0.1       // Rate loop time constant, sec
//     gacp = 10.0    // Root locus gain, rad/s²
//     alphax = 0.0   // Initial angle of attack, deg
//     betax = 0.0    // Initial sideslip, deg
//
// NOTES:
//   - Separate loops for pitch (alpha) and yaw (beta)
//   - Limits angles to alpmax to prevent stall
//   - Uses incidence angle lag model for realism
//   - State variables integrated with CADAC integrate() function
//   - Assumes small angle approximation for body rates
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
	vehicle[127].init("ta",0,"Ratio of prop/integral gain - ND","control","data","");
	vehicle[128].init("tr",0,"Rate loop time constant - sec","control","data","");
	vehicle[129].init("gacp",0,"Root locus gain - rad/s2","control","data","");
	//state variables
	vehicle[131].init("xi",0,"Integral feedback pitch - rad/s","control","state","");
	vehicle[132].init("xid",0,"Integral feedback pitch deriv - rad/s^2","control","state","");
	vehicle[133].init("ratep",0,"Pitch rate - rad/s","control","state","");
	vehicle[134].init("ratepd",0,"Pitch rate derivative - rad/s^2","control","state","");
	vehicle[135].init("alp",0,"Angle of attack - rad","control","state","");
	vehicle[136].init("alpd",0,"Angle of attack derivative - rad/s","control","state","");
	vehicle[137].init("yi",0,"Integral feedback yaw - rad/s","control","state","");
	vehicle[138].init("yid",0,"Integral feedback yaw deriv - rad/s^2","control","state","");
	vehicle[139].init("ratey",0,"Yaw rate - rad/s","control","state","");
	vehicle[140].init("rateyd",0,"Yaw rate derivative - rad/s^2","control","state","");
	vehicle[141].init("bet",0,"Sideslip angle - rad","control","state","");
	vehicle[142].init("betd",0,"Sideslip angle derivative - rad/s","control","state","");
	//output to other modules
	vehicle[143].init("alphax",0,"Angle of attack - deg","control","in/out","scrn,plot");
	vehicle[144].init("betax",0,"Sideslip angle - deg","control","in/out","scrn,plot");
	//diagnostics
	vehicle[130].init("tip",0,"Incidence lag time constant - sec","control","diag","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of control module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_control()
{
	//localizing module-variables
	//input data
	double alphax=vehicle[143].real();
	double betax=vehicle[144].real();

	//-------------------------------------------------------------------------
	//Initializing incidence angles (convert to radians)
	double alp = alphax * RAD;
	double bet = betax * RAD;

	//-------------------------------------------------------------------------
	//loading module-variables
	vehicle[135].gets(alp);
	vehicle[141].gets(bet);
}

///////////////////////////////////////////////////////////////////////////////
//Control module
//Member function of class 'Vehicle'
//
// Acceleration autopilot with P-I control
// Dual channel: pitch (alpha) and yaw (beta)
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::control(double int_step)
{
	//local module-variables
	double tip(0);
	double alphax(0);
	double betax(0);

	//localizing module-variables
	//input data
	double ta=vehicle[127].real();
	double tr=vehicle[128].real();
	double gacp=vehicle[129].real();
	//input from other modules
	double grav=vehicle[0].real();
	double pdynmc=vehicle[57].real();
	double dvba=vehicle[75].real();  // Changed from dvae to dvba (airspeed)
	double area=vehicle[11].real();
	double alpmax=vehicle[14].real();
	double cyaim=vehicle[26].real();
	double cnaim=vehicle[27].real();
	double cnalp=vehicle[28].real();
	double cybet=vehicle[29].real();
	double thrust=vehicle[60].real();
	double mass=vehicle[61].real();
	double ancomx=vehicle[110].real();  // Commanded normal accel
	double alcomx=vehicle[111].real();  // Commanded lateral accel
	//state variables
	double xi=vehicle[131].real();
	double xid=vehicle[132].real();
	double ratep=vehicle[133].real();
	double ratepd=vehicle[134].real();
	double alp=vehicle[135].real();
	double alpd=vehicle[136].real();
	double yi=vehicle[137].real();
	double yid=vehicle[138].real();
	double ratey=vehicle[139].real();
	double rateyd=vehicle[140].real();
	double bet=vehicle[141].real();
	double betd=vehicle[142].real();

	//-------------------------------------------------------------------------
	//PITCH ACCELERATION CONTROLLER
	//
	// Control law structure:
	// 1. Acceleration error → P-I shaping → Rate command
	// 2. Rate error → First-order lag → Rate
	// 3. Rate → Incidence lag → Angle of attack

	//Incidence lag time constant (aerodynamic + thrust coupling)
	tip = dvba * mass / (pdynmc * area * fabs(cnalp) + thrust);

	//Pitch specific force (current aerodynamic force)
	double fspz = -pdynmc * area * cnaim / mass;

	//P-I controller gains
	double gr = gacp * tip * tr / dvba;  // Proportional gain
	double gi = gr / ta;                  // Integral gain

	//Acceleration error (commanded - actual)
	double abez = -ancomx * grav;  // Convert g's to m/s²
	double ep = abez - fspz;

	//Integral path
	double xid_new = gi * ep;
	xi = integrate(xid_new, xid, xi, int_step);
	xid = xid_new;

	//Rate command (P + I)
	double ratepc = -(ep * gr + xi);

	//Rate loop (first-order lag)
	double ratepd_new = (ratepc - ratep) / tr;
	ratep = integrate(ratepd_new, ratepd, ratep, int_step);
	ratepd = ratepd_new;

	//Incidence angle lag (vehicle dynamics)
	double alpd_new = (tip * ratep - alp) / tip;
	alp = integrate(alpd_new, alpd, alp, int_step);
	alpd = alpd_new;

	//Convert to degrees for output
	alphax = alp * DEG;

	//Alpha limiter (prevent stall)
	if(fabs(alphax) > alpmax)
		alphax = alpmax * sign(alphax);

	//-------------------------------------------------------------------------
	//YAW ACCELERATION CONTROLLER
	//(Same structure as pitch channel)

	//Incidence lag time constant
	double tiy = dvba * mass / (pdynmc * area * fabs(cybet) + thrust);

	//Yaw specific force
	double fspy = pdynmc * area * cyaim / mass;

	//P-I controller gains
	gr = gacp * tiy * tr / dvba;
	gi = gr / ta;

	//Acceleration error
	double abey = alcomx * grav;
	double ey = abey - fspy;

	//Integral path
	double yid_new = gi * ey;
	yi = integrate(yid_new, yid, yi, int_step);
	yid = yid_new;

	//Rate command
	double rateyc = (ey * gr + yi);

	//Rate loop
	double rateyd_new = (rateyc - ratey) / tr;
	ratey = integrate(rateyd_new, rateyd, ratey, int_step);
	rateyd = rateyd_new;

	//Incidence angle lag
	double betd_new = -(tiy * ratey + bet) / tiy;
	bet = integrate(betd_new, betd, bet, int_step);
	betd = betd_new;

	//Convert to degrees
	betax = bet * DEG;

	//Beta limiter (same as alpha)
	if(fabs(betax) > alpmax)
		betax = alpmax * sign(betax);

	//-------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[131].gets(xi);
	vehicle[132].gets(xid);
	vehicle[133].gets(ratep);
	vehicle[134].gets(ratepd);
	vehicle[135].gets(alp);
	vehicle[136].gets(alpd);
	vehicle[137].gets(yi);
	vehicle[138].gets(yid);
	vehicle[139].gets(ratey);
	vehicle[140].gets(rateyd);
	vehicle[141].gets(bet);
	vehicle[142].gets(betd);
	//output to other modules
	vehicle[143].gets(alphax);
	vehicle[144].gets(betax);
	//diagnostics
	vehicle[130].gets(tip);
}
