///////////////////////////////////////////////////////////////////////////////
// COMPONENT: aerodynamics_6dof
// CATEGORY: Aerodynamics
// DoF: 6DoF
//
// DESCRIPTION:
//   Full 6-DoF aerodynamics with table-based coefficients
//   Computes forces (cx, cy, cz) and moments (cll, clm, cln) in body frame
//   Handles multi-stage rocket configurations with different aero decks
//   Use for: Space launch vehicles, rockets with varying configurations
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[53] - rho - double - Air density kg/m³
//   vehicle[56] - vmach - double - Mach number
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[75] - dvba - double - Airspeed m/s
//   vehicle[140] - alppx - double - Total angle of attack deg
//   vehicle[141] - phipx - double - Aerodynamic roll angle deg
//   vehicle[144] - alphax - double - Angle of attack deg
//   vehicle[145] - betax - double - Sideslip angle deg
//   vehicle[160] - ppx - double - Roll rate deg/s
//   vehicle[161] - qqx - double - Pitch rate deg/s
//   vehicle[162] - rrx - double - Yaw rate deg/s
//   vehicle[10] - mprop - int - Propulsion mode (for base drag)
//   vehicle[15] - vmass - double - Vehicle mass kg
//   vehicle[17] - xcg - double - CG location from nose m
//
// OUTPUTS (to vehicle array):
//   vehicle[104] - refa - double - Reference area m²
//   vehicle[105] - refd - double - Reference length m
//   vehicle[108] - xcg_ref - double - Reference CG location m
//   vehicle[112] - cy - double - Side force coefficient
//   vehicle[113] - cll - double - Roll moment coefficient
//   vehicle[114] - clm - double - Pitch moment coefficient
//   vehicle[115] - cln - double - Yaw moment coefficient
//   vehicle[116] - cx - double - Axial force coefficient
//   vehicle[117] - cz - double - Normal force coefficient
//   vehicle[118] - ca0 - double - Axial force coeff(Mach)
//   vehicle[119] - caa - double - Delta axial force due to alpha
//   vehicle[120] - cn0 - double - Normal force coeff(Mach,alpha)
//   vehicle[121] - clm0 - double - Pitch moment coeff(Mach,alpha)
//   vehicle[122] - clmq - double - Pitch damping derivative 1/deg
//   vehicle[123] - cla - double - Lift slope derivative 1/deg
//   vehicle[133] - cma - double - Pitch moment derivative 1/deg
//   vehicle[191] - gnmax - double - Max g available (pitch) g
//   vehicle[192] - gymax - double - Max g available (yaw) g
//
// PARAMETERS (from input.asc):
//   vehicle[100] - maero - int - Aero mode (11=1-stage, 12=2-stage, 13=3-stage)
//   vehicle[104] - refa - double - Reference area m²
//   vehicle[105] - refd - double - Reference length m
//   vehicle[108] - xcg_ref - double - Reference CG location m
//   vehicle[187] - alplimx - double - Alpha limiter deg
//   vehicle[188] - alimitx - double - Structural g limiter g
//
// DEPENDENCIES:
//   - Requires: Environment module for rho, vmach, pdynmc
//   - Requires: Kinematics module for angles and rates
//   - Requires: Propulsion module for vmass, xcg
//   - Requires: Aero table file (e.g., aero_deck_SLV.asc)
//   - Provides: Force/moment coefficients for forces module
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 8: Aerodynamics
//
//   Force coefficients in aeroballistic frame:
//     CA = axial force coefficient (drag direction)
//     CN = normal force coefficient (perpendicular to velocity)
//
//   Transform to body frame:
//     CX = -CA
//     CY = -CN * sin(phi_aero)
//     CZ = -CN * cos(phi_aero)
//
//   Moment coefficients:
//     Cll = roll moment coefficient
//     Clm = pitch moment coefficient
//     Cln = yaw moment coefficient
//
//   CG shift correction:
//     Clm_actual = Clm_ref - CN * (xcg_ref - xcg) / refd
//
// USAGE:
//   Include in class header:
//     virtual void def_aerodynamics();
//     virtual void init_aerodynamics();
//     virtual void aerodynamics(double int_step);
//
//   In input.asc MODULES section:
//     aerodynamics   def,init,exec
//
//   In input.asc DATA section:
//     maero = 13          // 3-stage configuration
//     refa = 1.767        // Reference area, m²
//     refd = 1.5          // Reference diameter, m
//     xcg_ref = 8.5       // Reference CG, m from nose
//     alplimx = 20        // Max alpha, deg
//     alimitx = 5         // Structural limit, g
//     AERO_DECK aero_deck_SLV.asc
//
// NOTES:
//   - maero flag selects which stage's aero tables to use:
//     * 11 = 1st stage (last remaining stage)
//     * 12 = 2-stage configuration
//     * 13 = 3-stage configuration (launch)
//   - Aero tables vs Mach and alpha (total angle of attack)
//   - Base drag (ca0b) added when propulsion active (thrust plume)
//   - CG shift from reference handled by moment transfer
//   - Damping derivatives (clmq, cnr) stabilize pitch/yaw rates
//   - Aeroballistic coordinates: x-axis along velocity, phi around velocity
//   - Body coordinates: x-forward, y-right, z-down
//   - Load factor limits computed from max alpha capability
//   - Typical rocket aerodynamics: high drag at transonic, low at hypersonic
//   - Normal force slope (cla) typically 0.01-0.02 per degree
//   - Pitch damping (clmq) typically -5 to -20 per radian
//   - Static stability: cma < 0 (restoring moment)
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of aerodynamic module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_aerodynamics()
{
	//Definition and initialization of module-variables
	vehicle[100].init("maero","int",0,"=11: last stage; =12: 2 stages; =13: 3 stages","aerodynamics","data","");
	vehicle[104].init("refa",0,"Reference area for aero coefficients - m^2","aerodynamics","init","");
	vehicle[105].init("refd",0,"Reference length for aero coefficients - m","aerodynamics","init","");
	vehicle[108].init("xcg_ref",0,"Reference cg location from nose - m","aerodynamics","init","");
	//main force and moment coefficients
	vehicle[112].init("cy",0,"Side force coefficient - ND","aerodynamics","out","");
	vehicle[113].init("cll",0,"Rolling moment coefficient - ND","aerodynamics","out","");
	vehicle[114].init("clm",0,"Pitching moment coefficient - ND","aerodynamics","out","");
	vehicle[115].init("cln",0,"Yawing moment coefficient - ND","aerodynamics","out","");
	vehicle[116].init("cx",0,"X-force coefficient - ND","aerodynamics","out","");
	vehicle[117].init("cz",0,"Z-force coefficient - ND","aerodynamics","out","");
	//table look-up coefficients
	vehicle[118].init("ca0",0,"Axial force coeff(Mach) - ND","aerodynamics","diag","");
	vehicle[119].init("caa",0,"Delta axial force due to alpha(Mach) - ND","aerodynamics","diag","");
	vehicle[120].init("cn0",0,"Normal force coeff(Mach,alpha) - ND","aerodynamics","diag","");
	vehicle[121].init("clm0",0,"Pitch moment coeff(Mach,alpha) - ND","aerodynamics","diag","");
	vehicle[122].init("clmq",0,"Pitch dampning deriv(Mach) - 1/deg","aerodynamics","diag","");
	vehicle[123].init("cla",0,"Lift slope derivative(alpha,mach) - 1/deg","aerodynamics","out","");
	vehicle[133].init("cma",0,"Pitch moment due to alpha deriv(alpha,mach) -1/deg","aerodynamics","diag","");
	//limiters
	vehicle[187].init("alplimx",0,"Alpha limiter for vehicle - deg","aerodynamics","data","");
	vehicle[188].init("alimitx",0,"Structural  limiter for vehicle - g's","aerodynamics","data","");
	vehicle[189].init("gnavail",0,"G available in pitch for vehicle - g's","aerodynamics","diag","");
	vehicle[190].init("gyavail",0,"G available in yaw for vehicle - g's","aerodynamics","diag","");
	vehicle[191].init("gnmax",0,"Max g permissable in pitch for vehicle - g's","aerodynamics","out","plot");
	vehicle[192].init("gymax",0,"Max g permissable in yaw for vehicle - g's","aerodynamics","out","plot");
}

