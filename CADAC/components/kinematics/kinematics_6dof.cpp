///////////////////////////////////////////////////////////////////////////////
// COMPONENT: kinematics_6dof
// CATEGORY: Kinematics
// DoF: 6DoF
//
// DESCRIPTION:
//   Full 6-DoF attitude kinematics with direction cosine matrix integration
//   Computes Euler angles, incidence angles, and coordinate transformations
//   Use for: Rockets, missiles, aircraft in 6DoF
//
// INPUTS (from vehicle array):
//   vehicle[75] - dvba - double - Airspeed m/s
//   vehicle[164] - WBIB - Matrix(3x1) - Body rates rad/s (p,q,r)
//   vehicle[219] - lonx - double - Longitude deg
//   vehicle[220] - latx - double - Latitude deg
//   vehicle[221] - alt - double - Altitude m
//   vehicle[232] - VBED - Matrix(3x1) - Velocity in geodetic frame m/s
//   vehicle[236] - VBII - Matrix(3x1) - Velocity in inertial frame m/s
//   vehicle[72] - VAED - Matrix(3x1) - Wind velocity m/s (optional)
//
// OUTPUTS (to vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[120] - TBD - Matrix(3x3) - DCM body to geodetic
//   vehicle[121] - TBI - Matrix(3x3) - DCM body to inertial (state)
//   vehicle[137] - psibdx - double - Yaw angle deg
//   vehicle[138] - thtbdx - double - Pitch angle deg
//   vehicle[139] - phibdx - double - Roll angle deg
//   vehicle[140] - alppx - double - Total angle of attack deg
//   vehicle[141] - phipx - double - Aerodynamic roll angle deg
//   vehicle[144] - alphax - double - Angle of attack deg
//   vehicle[145] - betax - double - Sideslip angle deg
//   vehicle[123] - ortho_error - double - DCM orthogonality error
//
// PARAMETERS (from input.asc):
//   vehicle[137] - psibdx - double - Initial yaw deg
//   vehicle[138] - thtbdx - double - Initial pitch deg
//   vehicle[139] - phibdx - double - Initial roll deg
//
// DEPENDENCIES:
//   - Requires: Euler module for body rates (WBIB)
//   - Requires: Newton module for position (lon/lat/alt) and velocity (VBED, VBII)
//   - Requires: CADAC utility functions (mat3tr, cad_tdi84)
//   - Provides: Attitude for all modules
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 10: Direction Cosine Matrix Integration
//
//   DCM differential equation: TBI_dot = -skew(WBIB) * TBI
//   where skew(w) is the skew-symmetric matrix of angular velocity
//
// USAGE:
//   Include in class header:
//     virtual void def_kinematics();
//     virtual void init_kinematics(double sim_time, double int_step);
//     virtual void kinematics(double sim_time, double event_time,
//                            double &int_step, double &out_fact);
//
//   In input.asc MODULES section:
//     kinematics   def,init,exec
//
//   In input.asc DATA section:
//     psibdx = 0    // Initial yaw, deg
//     thtbdx = 90   // Initial pitch, deg (vertical)
//     phibdx = 0    // Initial roll, deg
//
// NOTES:
//   - Uses direction cosine matrix (DCM) for attitude representation
//   - DCM remains orthonormal via Gram-Schmidt correction
//   - Avoids gimbal lock inherent in Euler angle integration
//   - Euler angles extracted for display/output only
//   - Handles singularities at pitch = ±90°
//   - Computes both aerodynamic and inertial incidence angles
//   - Requires high-fidelity integration (small time steps ~0.001s)
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of kinematics module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_kinematics()
{
	//Definition of module-variables
	//time management
	vehicle[0].init("time",0,"Simulation time - sec","kinematics","exec","scrn,plot");
	vehicle[1].init("event_time",0,"Time since event - sec","kinematics","exec","");
	vehicle[2].init("int_step_new",0,"New integration step - sec","kinematics","data","");
	vehicle[5].init("stop","int",0,"Stop flag - ND","kinematics","exec","");

	//direction cosine matrices
	vehicle[120].init("TBD",0,0,0,0,0,0,0,0,0,"DCM body to geodetic","kinematics","out","");
	vehicle[121].init("TBI",0,0,0,0,0,0,0,0,0,"DCM body to inertial","kinematics","state","");
	vehicle[122].init("TBID",0,0,0,0,0,0,0,0,0,"DCM derivative - 1/s","kinematics","state","");
	vehicle[123].init("ortho_error",0,"DCM orthogonality error - ND","kinematics","diag","scrn");

	//Euler angles (geodetic frame)
	vehicle[134].init("psibd",0,"Yaw angle - rad","kinematics","diag","");
	vehicle[135].init("thtbd",0,"Pitch angle - rad","kinematics","diag","");
	vehicle[136].init("phibd",0,"Roll angle - rad","kinematics","diag","");
	vehicle[137].init("psibdx",0,"Yaw angle - deg","kinematics","in/out","scrn,plot");
	vehicle[138].init("thtbdx",0,"Pitch angle - deg","kinematics","in/out","scrn,plot");
	vehicle[139].init("phibdx",0,"Rolling angle - deg","kinematics","in/out","scrn,plot");

	//incidence angles
	vehicle[140].init("alppx",0,"Total angle of attack - deg","kinematics","out","scrn,plot");
	vehicle[141].init("phipx",0,"Aerodynamic roll angle - deg","kinematics","out","scrn,plot");
	vehicle[144].init("alphax",0,"Angle of attack - deg","kinematics","out","scrn,plot");
	vehicle[145].init("betax",0,"Sideslip angle - deg","kinematics","diag","scrn,plot");
	vehicle[146].init("alphaix",0,"AOA inertial velocity - deg","kinematics","diag","");
	vehicle[147].init("betaix",0,"Sideslip inertial velocity - deg","kinematics","diag","");
}

