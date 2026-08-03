///////////////////////////////////////////////////////////////////////////////
// COMPONENT: seeker_perfect
// CATEGORY: Sensors
// DoF: 3DoF / 5DoF / 6DoF
//
// DESCRIPTION:
//   Perfect (ideal) target tracking sensor
//   Provides exact line-of-sight angles and rates to target
//   Use for: Initial guidance development, performance bounds
//
// INPUTS (from vehicle array):
//   vehicle[20] - SAEL - Matrix(3x1) - Missile position in local frame m
//   vehicle[21] - VAEL - Matrix(3x1) - Missile velocity in local frame m/s
//   vehicle[200] - STEL - Matrix(3x1) - Target position in local frame m
//   vehicle[201] - VTEL - Matrix(3x1) - Target velocity in local frame m/s
//
// OUTPUTS (to vehicle array):
//   vehicle[80] - dta - double - Range to target m
//   vehicle[81] - dvta - double - Closing velocity m/s
//   vehicle[82] - tgo - double - Time-to-go sec
//   vehicle[87] - UTAA - Matrix(3x1) - LOS unit vector in missile body
//   vehicle[88] - WOEA - Matrix(3x1) - LOS rate vector in missile body rad/s
//   vehicle[89] - STAL - Matrix(3x1) - Relative position vector m
//
// PARAMETERS (from input.asc):
//   None (perfect sensor, no errors)
//
// DEPENDENCIES:
//   - Requires: Kinematics for own position/velocity
//   - Requires: Target state (from combus or external source)
//   - Provides: Tracking data for guidance
//
// REFERENCE:
//   Ideal seeker with perfect knowledge of target state
//   LOS rate: omega = (r x v_rel) / |r|²
//
// USAGE:
//   Include in class header:
//     virtual void def_seeker();
//     virtual void seeker(double int_step);
//
//   In input.asc MODULES section:
//     seeker   def,exec
//
// NOTES:
//   - This is an idealized sensor for studying guidance performance
//   - Real seekers have: noise, lag, gimbal limits, FOV limits
//   - For realistic seeker, use seeker_radar or seeker_ir components
//   - Target state must be provided externally (from combus or file)
//   - Body frame LOS requires attitude (TAL transformation)
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of seeker module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_seeker()
{
	//Definition of module-variables
	//output to other modules
	vehicle[80].init("dta",0,"Range to target - m","seeker","out","scrn,plot");
	vehicle[81].init("dvta",0,"Closing velocity - m/s","seeker","out","scrn,plot");
	vehicle[82].init("tgo",0,"Time-to-go - sec","seeker","out","scrn,plot");
	vehicle[87].init("UTAA",0,0,0,"LOS unit vector in missile body - ND","seeker","out","");
	vehicle[88].init("WOEA",0,0,0,"LOS rate vector in missile body - rad/s","seeker","out","");
	vehicle[89].init("STAL",0,0,0,"Relative position vector - m","seeker","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Seeker module
//Member function of class 'Vehicle'
//
// Perfect target tracking
// Provides exact LOS angles and rates for guidance
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::seeker(double int_step)
{
	//local module-variables
	double dta(0);
	double dvta(0);
	double tgo(0);
	Matrix UTAA(3,1);
	Matrix WOEA(3,1);
	Matrix STAL(3,1);

	//localizing module-variables
	//input from other modules
	Matrix SAEL=vehicle[20].vec();  // Missile position
	Matrix VAEL=vehicle[21].vec();  // Missile velocity
	Matrix STEL=vehicle[200].vec(); // Target position
	Matrix VTEL=vehicle[201].vec(); // Target velocity

	//-------------------------------------------------------------------------
	//Line-of-sight kinematics

	//Relative position: target (T) wrt missile (A)
	STAL = STEL - SAEL;

	//Range to target
	dta = STAL.absolute();

	//Unit LOS vector (local level frame)
	Matrix UTAL(3,1);
	if(dta > 0.1) {
		double dum = 1.0 / dta;
		UTAL = STAL * dum;
	} else {
		UTAL.zero();
	}

	//Relative velocity: target wrt missile
	Matrix VTAEL = VTEL - VAEL;

	//Closing velocity (positive = closing)
	//vc = -d(range)/dt = -(UTAL · VTAEL)
	dvta = -(UTAL ^ VTAEL);  // ^ is dot product in CADAC Matrix class

	//Time-to-go (approximate, constant velocity assumption)
	if(fabs(dvta) > 0.1) {
		tgo = dta / fabs(dvta);
	} else {
		tgo = 0;
	}

	//LOS rate vector in local level frame
	//omega = (UTAL x VTAEL) / dta
	//This is the inertial LOS rate
	if(dta > 0.1) {
		double dum = 1.0 / dta;
		WOEA = UTAL.skew_sym() * VTAEL * dum;
	} else {
		WOEA.zero();
	}

	//Transform LOS unit vector to missile body frame
	//For 3DoF, assume body frame aligned with velocity (no attitude)
	//For 6DoF, need TAL transformation matrix from attitude module
	//Here we use simplified 3DoF assumption: UTAA ≈ UTAL
	UTAA = UTAL;  // Simplification for 3DoF

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[80].gets(dta);
	vehicle[81].gets(dvta);
	vehicle[82].gets(tgo);
	vehicle[87].gets_vec(UTAA);
	vehicle[88].gets_vec(WOEA);
	vehicle[89].gets_vec(STAL);
}
