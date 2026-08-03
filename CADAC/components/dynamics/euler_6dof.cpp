///////////////////////////////////////////////////////////////////////////////
// COMPONENT: euler_6dof
// CATEGORY: Dynamics
// DoF: 6DoF
//
// DESCRIPTION:
//   6-DoF rotational dynamics using Euler's equation (M = Iα + ω × Iω)
//   Integrates angular velocity with gyroscopic coupling
//   Handles Earth rotation transformation
//   Use for: Rockets, missiles, aircraft in 6DoF
//
// INPUTS (from vehicle array):
//   vehicle[121] - TBI - Matrix(3x3) - DCM body to inertial
//   vehicle[201] - FMB - Matrix(3x1) - Applied moment in body frame N·m
//   vehicle[18] - IBBB - Matrix(3x3) - Inertia tensor body frame kg·m²
//
// OUTPUTS (to vehicle array):
//   vehicle[160] - ppx - double - Roll rate deg/s
//   vehicle[161] - qqx - double - Pitch rate deg/s
//   vehicle[162] - rrx - double - Yaw rate deg/s
//   vehicle[163] - WBEB - Matrix(3x1) - Body rates wrt Earth rad/s
//   vehicle[164] - WBIB - Matrix(3x1) - Body rates wrt inertial rad/s (state)
//   vehicle[165] - WBIBD - Matrix(3x1) - Angular acceleration rad/s² (state)
//   vehicle[166] - WBII - Matrix(3x1) - Angular velocity in inertial frame rad/s
//
// PARAMETERS (from input.asc):
//   vehicle[160] - ppx - double - Initial roll rate deg/s
//   vehicle[161] - qqx - double - Initial pitch rate deg/s
//   vehicle[162] - rrx - double - Initial yaw rate deg/s
//
// DEPENDENCIES:
//   - Requires: Forces module for FMB
//   - Requires: Kinematics module for TBI
//   - Requires: Propulsion module for IBBB
//   - Provides: WBIB for kinematics module
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 5: Rotational Dynamics
//
//   Euler's rotational equation:
//     WBIB_dot = IBBB^-1 * (FMB - WBIB × IBBB * WBIB)
//
//   where:
//     WBIB × IBBB * WBIB is the gyroscopic coupling term
//     (ω × Iω represents centrifugal stiffening)
//
//   Coordinate frame transformations:
//     WBIB = WBEB + TBI * WEII  (add Earth rotation)
//     WBEB = WBIB - TBI * WEII  (remove Earth rotation)
//
// USAGE:
//   Include in class header:
//     virtual void def_euler();
//     virtual void init_euler();
//     virtual void euler(double int_step);
//
//   In input.asc MODULES section:
//     euler   def,init,exec
//
//   In input.asc DATA section:
//     ppx = 0   // Initial roll rate, deg/s
//     qqx = 0   // Initial pitch rate, deg/s
//     rrx = 0   // Initial yaw rate, deg/s
//
// NOTES:
//   - Uses body rates wrt inertial frame (WBIB) as state variables
//   - Gyroscopic coupling term (ω × Iω) is critical for spinning vehicles
//   - Earth rotation (WEII3 = 7.292115e-5 rad/s) added for inertial frame
//   - Body rates wrt Earth (WBEB) used for display and control
//   - Inertia tensor IBBB must be positive definite for stable integration
//   - Skew-symmetric cross product: a × b = skew(a) * b
//   - For non-spinning vehicles (low rotation), gyroscopic term is small
//   - Requires integration timestep ~0.001s for accurate dynamics
//   - Matrix inversion of IBBB computed each timestep (usually diagonal)
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of euler module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_euler()
{
	//Definition and initialization of module-variables
	vehicle[160].init("ppx",0,"Body roll angular vel wrt Earth in body axes - deg/s","euler","out","plot");
	vehicle[161].init("qqx",0,"Body pitch angular vel wrt Earth in body axes - deg/s","euler","out","plot");
	vehicle[162].init("rrx",0,"Body yaw angular vel wrt Earth in body axes - deg/s","euler","out","plot");
	vehicle[163].init("WBEB",0,0,0,"Ang vel of veh wrt Earth, body axes - rad/s","euler","diag","");
	vehicle[164].init("WBIB",0,0,0,"Ang vel of veh wrt inertial frame, body axes - rad/s","euler","state","");
	vehicle[165].init("WBIBD",0,0,0,"Ang vel of veh wrt inertl frame, deriv - rad/s^2","euler","state","");
	vehicle[166].init("WBII",0,0,0,"Vehicle's inertial angular vel in inertial coord - rad/s^2","euler","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Euler initialization module
//Member function of class 'Vehicle'
//
// Converts initial body rates wrt Earth to inertial frame
//
// 030424 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_euler()
{
	//local variable
	Matrix WEII(3,1);

	//local module-variables
	Matrix WBEB(3,1);
	Matrix WBIB(3,1);

	//localizing module-variables
	//input data
	double ppx=vehicle[160].real();  // Roll rate
	double qqx=vehicle[161].real();  // Pitch rate
	double rrx=vehicle[162].real();  // Yaw rate
	//input from other modules
	Matrix TBI=vehicle[121].mat();   // DCM from kinematics
	//-------------------------------------------------------------------------
	//Body rate wrt Earth frame in body coordinates
	WBEB.build_vec3(ppx*RAD, qqx*RAD, rrx*RAD);

	//Body rate wrt inertial frame in body coordinates
	//WBIB = WBEB + TBI * WEII (add Earth rotation contribution)
	WEII.build_vec3(0, 0, WEII3);  // Earth rotation vector (0,0,7.292115e-5)
	WBIB = WBEB + TBI * WEII;
	//-------------------------------------------------------------------------
	//loading module-variables
	//initialization
	vehicle[164].gets_vec(WBIB);
}

///////////////////////////////////////////////////////////////////////////////
//Euler module
//Member function of class 'Vehicle'
//
// Integrates angular velocity using Euler's rotational equation
// Handles gyroscopic coupling and Earth rotation
//
// 030424 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::euler(double int_step)
{
	//local variable
	Matrix WEII(3,1);

	//local module-variables
	double ppx(0), qqx(0), rrx(0);
	Matrix WBEB(3,1);
	Matrix WBII(3,1);

	//localizing module-variables
	//input from other modules
	Matrix FMB=vehicle[201].vec();   // Applied moment from forces module
	Matrix TBI=vehicle[121].mat();   // DCM from kinematics
	Matrix IBBB=vehicle[18].mat();   // Inertia tensor from propulsion
	//state variable
	Matrix WBIB=vehicle[164].vec();  // Angular velocity (state)
	Matrix WBIBD=vehicle[165].vec(); // Angular acceleration (state derivative)
	//----------------------------------------------------------------------------
	//**INTEGRATE EULER'S ROTATIONAL EQUATION**
	//
	// Euler equation: I * α = M - ω × I * ω
	// Solving for α: α = I^-1 * (M - ω × I * ω)
	//
	// where:
	//   α = WBIB_dot (angular acceleration)
	//   M = FMB (applied moment)
	//   ω = WBIB (angular velocity)
	//   I = IBBB (inertia tensor)
	//   ω × I*ω is gyroscopic coupling (centrifugal stiffening)

	Matrix WACC_NEXT = IBBB.inverse() * (FMB - WBIB.skew_sym() * IBBB * WBIB);
	WBIB = integrate(WACC_NEXT, WBIBD, WBIB, int_step);
	WBIBD = WACC_NEXT;

	//Angular velocity wrt inertial frame in inertial coordinates
	WBII = ~TBI * WBIB;

	//Angular velocity wrt Earth in body coordinates
	//(remove Earth rotation for display and control)
	WEII.build_vec3(0, 0, WEII3);
	WBEB = WBIB - TBI * WEII;

	//Body rates in deg/s for display
	ppx = WBEB.get_loc(0,0) * DEG;  // Roll rate
	qqx = WBEB.get_loc(1,0) * DEG;  // Pitch rate
	rrx = WBEB.get_loc(2,0) * DEG;  // Yaw rate
	//----------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[164].gets_vec(WBIB);
	vehicle[165].gets_vec(WBIBD);
	//output to other modules
	vehicle[160].gets(ppx);
	vehicle[161].gets(qqx);
	vehicle[162].gets(rrx);
	vehicle[163].gets_vec(WBEB);
	vehicle[166].gets_vec(WBII);
}