///////////////////////////////////////////////////////////////////////////////
//Initialization of kinematics module
//Member function of class 'Vehicle'
//
// Initializes direction cosine matrix from Euler angles
//
// 011126 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_kinematics(double sim_time, double int_step)
{
	//local module-variables
	double time(0);
	double int_step_new(0);
	Matrix TBD(3,3);
	Matrix TBI(3,3);

	//localizing module-variables
	//input data
	double psibdx=vehicle[137].real();
	double thtbdx=vehicle[138].real();
	double phibdx=vehicle[139].real();
	//input from other modules
	double lonx=vehicle[219].real();
	double latx=vehicle[220].real();
	double alt=vehicle[221].real();

	//-------------------------------------------------------------------------
	//Set time to simulation time
	time = sim_time;
	int_step_new = int_step;

	//Initialize DCM from Euler angles
	//mat3tr: 3-2-1 Euler angle sequence (yaw, pitch, roll)
	TBD = mat3tr(psibdx*RAD, thtbdx*RAD, phibdx*RAD);

	//Transform to inertial frame
	//cad_tdi84: WGS84 geodetic to inertial transformation
	Matrix TDI = cad_tdi84(lonx*RAD, latx*RAD, alt, time);
	TBI = TBD * TDI;

	//-------------------------------------------------------------------------
	//loading module-variables
	vehicle[0].gets(time);
	vehicle[2].gets(int_step_new);
	vehicle[120].gets_mat(TBD);
	vehicle[121].gets_mat(TBI);
}

