///////////////////////////////////////////////////////////////////////////////
// COMPONENT: forces_6dof
// CATEGORY: Dynamics
// DoF: 6DoF
//
// DESCRIPTION:
//   Summation of all forces and moments for 6-DoF dynamics
//   Combines aerodynamics, propulsion, TVC, and RCS contributions
//   Use for: Rockets, missiles, aircraft in 6DoF
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[57] - pdynmc - double - Dynamic pressure Pa
//   vehicle[10] - mprop - int - Propulsion mode flag
//   vehicle[26] - thrust - double - Thrust force N
//   vehicle[50] - mrcs_moment - int - RCS moment mode (0-23)
//   vehicle[51] - mrcs_force - int - RCS force mode (0,1,2)
//   vehicle[64] - FMRCS - Matrix(3x1) - RCS moments N·m
//   vehicle[84] - FARCS - Matrix(3x1) - RCS forces N
//   vehicle[104] - refa - double - Reference area m²
//   vehicle[105] - refd - double - Reference length m
//   vehicle[112] - cy - double - Side force coefficient
//   vehicle[113] - cll - double - Roll moment coefficient
//   vehicle[114] - clm - double - Pitch moment coefficient
//   vehicle[115] - cln - double - Yaw moment coefficient
//   vehicle[116] - cx - double - Axial force coefficient
//   vehicle[117] - cz - double - Normal force coefficient
//   vehicle[900] - mtvc - int - TVC mode flag (0,1,2,3)
//   vehicle[910] - FPB - Matrix(3x1) - TVC thrust vector N
//   vehicle[911] - FMPB - Matrix(3x1) - TVC moments N·m
//
// OUTPUTS (to vehicle array):
//   vehicle[200] - FAPB - Matrix(3x1) - Applied force body frame N
//   vehicle[201] - FMB - Matrix(3x1) - Applied moment body frame N·m
//
// PARAMETERS (from input.asc):
//   (None - all inputs come from other modules)
//
// DEPENDENCIES:
//   - Requires: Aerodynamics module for coefficients (cx,cy,cz,cll,clm,cln)
//   - Requires: Propulsion module for thrust
//   - Requires: Environment module for pdynmc
//   - Optional: TVC module for FPB, FMPB
//   - Optional: RCS module for FARCS, FMRCS
//   - Provides: FAPB for newton module (translational dynamics)
//   - Provides: FMB for euler module (rotational dynamics)
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 6: Forces and Moments
//
//   Aerodynamic forces:
//     F_aero = q * S * C
//     where q = dynamic pressure, S = reference area, C = coefficient
//
//   Aerodynamic moments:
//     M_aero = q * S * d * C
//     where d = reference length (diameter or chord)
//
//   Total forces and moments:
//     FAPB = F_aero + F_thrust + F_rcs
//     FMB = M_aero + M_tvc + M_rcs
//
// USAGE:
//   Include in class header:
//     virtual void def_forces();
//     virtual void forces();
//
//   In input.asc MODULES section:
//     forces   def,exec
//
//   In input.asc DATA section:
//     refa = 0.1767   // Reference area, m²
//     refd = 0.476    // Reference diameter, m
//
// NOTES:
//   - No initialization required (pure summing function)
//   - Aerodynamic forces computed as q*S*C (standard aircraft formulation)
//   - Thrust added along X-axis unless TVC is active
//   - TVC (Thrust Vector Control) deflects thrust and creates moments
//   - RCS (Reaction Control System) provides auxiliary forces/moments
//   - Mode flags control which subsystems contribute:
//     * mprop > 0: propulsion active
//     * mtvc = 1,2,3: TVC active
//     * mrcs_force = 1,2: RCS force active
//     * mrcs_moment = 1-23: RCS moment active
//   - Sign conventions: X forward, Y right, Z down (body frame)
//   - Gravity NOT included here (handled separately in newton module)
//   - All forces/moments in body frame for dynamics integration
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of forces module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_forces()
{
	//Definition and initialization of module-variables
	vehicle[200].init("FAPB",0,0,0,"Aerodynamic and propulsive forces in body axes - N","forces","out","");
	vehicle[201].init("FMB",0,0,0,"Aerodynamic and propulsive moments in body axes - N*m","forces","out","");
}

///////////////////////////////////////////////////////////////////////////////
//Force module
//Member function of class 'Vehicle'
//
// Sums all non-gravitational forces and moments
// Combines aerodynamics, propulsion, TVC, and RCS
//
// 030507 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::forces()
{
	//local module-variables
	Matrix FAPB(3,1);
	Matrix FMB(3,1);

	//localizing module-variables
	//input from other modules
	double time=vehicle[0].real();
	double pdynmc=vehicle[57].real();        // Dynamic pressure
	int mprop=vehicle[10].integer();         // Propulsion mode
	double thrust=vehicle[26].real();        // Thrust
	int mrcs_moment=vehicle[50].integer();   // RCS moment mode
	int mrcs_force=vehicle[51].integer();    // RCS force mode
	Matrix FMRCS=vehicle[64].vec();          // RCS moments
	Matrix FARCS=vehicle[84].vec();          // RCS forces
	double refa=vehicle[104].real();         // Reference area
	double refd=vehicle[105].real();         // Reference length
	double cy=vehicle[112].real();           // Side force coeff
	double cll=vehicle[113].real();          // Roll moment coeff
	double clm=vehicle[114].real();          // Pitch moment coeff
	double cln=vehicle[115].real();          // Yaw moment coeff
	double cx=vehicle[116].real();           // Axial force coeff
	double cz=vehicle[117].real();           // Normal force coeff
	int mtvc=vehicle[900].integer();         // TVC mode
	Matrix FPB=vehicle[910].vec();           // TVC thrust vector
	Matrix FMPB=vehicle[911].vec();          // TVC moments
	//----------------------------------------------------------------------------
	//**AERODYNAMIC FORCES**
	//
	// Standard aircraft formulation: F = q * S * C
	// Body frame: X forward, Y right, Z down
	FAPB.assign_loc(0,0, pdynmc*refa*cx);  // Axial force
	FAPB.assign_loc(1,0, pdynmc*refa*cy);  // Side force
	FAPB.assign_loc(2,0, pdynmc*refa*cz);  // Normal force

	//**AERODYNAMIC MOMENTS**
	//
	// M = q * S * d * C
	// Body frame: L roll, M pitch, N yaw
	FMB.assign_loc(0,0, pdynmc*refa*refd*cll);  // Roll moment
	FMB.assign_loc(1,0, pdynmc*refa*refd*clm);  // Pitch moment
	FMB.assign_loc(2,0, pdynmc*refa*refd*cln);  // Yaw moment

	//**ADD THRUST**
	//
	// If TVC active: use deflected thrust vector and moments
	// Otherwise: plain thrust along X-axis
	if(mtvc==1 || mtvc==2 || mtvc==3) {
		FAPB = FAPB + FPB;   // Add TVC thrust vector
		FMB = FMB + FMPB;    // Add TVC moments
	}
	else if(mprop)
		FAPB[0] = FAPB[0] + thrust;  // Plain thrust along X

	//**ADD RCS FORCES**
	if(mrcs_force==1 || mrcs_force==2)
		FAPB = FAPB + FARCS;

	//**ADD RCS MOMENTS**
	if(mrcs_moment > 0 && mrcs_moment <= 23)
		FMB = FMB + FMRCS;
	//----------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[200].gets_vec(FAPB);  // To newton module
	vehicle[201].gets_vec(FMB);   // To euler module
}
