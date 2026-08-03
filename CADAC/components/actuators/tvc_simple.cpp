///////////////////////////////////////////////////////////////////////////////
// COMPONENT: tvc_simple
// CATEGORY: Actuators
// DoF: 6DoF
//
// DESCRIPTION:
//   Thrust Vector Control (TVC) for rocket engines
//   Converts control commands to nozzle deflections with dynamics
//   Computes thrust forces and moments from nozzle deflection
//   Use for: Rockets, missiles with gimbaled nozzles
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[17] - xcg - double - CG location from nose m
//   vehicle[26] - thrust - double - Thrust N
//   vehicle[520] - delecx - double - Pitch command deg
//   vehicle[521] - delrcx - double - Yaw command deg
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa (for variable gain)
//
// OUTPUTS (to vehicle array):
//   vehicle[910] - FPB - Matrix(3x1) - Thrust vector in body N
//   vehicle[911] - FMPB - Matrix(3x1) - Thrust moments in body N·m
//   vehicle[912] - etax - double - Nozzle pitch deflection deg
//   vehicle[913] - zetx - double - Nozzle yaw deflection deg
//   vehicle[916-923] - State variables for nozzle dynamics rad, rad/s
//
// PARAMETERS (from input.asc):
//   vehicle[900] - mtvc - int - TVC mode (0=off, 1=no dynamics, 2=2nd order)
//   vehicle[902] - tvclimx - double - Nozzle deflection limit deg
//   vehicle[904] - dtvclimx - double - Nozzle rate limit deg/s
//   vehicle[905] - wntvc - double - TVC natural frequency rad/s
//   vehicle[906] - zettvc - double - TVC damping
//   vehicle[908] - gtvc - double - TVC gain (deflection per command)
//   vehicle[909] - parm - double - Propulsion moment arm from nose m
//
// DEPENDENCIES:
//   - Requires: Propulsion module for thrust
//   - Requires: Control module for delecx, delrcx
//   - Provides: FPB and FMPB for forces module
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 14: Actuators
//
//   TVC thrust vector:
//     Fx = T * cos(eta) * cos(zeta)
//     Fy = T * cos(eta) * sin(zeta)
//     Fz = -T * sin(eta)
//
//   TVC moments:
//     Mx = 0 (no roll moment from TVC)
//     My = (parm - xcg) * Fz  (pitch moment)
//     Mz = -(parm - xcg) * Fy (yaw moment)
//
//   Second-order dynamics:
//     η̈ + 2*ζ*ω_n*η̇ + ω_n²*η = ω_n²*η_cmd
//
// USAGE:
//   Include in class header:
//     virtual void def_tvc();
//     virtual void tvc(double int_step);
//
//   In input.asc MODULES section:
//     tvc   def,exec
//
//   In input.asc DATA section:
//     mtvc = 2            // Second-order dynamics
//     gtvc = 1.0          // Unity gain
//     tvclimx = 10        // ±10° limit
//     dtvclimx = 50       // 50°/s rate limit
//     wntvc = 20          // 20 rad/s bandwidth
//     zettvc = 0.7        // 0.7 damping
//     parm = 5.0          // Moment arm, m from nose
//
// NOTES:
//   - mtvc modes: 0=no TVC, 1=no dynamics (instant), 2=2nd order lag
//   - gtvc typically 0.5-1.0 (full deflection per degree command)
//   - Moment arm = distance from nozzle to CG (parm - xcg)
//   - Positive eta = nozzle down, thrust up (positive pitch moment)
//   - Positive zeta = nozzle right, thrust left (negative yaw moment)
//   - Sign conventions: X forward, Y right, Z down
//   - TVC effective at low dynamic pressure (launch, high altitude)
//   - Rate limiting prevents excessive nozzle slew rates
//   - Position limiting prevents over-deflection
//   - Typical TVC bandwidth: 10-30 rad/s
//   - Higher bandwidth requires stiffer actuators
//   - Moment arm decreases as fuel burns (CG moves aft)
//   - Small deflections approximation: sin(θ) ≈ θ, cos(θ) ≈ 1
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of TVC module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_tvc()
{
	//Definition and initialization of module-variables
	vehicle[900].init("mtvc","int",0,"=0:no TVC;=1:no dyn;=2:scnd order;=3:2+gain","tvc","data","");
	vehicle[902].init("tvclimx",0,"Nozzle deflection limiter - deg","tvc","data","");
	vehicle[904].init("dtvclimx",0,"Nozzle deflection rate limiter - deg/s","tvc","data","");
	vehicle[905].init("wntvc",0,"Natural frequency of TVC - rad/s","tvc","data","");
	vehicle[906].init("zettvc",0,"Damping of TVC - ND","tvc","data","");
	vehicle[908].init("gtvc",0,"TVC nozzle deflection gain - ND","tvc","data,diag","");
	vehicle[909].init("parm",0,"Propulsion moment arm from vehicle nose - m","tvc","data","");
	vehicle[910].init("FPB",0,0,0,"Thrust force in body axes - N","tvc","out","");
	vehicle[911].init("FMPB",0,0,0,"Thrust moment in body axes - Nm","tvc","out","");
	vehicle[912].init("etax",0,"Nozzle pitch deflection - deg","tvc","diag","plot");
	vehicle[913].init("zetx",0,"Nozzle yaw deflection - deg","tvc","diag","plot");
	vehicle[916].init("etasd",0,"Pitch nozzle derivative - rad/s","tvc","state","");
	vehicle[917].init("zetad",0,"Yaw nozzle derivative - rad/s","tvc","state","");
	vehicle[918].init("etas",0,"Pitch nozzle deflection - rad","tvc","state","");
	vehicle[919].init("zeta",0,"Yaw nozzle deflection - rad","tvc","state","");
	vehicle[920].init("detasd",0,"Pitch nozzle rate derivative - rad/s^2","tvc","state","");
	vehicle[921].init("dzetad",0,"Yaw nozzle rate derivative - rad/s^2","tvc","state","");
	vehicle[922].init("detas",0,"Pitch nozzle rate - rad/s","tvc","state","");
	vehicle[923].init("dzeta",0,"Yaw nozzle rate - rad/s","tvc","state","");
}

