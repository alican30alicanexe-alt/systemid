///////////////////////////////////////////////////////////////////////////////
// COMPONENT: propulsion_staging
// CATEGORY: Propulsion
// DoF: 6DoF
//
// DESCRIPTION:
//   Rocket propulsion with staging, fuel depletion, and variable mass/inertia
//   Computes thrust with nozzle expansion effects
//   Interpolates CG and inertia tensor as fuel burns
//   Use for: Multi-stage rockets, boosters with significant mass variation
//
// INPUTS (from vehicle array):
//   vehicle[0] - time - double - Simulation time sec
//   vehicle[52] - press - double - Ambient pressure Pa
//   vehicle[503] - mfreeze - int - Freeze flag for autopilot testing
//
// OUTPUTS (to vehicle array):
//   vehicle[10] - mprop - int - Propulsion mode
//   vehicle[15] - vmass - double - Vehicle mass kg
//   vehicle[17] - xcg - double - CG location from nose m
//   vehicle[18] - IBBB - Matrix(3x3) - Inertia tensor kg·m²
//   vehicle[22] - fmasse - double - Fuel mass expended kg (state)
//   vehicle[23] - fmassd - double - Fuel mass rate kg/s (state)
//   vehicle[26] - thrust - double - Thrust N
//   vehicle[27] - fmassr - double - Remaining fuel mass kg
//
// PARAMETERS (from input.asc):
//   vehicle[10] - mprop - int - Propulsion mode (0=off, 3=const thrust, 4=LTG)
//   vehicle[11] - acowl - double - Cowl area m² (reserved)
//   vehicle[16] - vmass0 - double - Initial gross mass kg
//   vehicle[21] - fmass0 - double - Initial fuel mass kg
//   vehicle[24] - aexit - double - Nozzle exit area m²
//   vehicle[25] - spi - double - Specific impulse sec
//   vehicle[28] - xcg_0 - double - Initial CG from nose m
//   vehicle[29] - xcg_1 - double - Final CG from nose m
//   vehicle[33] - fuel_flow_rate - double - Fuel flow rate kg/s
//   vehicle[38] - moi_roll_0 - double - Initial roll MOI kg·m²
//   vehicle[39] - moi_roll_1 - double - Burnout roll MOI kg·m²
//   vehicle[40] - moi_trans_0 - double - Initial transverse MOI kg·m²
//   vehicle[41] - moi_trans_1 - double - Burnout transverse MOI kg·m²
//
// DEPENDENCIES:
//   - Requires: Environment module for press
//   - Provides: vmass for newton module
//   - Provides: IBBB for euler module
//   - Provides: thrust for forces module
//
// REFERENCE:
//   Zipfel, "Modeling and Simulation of Aerospace Vehicle Dynamics", 2nd ed.
//   Chapter 7: Propulsion
//
//   Rocket thrust equation:
//     T = Isp * mdot * g + (P_sl - P_amb) * A_exit
//
//   where:
//     Isp = specific impulse (sec)
//     mdot = fuel flow rate (kg/s)
//     g = 9.80665 m/s² (standard gravity)
//     P_sl = 101325 Pa (sea level pressure)
//     P_amb = ambient pressure (Pa)
//     A_exit = nozzle exit area (m²)
//
//   Fuel depletion:
//     dmfuel/dt = T / (Isp * g)
//     m(t) = m0 - mfuel_expended
//
//   Linear interpolation of properties:
//     CG(t) = CG_0 + (CG_1 - CG_0) * (mfuel_expended / mfuel_initial)
//     I(t) = I_0 + (I_1 - I_0) * (mfuel_expended / mfuel_initial)
//
// USAGE:
//   Include in class header:
//     virtual void def_propulsion();
//     virtual void init_propulsion();
//     virtual void propulsion(double int_step);
//
//   In input.asc MODULES section:
//     propulsion   def,init,exec
//
//   In input.asc DATA section:
//     mprop = 3               // Constant thrust mode
//     vmass0 = 23474          // Initial mass, kg
//     fmass0 = 18500          // Fuel mass, kg
//     spi = 290               // Specific impulse, sec
//     fuel_flow_rate = 150    // Flow rate, kg/s
//     aexit = 0.6             // Exit area, m²
//     xcg_0 = 3.5             // Initial CG, m
//     xcg_1 = 4.2             // Burnout CG, m
//     moi_roll_0 = 1500       // Initial roll MOI, kg·m²
//     moi_roll_1 = 300        // Burnout roll MOI, kg·m²
//     moi_trans_0 = 150000    // Initial transverse MOI, kg·m²
//     moi_trans_1 = 30000     // Burnout transverse MOI, kg·m²
//
// NOTES:
//   - mprop modes: 0=off, 3=constant thrust input, 4=LTG guidance control
//   - Engine shuts down when fuel exhausted (fmassr <= 0)
//   - CG shifts aft as fuel (typically stored forward) depletes
//   - Inertia decreases significantly as fuel burns (mass closer to CG)
//   - Roll MOI (Ixx) typically 1/100 of transverse MOI (Iyy, Izz)
//   - Transverse MOI dominates pitch/yaw dynamics
//   - Nozzle expansion term (P_sl - P_amb)*A_exit adjusts for altitude
//   - Over-expanded at sea level (loss), under-expanded in vacuum (gain)
//   - mfreeze flag allows freezing thrust/mass/inertia for autopilot gain tuning
//   - For multi-stage: use discrete changes in initial conditions
//   - Fuel consumption: mdot = T / (Isp * g)
//   - Typical Isp: 250-300 sec (solid), 350-450 sec (liquid)
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of propulsion module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_propulsion()
{
	//Definition and initialization of module-variables
	vehicle[10].init("mprop","int",0,"=0:none; =3 input; =4 LTG control","propulsion","data","");
	vehicle[11].init("acowl",0,"Cowl area of engine inlet - m^2","propulsion","data","");
	vehicle[15].init("vmass",0,"Vehicle mass - kg","propulsion","out","scrn,plot");
	vehicle[16].init("vmass0",0,"Initial gross mass - kg","propulsion","data","");
	vehicle[17].init("xcg",0,"CG location from nose (pos) - m","propulsion","out","plot");
	vehicle[18].init("IBBB",0,0,0,0,0,0,0,0,0,"Vehicle moment of inertia - kgm^2","propulsion","out","");
	vehicle[21].init("fmass0",0,"Initial fuel mass in stage - kg","propulsion","data","");
	vehicle[22].init("fmasse",0,"Fuel mass expended (zero initialization required) - kg","propulsion","state","scrn,plot");
	vehicle[23].init("fmassd",0,"Fuel mass expended derivative - kg/s","propulsion","state","");
	vehicle[24].init("aexit",0,"Nozzle exit area - m^2","propulsion","data","");
	vehicle[25].init("spi",0,"Specific impulse - sec","propulsion","data","");
	vehicle[26].init("thrust",0,"Thrust - N","propulsion","out","scrn,plot");
	vehicle[27].init("fmassr",0,"Remaining fuel mass - kg","propulsion","save","scrn,plot");
	vehicle[28].init("xcg_0",0,"Initial cg location from nose - m","propulsion","data","");
	vehicle[29].init("xcg_1",0,"Final cg location from nose - m","propulsion","data","");
	vehicle[33].init("fuel_flow_rate",0,"Fuel flow rate of rocket motor - kg/s","propulsion","data","");
	vehicle[36].init("vmass0_st",0,"Initial mass of exo-vehicle - kg","propulsion","data","");
	vehicle[37].init("fmass0_st",0,"Initial fuel mass of exo-vehicle - kg","propulsion","data","");
	vehicle[38].init("moi_roll_0",0,"Roll MOI of vehicle, initial - kgm^2","propulsion","data","");
	vehicle[39].init("moi_roll_1",0,"Roll MOI of vehicle, burn-out - kgm^2","propulsion","data","");
	vehicle[40].init("moi_trans_0",0,"Transverse MOI of vehicle, initial - kgm^2","propulsion","data","");
	vehicle[41].init("moi_trans_1",0,"Transverse MOI of vehicle, burn-out - kgm^2","propulsion","data","");
	vehicle[42].init("mfreeze_prop","int",0,"Saving 'mfreeze' value","propulsion","save","");
	vehicle[43].init("thrustf",0,"Saved thrust when mfreeze=1 - N ","propulsion","save","");
	vehicle[44].init("vmassf",0,"Saved mass when mfreeze=1 - N ","propulsion","save","");
	vehicle[45].init("IBBBF",0,0,0,0,0,0,0,0,0,"Saved MOI when mfreeze=1 - kgm^2","propulsion","save","");
}

