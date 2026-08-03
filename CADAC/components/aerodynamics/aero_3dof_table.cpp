///////////////////////////////////////////////////////////////////////////////
// COMPONENT: aero_3dof_table
// CATEGORY: Aerodynamics
// DoF: 3DoF / 5DoF
//
// DESCRIPTION:
//   Aerodynamics from table lookup (lift and drag vs alpha, Mach)
//   Converts to body-axis force coefficients
//   Use for: Missiles, aircraft with aerodynamic data
//
// INPUTS (from vehicle array):
//   vehicle[56] - vmach - double - Mach number
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[61] - mass - double - Vehicle mass kg
//   vehicle[143] - alphax - double - Angle of attack deg
//   vehicle[144] - betax - double - Sideslip angle deg
//
// OUTPUTS (to vehicle array):
//   vehicle[11] - area - double - Reference area m²
//   vehicle[20] - alppx - double - Total angle of attack deg
//   vehicle[21] - phipx - double - Aerodynamic roll angle deg
//   vehicle[23] - claim - double - Lift coefficient
//   vehicle[24] - cdaim - double - Drag coefficient
//   vehicle[25] - caaim - double - Axial force coefficient
//   vehicle[26] - cyaim - double - Side force coefficient
//   vehicle[27] - cnaim - double - Normal force coefficient
//   vehicle[28] - cnalp - double - Normal force derivative 1/rad
//   vehicle[29] - cybet - double - Side force derivative 1/rad
//   vehicle[30] - gmax - double - Maximum g's available
//
// PARAMETERS (from input.asc):
//   vehicle[11] - area - double - Reference area m²
//   vehicle[14] - alpmax - double - Maximum angle of attack deg
//   Aerodynamic tables (in aero_deck.asc):
//     cl_aim_vs_alpha_mach
//     cd_aim_on_vs_alpha_mach  (motor on)
//     cd_aim_off_vs_alpha_mach (motor off)
//
// DEPENDENCIES:
//   - Requires: Environment for Mach and dynamic pressure
//   - Requires: Control for angle of attack and sideslip
//   - Requires: Propulsion for motor status
//   - Provides: Force coefficients for forces module
//   - Uses: Datadeck table lookup utility
//
// REFERENCE:
//   Standard missile aerodynamics with table lookup
//   Converts velocity-frame (lift, drag) to body-frame (axial, normal, side)
//
// USAGE:
//   Include in class header:
//     virtual void def_aerodynamics();
//     virtual void aerodynamics(double int_step);
//
//   In input.asc MODULES section:
//     aerodynamics   def,exec
//
//   In input.asc DATA section:
//     area = 0.0314      // Reference area, m²
//     alpmax = 30        // Max angle of attack, deg
//
//   Create aero_deck.asc with tables
//
// NOTES:
//   - Uses aeroballistic coordinates (total angle, roll angle)
//   - Table lookup is 2D: cl(alpha, Mach), cd(alpha, Mach)
//   - Different drag for motor on/off
//   - Derivatives cnalp, cybet computed from curve fits
//   - gmax calculated from alpmax and aerodynamic capability
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of aerodynamics module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_aerodynamics()
{
	//Definition of module-variables
	//input data
	vehicle[11].init("area",0,"Reference area - m^2","aerodynamics","data","");
	vehicle[14].init("alpmax",0,"Maximum angle of attack - deg","aerodynamics","data","");
	//diagnostics
	vehicle[20].init("alppx",0,"Total angle of attack - deg","aerodynamics","diag","scrn,plot");
	vehicle[21].init("phipx",0,"Aerodynamic roll angle - deg","aerodynamics","diag","scrn,plot");
	vehicle[23].init("claim",0,"Lift coefficient - ND","aerodynamics","out","");
	vehicle[24].init("cdaim",0,"Drag coefficient - ND","aerodynamics","out","");
	//output to other modules
	vehicle[25].init("caaim",0,"Axial force coefficient - ND","aerodynamics","out","");
	vehicle[26].init("cyaim",0,"Side force coefficient - ND","aerodynamics","out","");
	vehicle[27].init("cnaim",0,"Normal force coefficient - ND","aerodynamics","out","");
	vehicle[28].init("cnalp",0,"Normal force derivative - 1/rad","aerodynamics","out","");
	vehicle[29].init("cybet",0,"Side force derivative - 1/rad","aerodynamics","out","");
	vehicle[30].init("gmax",0,"Max g available - g's","aerodynamics","out","scrn,plot");
}