///////////////////////////////////////////////////////////////////////////////
//Initialization of aerodynamic module
//Member function of class 'Vehicle'
//
// Placeholder - reference parameters set from input data
//
// 050103 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_aerodynamics()
{
	//placeholder
}

///////////////////////////////////////////////////////////////////////////////
//Aerodynamic module
//Member function of class 'Vehicle'
//
// Performs aerodynamic table look-up from aero deck
// Calculates force and moment coefficients in body coordinates
//
// 050103 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::aerodynamics(double int_step)
{
	//local variable
	int thrust_on=false;
	double cn(0);
	double cn0mx(0);
	double ca0b(0);

	//local module-variables
	double ca(0);
	double cna(0);
	double cy(0);
	double cx(0);
	double cz(0);
	double cll(0);
	double clm(0);
	double cln(0);
	double ca0(0);
	double caa(0);
	double cn0(0);
	double clm0(0);
	double clmq(0);
	double gnavail(0);
	double gyavail(0);
	double gnmax(0);
	double gymax(0);

	//localizing module-variables
	//input data
	int maero=vehicle[100].integer();
	double alplimx=vehicle[187].real();
	double alimitx=vehicle[188].real();
	double refa=vehicle[104].real();
	double refd=vehicle[105].real();
	double xcg_ref=vehicle[108].real();
	//loading saved value
	double cla=vehicle[123].real();
	double cma=vehicle[133].real();
	//input from other modules
	double time=vehicle[0].real();
	double alppx=vehicle[140].real();     // Total AOA
	double phipx=vehicle[141].real();     // Aero roll angle
	double alphax=vehicle[144].real();    // AOA
	double betax=vehicle[145].real();     // Sideslip
	double rho=vehicle[53].real();
	double vmach=vehicle[56].real();
	double pdynmc=vehicle[57].real();
	double dvba=vehicle[75].real();
	double ppx=vehicle[160].real();       // Body rates
	double qqx=vehicle[161].real();
	double rrx=vehicle[162].real();
	int mprop=vehicle[10].integer();
	double vmass=vehicle[15].real();
	double xcg=vehicle[17].real();
	//----------------------------------------------------------------------------
	//**TRANSFORM BODY RATES TO AEROBALLISTIC FRAME**
	//
	// Aeroballistic frame: x along velocity, phi around velocity
	double phip = phipx * RAD;
	double cphip = cos(phip);
	double sphip = sin(phip);
	double qqax = qqx*cphip - rrx*sphip;  // Pitch rate in aero frame
	double rrax = qqx*sphip + rrx*cphip;  // Yaw rate in aero frame

	//----------------------------------------------------------------------------
	//**AXIAL FORCE COEFFICIENTS (TABLE LOOKUP)**
	//
	// Lookup based on stage configuration (maero)
	if(maero == 13) {
		ca0 = aerotable.look_up("ca0slv3_vs_mach", vmach);
		caa = aerotable.look_up("caaslv3_vs_mach", vmach);
		ca0b = aerotable.look_up("ca0bslv3_vs_mach", vmach);  // Base drag
	} else if(maero == 12) {
		ca0 = aerotable.look_up("ca0slv2_vs_mach", vmach);
		caa = aerotable.look_up("caaslv2_vs_mach", vmach);
		ca0b = aerotable.look_up("ca0bslv2_vs_mach", vmach);
	} else if(maero == 11) {
		ca0 = aerotable.look_up("ca0slv1_vs_mach", vmach);
		caa = aerotable.look_up("caaslv1_vs_mach", vmach);
		ca0b = aerotable.look_up("ca0bslv1_vs_mach", vmach);
	}

	// Total axial force coefficient
	// Add base drag if propulsion active (thrust plume interaction)
	if(mprop) thrust_on = true;
	ca = ca0 + caa*alppx + thrust_on*ca0b;

	//----------------------------------------------------------------------------
	//**NORMAL FORCE COEFFICIENTS (TABLE LOOKUP)**
	if(maero == 13) {
		cn0 = aerotable.look_up("cn0slv3_vs_mach_alpha", vmach, alppx);
	} else if(maero == 12) {
		cn0 = aerotable.look_up("cn0slv2_vs_mach_alpha", vmach, alppx);
	} else if(maero == 11) {
		cn0 = aerotable.look_up("cn0slv1_vs_mach_alpha", vmach, alppx);
	}

	cna = cn0;  // Normal force coefficient

	//----------------------------------------------------------------------------
	//**PITCHING MOMENT COEFFICIENTS (TABLE LOOKUP)**
	if(maero == 13) {
		clm0 = aerotable.look_up("clm0slv3_vs_mach_alpha", vmach, alppx);
		clmq = aerotable.look_up("clmqslv3_vs_mach", vmach);
	} else if(maero == 12) {
		clm0 = aerotable.look_up("clm0slv2_vs_mach_alpha", vmach, alppx);
		clmq = aerotable.look_up("clmqslv2_vs_mach", vmach);
	} else if(maero == 11) {
		clm0 = aerotable.look_up("clm0slv1_vs_mach_alpha", vmach, alppx);
		clmq = aerotable.look_up("clmqslv1_vs_mach", vmach);
	}

	// Pitch moment with rate damping and CG shift
	double clmaref = clm0 + clmq*qqax*refd/(2.*dvba);  // Add damping
	double clma = clmaref - cna*(xcg_ref - xcg)/refd;  // Correct for CG shift

	//----------------------------------------------------------------------------
	//**COMPUTE DIMENSIONAL DERIVATIVES** (for autopilot use)
	//
	// Lift slope derivative cla (per deg)
	// Compute by finite difference: cn(alpha+3°) - cn(alpha-3°)
	double alplx = alppx + 3.0;
	double alpmx = alppx - 3.0;
	if(alpmx < 0.) alpmx = 0.0;

	double cn0p(0), cn0m(0);
	if(maero == 13) {
		cn0p = aerotable.look_up("cn0slv3_vs_mach_alpha", vmach, alplx);
		cn0m = aerotable.look_up("cn0slv3_vs_mach_alpha", vmach, alpmx);
	} else if(maero == 12) {
		cn0p = aerotable.look_up("cn0slv2_vs_mach_alpha", vmach, alplx);
		cn0m = aerotable.look_up("cn0slv2_vs_mach_alpha", vmach, alpmx);
	} else if(maero == 11) {
		cn0p = aerotable.look_up("cn0slv1_vs_mach_alpha", vmach, alplx);
		cn0m = aerotable.look_up("cn0slv1_vs_mach_alpha", vmach, alpmx);
	}
	// Update cla only if within limits
	if(alplx < alplimx)
		cla = (cn0p - cn0m) / (alplx - alpmx);

	// Pitch moment derivative cma (per deg)
	double clm0p(0), clm0m(0);
	if(maero == 13) {
		clm0p = aerotable.look_up("clm0slv3_vs_mach_alpha", vmach, alplx);
		clm0m = aerotable.look_up("clm0slv3_vs_mach_alpha", vmach, alpmx);
	} else if(maero == 12) {
		clm0p = aerotable.look_up("clm0slv2_vs_mach_alpha", vmach, alplx);
		clm0m = aerotable.look_up("clm0slv2_vs_mach_alpha", vmach, alpmx);
	} else if(maero == 11) {
		clm0p = aerotable.look_up("clm0slv1_vs_mach_alpha", vmach, alplx);
		clm0m = aerotable.look_up("clm0slv1_vs_mach_alpha", vmach, alpmx);
	}
	// Update cma only if within limits
	if(alppx < alplimx)
		cma = (clm0p - clm0m) / (alplx - alpmx) - cla*(xcg_ref - xcg)/refd;

	//----------------------------------------------------------------------------
	//**CONVERT TO BODY FRAME COEFFICIENTS**
	//
	// Transform from aeroballistic (along velocity) to body frame
	cx = -ca;                 // X forward
	cy = -cna * sphip;        // Y right
	cz = -cna * cphip;        // Z down

	// Moment coefficients in body axes
	cll = 0;                  // No roll moment (axisymmetric)
	clm = clma * cphip;       // Pitch moment
	cln = -clma * sphip;      // Yaw moment

	//----------------------------------------------------------------------------
	//**CALCULATE LOAD FACTOR LIMITS**
	//
	// Max load factor available at alpha limiter
	if(maero == 13) {
		cn0mx = aerotable.look_up("cn0slv3_vs_mach_alpha", vmach, alplimx);
	} else if(maero == 12) {
		cn0mx = aerotable.look_up("cn0slv2_vs_mach_alpha", vmach, alplimx);
	} else if(maero == 11) {
		cn0mx = aerotable.look_up("cn0slv1_vs_mach_alpha", vmach, alplimx);
	}

	double anlmx = cn0mx * pdynmc * refa;  // Max normal force
	double weight = vmass * AGRAV;
	gnmax = anlmx / weight;                 // Max load factor (g's)
	if(gnmax >= alimitx) gnmax = alimitx;   // Apply structural limit

	double aloadn = cn * pdynmc * refa;     // Current normal force
	double gng = aloadn / weight;
	gnavail = gnmax - gng;                  // Available load factor

	// Same limits in yaw plane (axisymmetric)
	gymax = gnmax;
	gyavail = gnavail;
	//----------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[104].gets(refa);
	vehicle[112].gets(cy);
	vehicle[113].gets(cll);
	vehicle[114].gets(clm);
	vehicle[115].gets(cln);
	vehicle[116].gets(cx);
	vehicle[117].gets(cz);
	vehicle[191].gets(gnmax);
	vehicle[192].gets(gymax);
	vehicle[123].gets(cla);    // For autopilot
	vehicle[133].gets(cma);    // For autopilot
	//diagnostics
	vehicle[118].gets(ca0);
	vehicle[119].gets(caa);
	vehicle[120].gets(cn0);
	vehicle[121].gets(clm0);
	vehicle[122].gets(clmq);
	vehicle[189].gets(gnavail);
	vehicle[190].gets(gyavail);
}