///////////////////////////////////////////////////////////////////////////////
//Kinematics module
//Member function of class 'Vehicle'
//
// Integrates direction cosine matrix using body angular rates
// Extracts Euler angles for display
// Computes incidence angles
//
// 011127 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::kinematics(double sim_time, double event_time,
                        double &int_step, double &out_fact)
{
	//local variables
	Matrix UNIT(3,3);
	UNIT.identity();
	double cthtbd(0);
	double phip(0);

	//local module-variables
	double time(0);
	double ortho_error(0);
	double thtbd(0);
	double psibd(0);
	double phibd(0);
	double psibdx(0);
	double thtbdx(0);
	double phibdx(0);
	double alphax(0);
	double betax(0);
	Matrix TBD(3,3);
	double alppx(0);
	double phipx(0);

	//localizing module-variables
	//input data
	double int_step_new=vehicle[2].real();
	//input from other modules
	double dvba=vehicle[75].real();
	Matrix WBIB=vehicle[164].vec();  // Body rates from euler module
	double lonx=vehicle[219].real();
	double latx=vehicle[220].real();
	double alt=vehicle[221].real();
	Matrix VBED=vehicle[232].vec();  // Velocity in geodetic frame
	Matrix VAED=vehicle[72].vec();   // Wind velocity
	Matrix VBII=vehicle[236].vec();  // Velocity in inertial frame
	//state variables
	Matrix TBI=vehicle[121].mat();
	Matrix TBID=vehicle[122].mat();

	//----------------------------------------------------------------------------
	//Update time
	time = sim_time;
	int_step = int_step_new;

	//----------------------------------------------------------------------------
	//**INTEGRATE DIRECTION COSINE MATRIX**
	//
	// DCM differential equation: TBI_dot = -skew(WBIB) * TBI
	// where skew(w) creates skew-symmetric matrix:
	//     [  0  -r   q ]
	// W = [  r   0  -p ]
	//     [ -q   p   0 ]
	//
	// Note: The transpose (~) of skew_sym gives negative, so:
	// TBID = ~WBIB.skew_sym() * TBI implements the correct equation

	Matrix TBID_NEW = ~WBIB.skew_sym() * TBI;
	TBI = integrate(TBID_NEW, TBID, TBI, int_step);
	TBID = TBID_NEW;

	//**ORTHONORMALIZATION**
	//
	// Numerical integration causes DCM to drift from orthonormality
	// Apply Gram-Schmidt-like correction: TBI = TBI + 0.5*(I - TBI*TBI^T)*TBI

	Matrix EE = UNIT - TBI * ~TBI;  // Error matrix
	TBI = TBI + EE * TBI * 0.5;     // Correction

	//Check orthogonality (diagonal of error matrix)
	double e1 = EE.get_loc(0,0);
	double e2 = EE.get_loc(1,1);
	double e3 = EE.get_loc(2,2);
	ortho_error = sqrt(e1*e1 + e2*e2 + e3*e3);

	//----------------------------------------------------------------------------
	//**EXTRACT EULER ANGLES FROM DCM**
	//
	// Transform DCM from inertial to geodetic frame
	Matrix TDI = cad_tdi84(lonx*RAD, latx*RAD, alt, time);
	TBD = TBI * ~TDI;

	// DCM elements needed for Euler angle extraction
	double tbd13 = TBD.get_loc(0,2);
	double tbd11 = TBD.get_loc(0,0);
	double tbd33 = TBD.get_loc(2,2);
	double tbd12 = TBD.get_loc(0,1);
	double tbd23 = TBD.get_loc(1,2);

	//**Pitch angle** (theta)
	// theta = asin(-tbd13)
	// Handle singularity at |tbd13| >= 1 (gimbal lock)
	if(fabs(tbd13) < 1) {
		thtbd = asin(-tbd13);
		cthtbd = cos(thtbd);
	} else {
		thtbd = PI/2 * sign(-tbd13);
		cthtbd = EPS;  // Small number to avoid division by zero
	}

	//**Yaw angle** (psi)
	// psi = atan2(tbd12, tbd11/cos(theta))
	double cpsi = tbd11 / cthtbd;
	if(fabs(cpsi) > 1)
		cpsi = 1 * sign(cpsi);
	psibd = acos(cpsi) * sign(tbd12);

	//**Roll angle** (phi)
	// phi = atan2(tbd23, tbd33/cos(theta))
	double cphi = tbd33 / cthtbd;
	if(fabs(cphi) > 1)
		cphi = 1 * sign(cphi);
	phibd = acos(cphi) * sign(tbd23);

	// Convert to degrees for output
	psibdx = DEG * psibd;
	thtbdx = DEG * thtbd;
	phibdx = DEG * phibd;

	//----------------------------------------------------------------------------
	//**AERODYNAMIC INCIDENCE ANGLES** (relative to wind)
	//
	// Transform velocity to body frame, account for wind
	Matrix VBAB = TBD * (VBED - VAED);  // Airspeed in body frame
	double vbab1 = VBAB.get_loc(0,0);   // X (forward)
	double vbab2 = VBAB.get_loc(1,0);   // Y (right)
	double vbab3 = VBAB.get_loc(2,0);   // Z (down)

	// Angle of attack: alpha = atan2(vz, vx)
	double alpha = atan2(vbab3, vbab1);

	// Sideslip: beta = asin(vy / |v|)
	double beta = 0;
	if(dvba > 0.1)
		beta = asin(vbab2 / dvba);

	alphax = alpha * DEG;
	betax = beta * DEG;

	//**Aeroballistic angles** (total angle of attack and roll)
	double dum = vbab1 / dvba;
	if(fabs(dum) > 1)
		dum = 1 * sign(dum);
	double alpp = acos(dum);  // Total AOA

	// Aerodynamic roll angle
	if(vbab2 == 0 && vbab3 == 0)
		phip = 0.0;
	else if(fabs(vbab2) < EPS)
		phip = (vbab3 > 0) ? 0 : PI;
	else
		phip = atan2(vbab2, vbab3);

	alppx = alpp * DEG;
	phipx = phip * DEG;

	//**INERTIAL INCIDENCE ANGLES** (diagnostic)
	Matrix VBIB = TBI * VBII;  // Inertial velocity in body frame
	double vbib1 = VBIB.get_loc(0,0);
	double vbib2 = VBIB.get_loc(1,0);
	double vbib3 = VBIB.get_loc(2,0);
	double dvbi = VBIB.absolute();

	double alphai = atan2(vbib3, vbib1);
	double betai = (dvbi > 0.1) ? asin(vbib2 / dvbi) : 0;
	double alphaix = alphai * DEG;
	double betaix = betai * DEG;

	//----------------------------------------------------------------------------
	//loading module-variables
	//time
	vehicle[0].gets(time);
	vehicle[1].gets(event_time);
	//state variables
	vehicle[121].gets_mat(TBI);
	vehicle[122].gets_mat(TBID);
	//output
	vehicle[120].gets_mat(TBD);
	vehicle[123].gets(ortho_error);
	vehicle[134].gets(psibd);
	vehicle[135].gets(thtbd);
	vehicle[136].gets(phibd);
	vehicle[137].gets(psibdx);
	vehicle[138].gets(thtbdx);
	vehicle[139].gets(phibdx);
	vehicle[140].gets(alppx);
	vehicle[141].gets(phipx);
	vehicle[144].gets(alphax);
	vehicle[145].gets(betax);
	vehicle[146].gets(alphaix);
	vehicle[147].gets(betaix);
}