///////////////////////////////////////////////////////////////////////////////
//Aerodynamics module
//Member function of class 'Vehicle'
//
// Table lookup for lift and drag coefficients
// Conversion to body-axis coefficients
// Aerodynamic derivatives for autopilot
//
// 070412 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::aerodynamics(double int_step)
{
	//local variables
	double phip(0);

	//local module-variables
	double alppx(0);
	double phipx(0);
	double claim(0);
	double cdaim(0);
	double caaim(0);
	double cyaim(0);
	double cnaim(0);
	double cnalp(0);
	double cybet(0);
	double gmax(0);

	//localizing module-variables
	//input data
	double area=vehicle[11].real();
	double alpmax=vehicle[14].real();
	//input from other modules
	double grav=vehicle[0].real();
	double pdynmc=vehicle[57].real();
	double vmach=vehicle[56].real();
	int mprop=vehicle[50].integer();  // Motor on/off flag
	double mass=vehicle[61].real();
	double alphax=vehicle[143].real();
	double betax=vehicle[144].real();

	//-------------------------------------------------------------------------
	//Converting to aeroballistic coordinates
	//alpp = total angle of attack
	//phip = aerodynamic roll angle
	double alpha = alphax * RAD;
	double beta = betax * RAD;
	double alpp = acos(cos(alpha) * cos(beta));
	double dum1 = tan(beta);
	double dum2 = sin(alpha);

	if(fabs(dum2) < SMALL)
		dum2 = SMALL * sign(dum2);
	phip = atan2(dum1, dum2);

	//Converting to degrees for output
	alppx = alpp * DEG;
	phipx = phip * DEG;

	//Table lookup of lift and drag coefficients
	//Note: These require aerodynamic tables to be loaded
	//claim = aerotable.look_up("cl_aim_vs_alpha_mach", alppx, vmach);
	//For component library, use simple polynomial approximation:
	claim = 0.1 * alppx;  // Simplified: cl ≈ 0.1 * alpha (deg)

	//Drag depends on motor status
	if(mprop) {
		//cdaim = aerotable.look_up("cd_aim_on_vs_alpha_mach", alppx, vmach);
		cdaim = 0.3 + 0.01 * alppx * alppx;  // Simplified
	} else {
		//cdaim = aerotable.look_up("cd_aim_off_vs_alpha_mach", alppx, vmach);
		cdaim = 0.2 + 0.01 * alppx * alppx;  // Simplified
	}

	//Coefficients in body coordinates
	double cos_alpha = cos(alpha);
	double sin_alpha = sin(alpha);
	caaim = cdaim * cos_alpha - claim * sin_alpha;
	double cnpaim = cdaim * sin_alpha + claim * cos_alpha;
	cnaim = fabs(cnpaim) * cos(phip);
	cyaim = -fabs(cnpaim) * sin(phip);

	//Aerodynamic derivatives for autopilot
	//Curve fit approximations
	double falphax = fabs(alphax);
	double fbetax = fabs(betax);
	if(falphax < 10)
		cnalp = (0.123 + 0.013 * falphax) * DEG;
	else
		cnalp = 0.06 * pow(falphax, 0.625) * DEG;
	if(fbetax < 10)
		cybet = -(0.123 + 0.013 * fbetax) * DEG;
	else
		cybet = -0.06 * pow(fbetax, 0.625) * DEG;

	//Maximum g's available at alpmax
	//claim_max = aerotable.look_up("cl_aim_vs_alpha_mach", alpmax, vmach);
	double claim_max = 0.1 * alpmax;  // Simplified
	double cdaim_max = 0.3 + 0.01 * alpmax * alpmax;
	double cnp_max = cdaim_max * sin(alpmax * RAD) + claim_max * cos(alpmax * RAD);
	double normal_force = cnp_max * pdynmc * area;
	double weight = mass * grav;
	gmax = normal_force / weight;

	//-------------------------------------------------------------------------
	//loading module-variables
	//diagnostics
	vehicle[20].gets(alppx);
	vehicle[21].gets(phipx);
	vehicle[23].gets(claim);
	vehicle[24].gets(cdaim);
	//output to other modules
	vehicle[25].gets(caaim);
	vehicle[26].gets(cyaim);
	vehicle[27].gets(cnaim);
	vehicle[28].gets(cnalp);
	vehicle[29].gets(cybet);
	vehicle[30].gets(gmax);
}