///////////////////////////////////////////////////////////////////////////////
//TVC module
//Member function of class 'Vehicle'
//
// Converts control commands to nozzle deflections
// Computes thrust forces and moments from deflections
//
// 030608 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::tvc(double int_step)
{
	//local variables
	double eta(0), zet(0);

	//local module-variables
	Matrix FPB(3,1);
	Matrix FMPB(3,1);
	double etax(0), zetx(0);

	//localizing module-variables
	//input data
	int mtvc=vehicle[900].integer();
	double gtvc=vehicle[908].real();
	double parm=vehicle[909].real();
	//input from other modules
	double time=vehicle[0].real();
	double xcg=vehicle[17].real();
	double thrust=vehicle[26].real();
	double delecx=vehicle[520].real();  // Pitch command from control
	double delrcx=vehicle[521].real();  // Yaw command from control
	//----------------------------------------------------------------------------
	//**RETURN IF NO TVC**
	if(mtvc == 0) return;

	//**CONVERT CONTROL COMMANDS TO NOZZLE DEFLECTION COMMANDS**
	// Apply TVC gain
	double etac = gtvc * delecx * RAD;  // Pitch deflection command
	double zetc = gtvc * delrcx * RAD;  // Yaw deflection command

	//**NO NOZZLE DYNAMICS** (instant response)
	if(mtvc == 1) {
		eta = etac;
		zet = zetc;
	}

	//**SECOND ORDER NOZZLE DYNAMICS**
	if(mtvc >= 2)
		tvc_scnd(eta, zet, etac, zetc, int_step);

	//----------------------------------------------------------------------------
	//**COMPUTE THRUST FORCES IN BODY FRAME**
	//
	// Thrust vector from nozzle deflection angles:
	//   Fx = T * cos(eta) * cos(zeta)  // Forward thrust
	//   Fy = T * cos(eta) * sin(zeta)  // Side thrust
	//   Fz = -T * sin(eta)              // Down thrust (negative up)
	double seta = sin(eta);
	double ceta = cos(eta);
	double czet = cos(zet);
	double szet = sin(zet);

	FPB[0] = ceta * czet * thrust;   // X (forward)
	FPB[1] = ceta * szet * thrust;   // Y (right)
	FPB[2] = -seta * thrust;         // Z (down)

	//**COMPUTE THRUST MOMENTS IN BODY FRAME**
	//
	// Moment arm from nozzle gimbal to CG
	double arm = parm - xcg;

	// Moments from thrust vector offset:
	//   Mx = 0 (no roll moment)
	//   My = arm * Fz (pitch moment)
	//   Mz = -arm * Fy (yaw moment)
	FMPB[0] = 0;           // Roll (no TVC roll moment)
	FMPB[1] = arm * FPB[2];   // Pitch
	FMPB[2] = -arm * FPB[1];  // Yaw

	//**OUTPUT ANGLES IN DEGREES**
	etax = eta * DEG;
	zetx = zet * DEG;
	//----------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[910].gets_vec(FPB);   // To forces module
	vehicle[911].gets_vec(FMPB);  // To forces module
	//diagnostics
	vehicle[912].gets(etax);
	vehicle[913].gets(zetx);
}

