///////////////////////////////////////////////////////////////////////////////
//FILE: 'kinematics.cpp'
//
// Contains the 'kinematics' module
// Integrates position and velocity
//
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of kinematics module-variables
//Member function of class 'Ball'
///////////////////////////////////////////////////////////////////////////////
void Ball::def_kinematics()
{
	//Definition of module-variables
	ball[20].init("SBEL",0,0,0,"Position in Earth frame - m","state","","plot");
	ball[21].init("VBEL",0,0,0,"Velocity in Earth frame - m/s","state","","plot");
	ball[22].init("altitude",0,"Altitude above ground - m","out","scrn","plot");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of kinematics module
//Member function of class 'Ball'
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

	//input data (for integration)
	ball[23].gets(sbel1);
	ball[24].gets(sbel2);
	ball[25].gets(sbel3);
	ball[26].gets(vbel1);
	ball[27].gets(vbel2);
	ball[28].gets(vbel3);
}

///////////////////////////////////////////////////////////////////////////////
//Kinematics module
//Member function of class 'Ball'
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
