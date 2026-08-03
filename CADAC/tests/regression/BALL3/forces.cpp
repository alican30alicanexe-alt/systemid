// Merged forces module
#include "class_hierarchy.hpp"

void Ball::def_forces()
{
    // Variable definitions from forces_3dof and drag_simple
    ball[10].init("cd",0,"Drag coefficient - ND","data","","");
    ball[11].init("area",0,"Reference area - m^2","data","","");
    ball[13].init("dvbe",0,"Speed - m/s","out","scrn","plot");
    ball[14].init("FSPA",0,0,0,"Specific force in body frame - m/s^2","forces","out","");
}

void Ball::forces(double int_step)
{
    // Read parameters
    double cd=ball[10].real();
    double area=ball[11].real();
    double grav=ball[0].real();
    double rho=ball[12].real();

    // Read velocity
    Matrix VBEL=ball[21].vec();

    // Calculate speed
    double dvbe=VBEL.absolute();
    ball[13].gets(dvbe);

    // Calculate drag force
    Matrix FSPB(3,1);
    if(dvbe>0.01) {
        double q=0.5*rho*dvbe*dvbe; // dynamic pressure
        Matrix drag_dir = VBEL * (-1.0/dvbe); // opposite to velocity
        Matrix FDRAG = drag_dir * (q * cd * area / 1.0); // assuming unit mass
        FSPB = FDRAG;
    }

    // Add gravity
    FSPB[2] = FSPB[2] + grav; // down direction

    ball[14].gets_vec(FSPB);
}
