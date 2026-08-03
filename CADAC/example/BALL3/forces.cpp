///////////////////////////////////////////////////////////////////////////////
//FILE: 'forces.cpp'
//
// Contains the 'forces' module
// Calculates aerodynamic drag force
//
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of forces module-variables
//Member function of class 'Ball'
///////////////////////////////////////////////////////////////////////////////
void Ball::def_forces()
{
	//Definition of module-variables
	ball[10].init("cd",0,"Drag coefficient - ND","data","","");
	ball[11].init("area",0,"Reference area - m^2","data","","");
	ball[12].init("rho",0,"Air density - kg/m^3","diag","","");
	ball[13].init("dvbe",0,"Speed - m/s","out","scrn","plot");
	ball[14].init("FSPB",0,0,0,"Specific force in body frame - m/s^2","out","","");
}

///////////////////////////////////////////////////////////////////////////////
//Forces module
//Calculates drag force
//Member function of class 'Ball'
///////////////////////////////////////////////////////////////////////////////
void Ball::forces(double int_step)
{
	//local module-variables
	Matrix FSPB(3,1);

	//localizing module-variables
	//input data
	double cd=ball[10].real();
	double area=ball[11].real();
	//input from other modules
	double grav=ball[0].real();
	Matrix VBEL=ball[20].vec();

	//-------------------------------------------------------------------------
	//Air density (simple constant for low altitude)
	double rho = 1.225; // kg/m^3 at sea level

	//Speed
	double dvbe = VBEL.absolute();

	//Drag force per unit mass (specific force)
	//F_drag = 0.5 * rho * v^2 * cd * area
	//a_drag = F_drag / mass
	//Since we track specific force, and drag opposes velocity:
	double drag_mag = 0.5 * rho * dvbe * dvbe * cd * area;

	Matrix VBEL_unit(3,1);
	if(dvbe > 0.1) {
		VBEL_unit = VBEL * (1.0/dvbe); //unit vector in velocity direction
	} else {
		VBEL_unit.zero();
	}

	//Specific force (includes both gravity and drag)
	//Note: In Earth frame, gravity is downward (negative Z)
	//Drag opposes velocity
	FSPB[0] = -drag_mag * VBEL_unit[0]; // Drag in X
	FSPB[1] = -drag_mag * VBEL_unit[1]; // Drag in Y
	FSPB[2] = -drag_mag * VBEL_unit[2] - grav; // Drag in Z + gravity (down)

	//loading module-variables
	//output to other modules
	ball[12].gets(rho);
	ball[13].gets(dvbe);
	ball[14].gets_vec(FSPB);
}