///////////////////////////////////////////////////////////////////////////////
//Propulsion initialization module
//Member function of class 'Vehicle'
//
// Placeholder - mass properties initialized from input data
//
// 030514 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_propulsion()
{
	//placeholder
}

///////////////////////////////////////////////////////////////////////////////
//Propulsion module
//Member function of class 'Vehicle'
//
// Computes rocket thrust with nozzle expansion effects
// Integrates fuel consumption
// Updates vehicle mass, CG, and inertia tensor
//
// 030514 Created by Peter Zipfel
// xxxxxx Adapted to component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::propulsion(double int_step)
{
	//local variable
	double psl(101325); //sea level pressure - Pa
	double gainq(0);
	Matrix IBBB0(3,3);
	Matrix IBBB1(3,3);

	//local module-variables
	double thrust(0);

	//localizing module-variables
	//input data
	int mprop=vehicle[10].integer();
	double vmass0=vehicle[16].real();
	double fmass0=vehicle[21].real();
	double aexit=vehicle[24].real();
	double spi=vehicle[25].real();
	double xcg_0=vehicle[28].real();
	double xcg_1=vehicle[29].real();
	double fuel_flow_rate=vehicle[33].real();
	double moi_roll_0=vehicle[38].real();
	double moi_roll_1=vehicle[39].real();
	double moi_trans_0=vehicle[40].real();
	double moi_trans_1=vehicle[41].real();
	//getting saved variable
	double vmass=vehicle[15].real();
	double xcg=vehicle[17].real();
	Matrix IBBB=vehicle[18].mat();
	double fmassr=vehicle[27].real();
	int mfreeze_prop=vehicle[42].integer();
	double thrustf=vehicle[43].real();
	double vmassf=vehicle[44].real();
	Matrix IBBBF=vehicle[45].mat();
	//state variable
	double fmasse=vehicle[22].real();
	double fmassd=vehicle[23].real();
	//input from other modules
	double time=vehicle[0].real();
	double press=vehicle[52].real();      // Ambient pressure
	int mfreeze=vehicle[503].integer();   // Freeze flag
	//----------------------------------------------------------------------------
	//**NO THRUSTING**
	if(mprop == 0) {
		fmassd = 0;
		thrust = 0;
		fmasse = 0;
		fmassr = 0;
	}

	//**PROPULSION ACTIVE**
	if(mprop > 0) {

		//**Constant thrust rocket engine**
		if(mprop == 3 || mprop == 4) {

			// Rocket thrust equation:
			// T = Isp * mdot * g + (P_sl - P_amb) * A_exit
			thrust = spi * fuel_flow_rate * AGRAV + (psl - press) * aexit;

			// Load initial and final inertia tensors
			IBBB0.zero();
			IBBB0.assign_loc(0,0, moi_roll_0);   // Roll (Ixx)
			IBBB0.assign_loc(1,1, moi_trans_0);  // Pitch (Iyy)
			IBBB0.assign_loc(2,2, moi_trans_0);  // Yaw (Izz)

			IBBB1.zero();
			IBBB1.assign_loc(0,0, moi_roll_1);   // Roll at burnout
			IBBB1.assign_loc(1,1, moi_trans_1);  // Pitch at burnout
			IBBB1.assign_loc(2,2, moi_trans_1);  // Yaw at burnout
		}

		//**Fuel consumption integration**
		if (spi != 0) {
			// mdot = T / (Isp * g)
			double fmassd_next = thrust / (spi * AGRAV);
			fmasse = integrate(fmassd_next, fmassd, fmasse, int_step);
			fmassd = fmassd_next;
		}

		//**Update mass and fuel remaining**
		vmass = vmass0 - fmasse;
		fmassr = fmass0 - fmasse;

		//**Interpolate inertia tensor as function of fuel expended**
		// Linear interpolation: I(t) = I0 + (I1 - I0) * (fmasse/fmass0)
		double mass_ratio = fmasse / fmass0;
		IBBB = IBBB0 + (IBBB1 - IBBB0) * mass_ratio;

		//**Interpolate CG as function of fuel expended**
		// CG typically shifts aft as forward-stored fuel depletes
		xcg = xcg_0 + (xcg_1 - xcg_0) * mass_ratio;

		//**Engine shutdown when fuel exhausted**
		if(fmassr <= 0) {
			mprop = 0;
			thrust = 0;
		}
	}

	//**Freeze variables for autopilot response calculations**
	// Allows holding thrust/mass/inertia constant for gain tuning
	if(mfreeze == 0)
		mfreeze_prop = 0;
	else {
		if(mfreeze != mfreeze_prop) {
			mfreeze_prop = mfreeze;
			thrustf = thrust;
			vmassf = vmass;
			IBBBF = IBBB;
		}
		thrust = thrustf;
		vmass = vmassf;
		IBBB = IBBBF;
	}
	//----------------------------------------------------------------------------
	//loading module-variables
	//state variables
	vehicle[22].gets(fmasse);
	vehicle[23].gets(fmassd);
	//saving variables
	vehicle[10].gets(mprop);
	vehicle[27].gets(fmassr);
	vehicle[42].gets(mfreeze_prop);
	vehicle[43].gets(thrustf);
	vehicle[44].gets(vmassf);
	vehicle[45].gets_mat(IBBBF);
	//output to other modules
	vehicle[15].gets(vmass);     // To newton module
	vehicle[17].gets(xcg);
	vehicle[18].gets_mat(IBBB);  // To euler module
	vehicle[26].gets(thrust);    // To forces module
}
