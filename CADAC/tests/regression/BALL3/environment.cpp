// Merged environment module
#include "class_hierarchy.hpp"

void Ball::def_environment()
{
    // From gravity_constant
    ball[0].init("grav","D",0,"Gravity acceleration - m/s^2","environment","out","");

    // From atmosphere_constant
    ball[12].init("rho",0,"Air density - kg/m^3","out","","");
}

void Ball::environment(double int_step)
{
    // From gravity_constant and atmosphere_constant
    double grav=9.81; // constant gravity
    double rho=1.225; // constant atmosphere

    ball[0].gets(grav);
    ball[12].gets(rho);
}