///////////////////////////////////////////////////////////////////////////////
//Second order TVC dynamics
//Member function of class 'Vehicle'
//
// Models second order lags with position and rate limiting
//
// 030608 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::tvc_scnd(double &eta, double &zet, double etac, double zetc, double int_step)
{
	//localizing module-variables
	//input data
	double tvclimx=vehicle[902].real();
	double dtvclimx=vehicle[904].real();
	double wntvc=vehicle[905].real();
	double zettvc=vehicle[906].real();
	//state variables
	double etasd=vehicle[916].real();
	double zetad=vehicle[917].real();
	double etas=vehicle[918].real();
	double zeta=vehicle[919].real();
	double detasd=vehicle[920].real();
	double dzetad=vehicle[921].real();
	double detas=vehicle[922].real();
	double dzeta=vehicle[923].real();
	//----------------------------------------------------------------------------
	//**PITCH NOZZLE DYNAMICS**
	//
	// Position limiting
	if(fabs(etas) > tvclimx*RAD) {
		etas = tvclimx*RAD * sign(etas);
		if(etas*detas > 0.) detas = 0.;  // Stop if moving into limit
	}

	// Rate limiting
	int iflag = 0;
	if(fabs(detas) > dtvclimx*RAD) {
		iflag = 1;
		detas = dtvclimx*RAD * sign(detas);
	}

	// State integration: position
	double etasd_new = detas;
	etas = integrate(etasd_new, etasd, etas, int_step);
	etasd = etasd_new;

	// Second-order dynamics: η̈ = ω_n²*(η_cmd - η) - 2*ζ*ω_n*η̇
	double eetas = etac - etas;  // Error
	double detasd_new = wntvc*wntvc*eetas - 2.*zettvc*wntvc*etasd;
	detas = integrate(detasd_new, detasd, detas, int_step);
	detasd = detasd_new;

	// Stop acceleration if rate limited and still accelerating
	if(iflag && detas*detasd > 0.) detasd = 0.;

	eta = etas;

	//----------------------------------------------------------------------------
	//**YAW NOZZLE DYNAMICS** (same structure as pitch)
	//
	// Position limiting
	if(fabs(zeta) > tvclimx*RAD) {
		zeta = tvclimx*RAD * sign(zeta);
		if(zeta*dzeta > 0.) dzeta = 0.;
	}

	// Rate limiting
	iflag = 0;
	if(fabs(dzeta) > dtvclimx*RAD) {
		iflag = 1;
		dzeta = dtvclimx*RAD * sign(dzeta);
	}

	// State integration: position
	double zetad_new = dzeta;
	zeta = integrate(zetad_new, zetad, zeta, int_step);
	zetad = zetad_new;

	// Second-order dynamics
	double ezeta = zetc - zeta;
	double dzetad_new = wntvc*wntvc*ezeta - 2.*zettvc*wntvc*zetad;
	dzeta = integrate(dzetad_new, dzetad, dzeta, int_step);
	dzetad = dzetad_new;

	// Stop acceleration if rate limited
	if(iflag && dzeta*dzetad > 0.) dzetad = 0.;

	zet = zeta;
	//----------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[916].gets(etasd);
	vehicle[917].gets(zetad);
	vehicle[918].gets(etas);
	vehicle[919].gets(zeta);
	vehicle[920].gets(detasd);
	vehicle[921].gets(dzetad);
	vehicle[922].gets(detas);
	vehicle[923].gets(dzeta);
}
