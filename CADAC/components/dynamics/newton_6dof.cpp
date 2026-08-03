///////////////////////////////////////////////////////////////////////////////
// COMPONENT: newton_6dof
// CATEGORY: Dynamics
// DoF: 6DoF
//
// DESCRIPTION:
//   6-DoF translational dynamics using Newton's 2nd Law (F = ma)
//   Integrates acceleration, velocity, and position in inertial frame
//   Handles geodetic/inertial coordinate transformations with WGS84
//   Use for: Rockets, missiles, aircraft in 6DoF
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[62] - GRAVG - Matrix(3x1) - Gravity in geocentric frame m/s²
//   vehicle[121] - TBI - Matrix(3x3) - DCM body to inertial
//   vehicle[200] - FAPB - Matrix(3x1) - Applied force in body frame N
//   vehicle[15] - vmass - double - Vehicle mass kg
//   vehicle[503] - mfreeze - int - Freeze flag for autopilot testing
//
// OUTPUTS (to vehicle array):
//   vehicle[219] - lonx - double - Longitude deg
//   vehicle[220] - latx - double - Latitude deg
//   vehicle[221] - alt - double - Altitude m
//   vehicle[222] - TVD - Matrix(3x3) - DCM velocity to geodetic
//   vehicle[223] - TDI - Matrix(3x3) - DCM geodetic to inertial
//   vehicle[225] - dvbe - double - Geographic speed m/s
//   vehicle[226] - dvbi - double - Inertial speed m/s
//   vehicle[227] - WEII - Matrix(3x3) - Earth angular velocity (skew-sym) rad/s
//   vehicle[228] - psivdx - double - Heading angle deg
//   vehicle[229] - thtvdx - double - Flight path angle deg
//   vehicle[230] - dbi - double - Distance from Earth center m
//   vehicle[231] - TGI - Matrix(3x3) - DCM geocentric to inertial
//   vehicle[232] - VBED - Matrix(3x1) - Geographic velocity in geodetic m/s
//   vehicle[234] - altx - double - Altitude kft (diagnostic)
//   vehicle[235] - SBII - Matrix(3x1) - Inertial position m (state)
//   vehicle[236] - VBII - Matrix(3x1) - Inertial velocity m/s (state)
//   vehicle[237] - ABII - Matrix(3x1) - Inertial acceleration m/s² (state)
//   vehicle[238] - grndtrck - double - Ground track distance m
//   vehicle[239] - FSPB - Matrix(3x1) - Specific force in body m/s²
//   vehicle[240] - ayx - double - Side acceleration g
//   vehicle[241] - anx - double - Normal acceleration g
//   vehicle[242] - gndtrkmx - double - Ground track km
//   vehicle[243] - gndtrnmx - double - Ground track nm
//
// PARAMETERS (from input.asc):
//   vehicle[217] - alpha0x - double - Initial angle of attack deg
//   vehicle[218] - beta0x - double - Initial sideslip angle deg
//   vehicle[219] - lonx - double - Initial longitude deg
//   vehicle[220] - latx - double - Initial latitude deg
//   vehicle[221] - alt - double - Initial altitude m
//   vehicle[225] - dvbe - double - Initial speed m/s
//   vehicle[137] - psibdx - double - Initial yaw deg
//   vehicle[138] - thtbdx - double - Initial pitch deg
//   vehicle[139] - phibdx - double - Initial roll deg
//
// DEPENDENCIES:
//   - Requires: Forces module for FAPB
//   - Requires: Propulsion module for vmass
//   - Requires: Kinematics module for TBI
//   - Requires: Environment module for GRAVG
//   - Requires: CADAC utilities (cad_in_geo84, cad_geo84_in, cad_tdi84,
//               cad_tgi84, mat2tr, mat3tr)
//   - Provides: Position and velocity for all modules
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 4: Translational Dynamics
//
//   Newton's 2nd Law in inertial frame:
//     VBII_dot = TBI^T * (FAPB/m) + TGI^T * GRAVG
//     SBII_dot = VBII
//
//   Geographic velocity (removes Earth rotation):
//     VBED = TDI * (VBII - WEII * SBII)
//
// USAGE:
//   Include in class header:
//     virtual void def_newton();
//     virtual void init_newton();
//     virtual void newton(double int_step);
//
//   In input.asc MODULES section:
//     newton   def,init,exec
//
//   In input.asc DATA section:
//     lonx = -80.5   // Initial longitude, deg
//     latx = 28.5    // Initial latitude (Cape Canaveral), deg
//     alt = 0        // Initial altitude, m
//     dvbe = 1       // Initial speed, m/s
//     alpha0x = 90   // Initial AOA (vertical launch), deg
//     beta0x = 0     // Initial sideslip, deg
//     psibdx = 0     // Initial yaw, deg
//     thtbdx = 90    // Initial pitch (vertical), deg
//     phibdx = 0     // Initial roll, deg
//
// NOTES:
//   - Uses WGS84 ellipsoid for Earth shape and gravity
//   - Integrates in inertial (ECI) frame to avoid fictitious forces
//   - Geodetic coordinates (lon/lat/alt) computed from SBII each timestep
//   - WEII is Earth rotation skew-symmetric matrix (7.292115e-5 rad/s)
//   - Ground track computed with ~10% accuracy (slightly high)
//   - mfreeze flag allows freezing speed for autopilot gain tuning
//   - Flight path angles (psivdx, thtvdx) relative to local geodetic frame
//   - Specific force FSPB = FAPB/mass is what accelerometers measure
//   - Requires small integration timestep (~0.001s) for accurate trajectory
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of newton module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_newton()
{
	//Definition of module-variables
	vehicle[217].init("alpha0x",0,"Initial angle-of-attack - deg","newton","data","");
	vehicle[218].init("beta0x",0,"Initial sideslip angle - deg","newton","data","");
	vehicle[219].init("lonx",0,"Vehicle longitude - deg","newton","init,diag","scrn,plot,com");
	vehicle[220].init("latx",0,"Vehicle latitude - deg","newton","init,diag","scrn,plot,com");
	vehicle[221].init("alt",0,"Vehicle altitude - m","newton","init,out","scrn,plot,com");
	vehicle[222].init("TVD",0,0,0,0,0,0,0,0,0,"TM of geo velocity wrt geodetic coord - ND ","newton","out","");
	vehicle[223].init("TDI",0,0,0,0,0,0,0,0,0,"TM of geodetic wrt inertial  coordinates ","newton","init","");
	vehicle[225].init("dvbe",0,"Vehicle geographic speed - m/s","newton","init,out","scrn,plot,com");
	vehicle[226].init("dvbi",0,"Vehicle inertial speed - m/s","newton","out","scrn,plot,com");
	vehicle[227].init("WEII",0,0,0,0,0,0,0,0,0,"Earth's angular velocity (skew-sym) - rad/s ","newton","init","");
	vehicle[228].init("psivdx",0,"Vehicle heading angle - deg","newton","init,out","scrn,plot,com");
	vehicle[229].init("thtvdx",0,"Vehicle flight path angle - deg","newton","init,out","scrn,plot,com");
	vehicle[230].init("dbi",0,"Vehicle distance from center of Earth - m","newton","out","");
	vehicle[231].init("TGI",0,0,0,0,0,0,0,0,0,"TM of geocentric wrt inertial  coordinates ","newton","init","");
	vehicle[232].init("VBED",0,0,0,"Geographic velocity in geodetic coord - m/s ","newton","out","");
	vehicle[234].init("altx",0,"Vehicle altitude - kft","newton","diag","");
	vehicle[235].init("SBII",0,0,0,"Inertial position - m ","newton","state","com");
	vehicle[236].init("VBII",0,0,0,"Inertial velocity - m/s ","newton","state","com");
	vehicle[237].init("ABII",0,0,0,"Inertial acceleration - m/s^2 ","newton","save","");
	vehicle[238].init("grndtrck",0,"Vehicle ground track on Earth surface - m","newton","diag","plot");
	vehicle[239].init("FSPB",0,0,0,"Specific force in body coord - m/s^2 ","newton","out","scrn");
	vehicle[240].init("ayx",0,"Achieved side acceleration - g's","newton","diag","plot");
	vehicle[241].init("anx",0,"Achieved normal acceleration - g's","newton","diag","plot");
	vehicle[242].init("gndtrkmx",0,"Ground track - km","newton","diag","");
	vehicle[243].init("gndtrnmx",0,"Ground track - nm","newton","diag","plot");
	vehicle[247].init("mfreeze_newt","int",0,"Saving mfreeze value - ND","newton","save","");
	vehicle[248].init("dvbef",0,"Saved speed when mfreeze=1 - m/s","newton","save","");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of newton module
//Member function of class 'Vehicle'
//
// Converts geodetic initial conditions to inertial position and velocity
// Initializes Earth rotation and transformation matrices
//
// 030410 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_newton()
{
	//local variables
	Matrix VBEB(3,1);
	Matrix POLAR(3,1);

	//local module-variables
	Matrix WBII(3,3);
	double dbi(0);
	Matrix VBED(3,1);
	Matrix SBII(3,1);
	Matrix VBII(3,1);
	double dvbi(0);
	double psivdx(0);
	double thtvdx(0);

	//localizing module-variables
	//input data
	double dvbe=vehicle[225].real();
	double lonx=vehicle[219].real();
	double latx=vehicle[220].real();
	double alt=vehicle[221].real();
	//input from other modules
	double time=vehicle[0].real();
	double psibdx=vehicle[137].real();
	double thtbdx=vehicle[138].real();
	double phibdx=vehicle[139].real();
	double alpha0x=vehicle[217].real();
	double beta0x=vehicle[218].real();
	//-------------------------------------------------------------------------
	//Earth's angular velocity skew-symmetric matrix (3x3)
	//WEII3 = 7.292115e-5 rad/s (Earth rotation rate)
	Matrix WEII(3,3);
	WEII.assign_loc(0,1,-WEII3);
	WEII.assign_loc(1,0,WEII3);

	//Converting geodetic (lon, lat, alt) to inertial position SBII
	SBII=cad_in_geo84(lonx*RAD, latx*RAD, alt, time);
	dbi=SBII.absolute();

	//Building geodetic velocity VBED from alpha, beta, and dvbe
	//Start with body frame velocity from incidence angles
	double salp=sin(alpha0x*RAD);
	double calp=cos(alpha0x*RAD);
	double sbet=sin(beta0x*RAD);
	double cbet=cos(beta0x*RAD);
	double vbeb1=calp*cbet*dvbe;  // X (forward)
	double vbeb2=sbet*dvbe;        // Y (right)
	double vbeb3=salp*cbet*dvbe;  // Z (down)
	VBEB.build_vec3(vbeb1,vbeb2,vbeb3);

	//Transform body velocity to geodetic frame
	Matrix TBD=mat3tr(psibdx*RAD, thtbdx*RAD, phibdx*RAD);
	VBED=~TBD*VBEB;

	//Building inertial velocity VBII
	//VBII = TDI^T * VBED + WEII * SBII
	//(add Earth rotation contribution)
	Matrix TDI=cad_tdi84(lonx*RAD, latx*RAD, alt, time);
	Matrix TGI=cad_tgi84(lonx*RAD, latx*RAD, alt, time);
	VBII=~TDI*VBED + WEII*SBII;
	dvbi=VBII.absolute();

	//Calculating geodetic flight path angles (for plotting initialization)
	POLAR=VBED.pol_from_cart();
	psivdx=DEG*POLAR.get_loc(1,0);  // Heading
	thtvdx=DEG*POLAR.get_loc(2,0);  // Flight path angle
	//-------------------------------------------------------------------------
	//loading module-variables
	vehicle[219].gets(lonx);
	vehicle[220].gets(latx);
	vehicle[223].gets_mat(TDI);
	vehicle[226].gets(dvbi);
	vehicle[227].gets_mat(WEII);
	vehicle[228].gets(psivdx);
	vehicle[229].gets(thtvdx);
	vehicle[230].gets(dbi);
	vehicle[231].gets_mat(TGI);
	vehicle[232].gets_vec(VBED);
	vehicle[235].gets_vec(SBII);
	vehicle[236].gets_vec(VBII);
	//output to other module
	vehicle[137].gets(psibdx);
}

///////////////////////////////////////////////////////////////////////////////
//Newton module
//Member function of class 'Vehicle'
//
// Integrates translational equations of motion using Newton's 2nd Law
// Converts between inertial and geodetic coordinate systems
// Computes flight path parameters
//
// 030410 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::newton(double int_step)
{
	//local variables
	double lon(0);
	double lat(0);

	//local module-variables
	double dvbe(0);
	double dvbi(0);
	double lonx(0);
	double latx(0);
	double alt(0);
	double altx(0);
	double psivdx(0);
	double thtvdx(0);
	Matrix FSPB(3,1);
	double anx(0);
	double ayx(0);
	double dbi(0);
	double gndtrkmx(0);
	double gndtrnmx(0);
	Matrix TVD(3,3);
	Matrix VBED(3,1);

	//localizing module-variables
	//initializations
	Matrix TDI=vehicle[223].mat();
	Matrix TGI=vehicle[231].mat();
	Matrix WEII=vehicle[227].mat();
	//input data
	//getting saved values
	double grndtrck=vehicle[238].real();
	int mfreeze_newt=vehicle[247].integer();
	double dvbef=vehicle[248].real();
	//state variables
	Matrix SBII=vehicle[235].vec();
	Matrix VBII=vehicle[236].vec();
	Matrix ABII=vehicle[237].vec();
	//input from other modules
	double time=vehicle[0].real();
	Matrix GRAVG=vehicle[62].vec();      // Gravity from environment
	Matrix TBI=vehicle[121].mat();       // DCM from kinematics
	Matrix FAPB=vehicle[200].vec();      // Applied force from forces module
	double vmass=vehicle[15].real();     // Mass from propulsion
	int mfreeze=vehicle[503].integer();  // Freeze flag for autopilot testing
	//----------------------------------------------------------------------------
	//**INTEGRATE NEWTON'S 2ND LAW IN INERTIAL FRAME**
	//
	// Specific force (what accelerometers measure)
	FSPB = FAPB * (1.0/vmass);

	// Acceleration in inertial frame:
	// ABII = TBI^T * FSPB + TGI^T * GRAVG
	// (transform body force to inertial, add gravity in geocentric frame)
	Matrix NEXT_ACC = ~TBI * FSPB + ~TGI * GRAVG;

	// Integrate velocity and position
	Matrix NEXT_VEL = integrate(NEXT_ACC, ABII, VBII, int_step);
	SBII = integrate(NEXT_VEL, VBII, SBII, int_step);
	ABII = NEXT_ACC;
	VBII = NEXT_VEL;

	dvbi = VBII.absolute();
	dbi = SBII.absolute();

	//----------------------------------------------------------------------------
	//**CONVERT INERTIAL TO GEODETIC COORDINATES**
	//
	// Convert SBII to geodetic longitude, latitude, altitude
	cad_geo84_in(lon, lat, alt, SBII, time);

	// Update transformation matrices at new position
	TDI = cad_tdi84(lon, lat, alt, time);
	TGI = cad_tgi84(lon, lat, alt, time);

	lonx = lon * DEG;
	latx = lat * DEG;

	// Altitude in kft for diagnostics
	altx = 0.001 * alt * FOOT;

	//----------------------------------------------------------------------------
	//**GEOGRAPHIC VELOCITY AND FLIGHT PATH ANGLES**
	//
	// Remove Earth rotation to get geographic velocity:
	// VBED = TDI * (VBII - WEII * SBII)
	VBED = TDI * (VBII - WEII * SBII);

	// Convert to polar coordinates for flight path angles
	Matrix POLAR = VBED.pol_from_cart();
	dvbe = POLAR[0];       // Geographic speed
	psivdx = DEG*POLAR[1]; // Heading angle
	thtvdx = DEG*POLAR[2]; // Flight path angle

	// Transformation matrix from velocity to geodetic frame
	TVD = mat2tr(psivdx*RAD, thtvdx*RAD);

	//----------------------------------------------------------------------------
	//**DIAGNOSTICS**

	// Acceleration achieved (in g's)
	ayx = FSPB[1] / AGRAV;  // Side acceleration
	anx = -FSPB[2] / AGRAV; // Normal acceleration

	// Ground track travelled (10% accuracy, usually on high side)
	double vbed1 = VBED[0];
	double vbed2 = VBED[1];
	grndtrck += sqrt(vbed1*vbed1 + vbed2*vbed2) * int_step * REARTH / dbi;
	gndtrkmx = 0.001 * grndtrck;      // km
	gndtrnmx = NMILES * grndtrck;     // nautical miles

	// Freeze variables for autopilot response calculations
	if(mfreeze == 0)
		mfreeze_newt = 0;
	else {
		if(mfreeze != mfreeze_newt) {
			mfreeze_newt = mfreeze;
			dvbef = dvbe;
		}
		dvbe = dvbef;
	}
	//----------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[235].gets_vec(SBII);
	vehicle[236].gets_vec(VBII);
	vehicle[237].gets_vec(ABII);
	//saving values
	vehicle[238].gets(grndtrck);
	vehicle[247].gets(mfreeze_newt);
	vehicle[248].gets(dvbef);
	//output to other modules
	vehicle[219].gets(lonx);
	vehicle[220].gets(latx);
	vehicle[221].gets(alt);
	vehicle[222].gets_mat(TVD);
	vehicle[223].gets_mat(TDI);
	vehicle[225].gets(dvbe);
	vehicle[226].gets(dvbi);
	vehicle[231].gets_mat(TGI);
	vehicle[232].gets_vec(VBED);
	vehicle[239].gets_vec(FSPB);
	//diagnostics
	vehicle[228].gets(psivdx);
	vehicle[229].gets(thtvdx);
	vehicle[230].gets(dbi);
	vehicle[234].gets(altx);
	vehicle[240].gets(ayx);
	vehicle[241].gets(anx);
	vehicle[242].gets(gndtrkmx);
	vehicle[243].gets(gndtrnmx);
}
