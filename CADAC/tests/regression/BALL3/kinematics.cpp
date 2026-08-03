///////////////////////////////////////////////////////////////////////////////
// COMPONENT: kinematics_3dof_flat
// CATEGORY: Kinematics
// DoF: 3DoF
//
// DESCRIPTION:
//   Point mass kinematics on flat Earth
//   Integrates position and velocity in Earth-fixed reference frame
//   Use for: Ballistics, short-range missiles, artillery
//
// INPUTS (from vehicle array):
//   ball[14] - FSPB - Matrix(3x1) - Specific force (acceleration) m/s^2
//
// OUTPUTS (to vehicle array):
//   ball[20] - SBEL - Matrix(3x1) - Position in Earth frame m
//   ball[21] - VBEL - Matrix(3x1) - Velocity in Earth frame m/s
//   ball[22] - altitude - double - Altitude above ground m
//
// PARAMETERS (from input.asc):
//   ball[23] - sbel1 - double - Initial X position m
//   ball[24] - sbel2 - double - Initial Y position m
//   ball[25] - sbel3 - double - Initial Z position m (down is positive)
//   ball[26] - vbel1 - double - Initial X velocity m/s
//   ball[27] - vbel2 - double - Initial Y velocity m/s
//   ball[28] - vbel3 - double - Initial Z velocity m/s
//
// DEPENDENCIES:
//   - Requires: Forces/dynamics module to compute FSPB (ball[14])
//   - Provides: Position and velocity for other modules
//
// REFERENCE:
//   Standard Newtonian integration in Earth frame
//   Flat Earth approximation (valid for ranges < 50 km)
//
// USAGE:
//   Include in class header:
//     virtual void def_kinematics();
//     virtual void init_kinematics();
//     virtual void kinematics(double int_step);
//
//   In input.asc MODULES section:
//     kinematics   def,init,exec
//
//   In input.asc DATA section:
//     sbel1 = 0      // Initial position X, m
//     sbel2 = 0      // Initial position Y, m
//     sbel3 = 0      // Initial position Z (negative altitude), m
//     vbel1 = 35.36  // Initial velocity X, m/s
//     vbel2 = 0      // Initial velocity Y, m/s
//     vbel3 = -35.36 // Initial velocity Z (upward), m/s
//
// NOTES:
//   - Earth frame: X=East, Y=North, Z=Down
//   - Altitude = -sbel3 (Z is down, altitude is up)
//   - Ground impact detection included (stops at altitude=0)
//   - Uses simple Euler integration: x' = x + v*dt, v' = v + a*dt
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of kinematics module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Ball::def_kinematics()
{
	//Definition of module-variables
	//state variables
	ball[20].init("SBEL",0,0,0,"Position in Earth frame - m","state","","plot");
	ball[21].init("VBEL",0,0,0,"Velocity in Earth frame - m/s","state","","plot");
	//output data
	ball[22].init("altitude",0,"Altitude above ground - m","out","scrn","plot");
	//input data
	ball[23].init("sbel1",0,"Initial X position - m","data","","");
	ball[24].init("sbel2",0,"Initial Y position - m","data","","");
	ball[25].init("sbel3",0,"Initial Z position (down) - m","data","","");
	ball[26].init("vbel1",0,"Initial X velocity - m/s","data","","");
	ball[27].init("vbel2",0,"Initial Y velocity - m/s","data","","");
	ball[28].init("vbel3",0,"Initial Z velocity (down) - m/s","data","","");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of kinematics module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Ball::init_kinematics()
{
	//local module-variables
	Matrix SBEL(3,1);
	Matrix VBEL(3,1);

	//input data
	double sbel1=ball[23].real();
	double sbel2=ball[24].real();
	double sbel3=ball[25].real();
	double vbel1=ball[26].real();
	double vbel2=ball[27].real();
	double vbel3=ball[28].real();

	//-------------------------------------------------------------------------
	//Build initial state vectors
	SBEL.build_vec3(sbel1, sbel2, sbel3);
	VBEL.build_vec3(vbel1, vbel2, vbel3);

	//Calculate altitude
	double altitude = -sbel3; // Negative because Z is down

	//-------------------------------------------------------------------------
	//loading module-variables
	//state variables
	ball[20].gets_vec(SBEL);
	ball[21].gets_vec(VBEL);
	//output
	ball[22].gets(altitude);
}

///////////////////////////////////////////////////////////////////////////////
//Kinematics module
//Member function of class 'Vehicle'
//
// Integrates Newton's equations of motion for point mass on flat Earth
//
// 030424 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Ball::kinematics(double int_step)
{
	//local module-variables
	Matrix VBEL(3,1);
	Matrix SBEL(3,1);
	Matrix FSPB(3,1); //specific force (acceleration)

	//localizing module-variables
	//input from other modules
	FSPB=ball[14].vec();
	//state variables
	VBEL=ball[21].vec();
	SBEL=ball[20].vec();

	//-------------------------------------------------------------------------
	//Integration
	//Acceleration from specific force
	Matrix ABEL = FSPB; // acceleration = specific force

	//Integrate velocity: v = v0 + a*dt
	VBEL = VBEL + ABEL * int_step;

	//Integrate position: s = s0 + v*dt
	SBEL = SBEL + VBEL * int_step;

	//Calculate altitude (Z is down, so altitude is -Z)
	double altitude = -SBEL[2];

	//Ground impact detection
	if(altitude <= 0) {
		altitude = 0;
		SBEL[2] = 0;
		VBEL.zero(); //Stop at ground
	}

	//-------------------------------------------------------------------------
	//loading module-variables
	//state variables
	ball[20].gets_vec(SBEL);
	ball[21].gets_vec(VBEL);
	//output
	ball[22].gets(altitude);
}
