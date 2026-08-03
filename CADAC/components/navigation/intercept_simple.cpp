///////////////////////////////////////////////////////////////////////////////
// COMPONENT: intercept_simple
// CATEGORY: Navigation
// DoF: 3DoF / 6DoF compatible
//
// DESCRIPTION:
//   Simple intercept/miss distance detection
//   Detects closest approach and records miss distance
//   Use for: End-game scoring, simulation termination
//
// INPUTS (from vehicle array):
//   vehicle[0]  - time - double - Simulation time sec
//   vehicle[80] - dta - double - Range to target m
//   vehicle[81] - dvta - double - Closing velocity m/s
//
// OUTPUTS (to vehicle array):
//   vehicle[160] - miss - double - Miss distance m
//   vehicle[161] - tintercept - double - Time of closest approach sec
//   vehicle[162] - intercept_flag - int - 1=intercept detected
//
// PARAMETERS (from input.asc):
//   vehicle[163] - detect_range - double - Detection sphere radius m
//
// DEPENDENCIES:
//   - Requires: Seeker for range and closing velocity
//   - Provides: Intercept metrics for scoring
//
// REFERENCE:
//   Point of closest approach (PCA) detection
//   Occurs when closing velocity changes sign (dvta > 0 → < 0)
//
// USAGE:
//   Include in class header:
//     virtual void def_intercept();
//     virtual void init_intercept();
//     virtual void intercept(double int_step);
//
//   In input.asc MODULES section:
//     intercept   def,init,exec
//
//   In input.asc DATA section:
//     detect_range = 500  // Detection sphere, m
//
// NOTES:
//   - Detects only first intercept (single-shot)
//   - Miss distance frozen at closest approach
//   - Can be used to trigger simulation stop
//   - Simple model (no target size, no warhead effects)
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"

///////////////////////////////////////////////////////////////////////////////
//Definition of intercept module-variables
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::def_intercept()
{
	//Definition of module-variables
	//input data
	vehicle[163].init("detect_range",0,"Detection sphere radius - m","intercept","data","");
	//output to other modules
	vehicle[160].init("miss",0,"Miss distance - m","intercept","out","scrn,plot");
	vehicle[161].init("tintercept",0,"Time of closest approach - sec","intercept","out","scrn");
	vehicle[162].init("intercept_flag","int",0,"Intercept detected flag - ND","intercept","out","");
	//saved values
	vehicle[164].init("dvta_prev",0,"Previous closing velocity - m/s","intercept","save","");
}

///////////////////////////////////////////////////////////////////////////////
//Initial calculations of intercept module
//Member function of class 'Vehicle'
///////////////////////////////////////////////////////////////////////////////
void Vehicle::init_intercept()
{
	//Initialize
	vehicle[160].gets(0.0);      // miss
	vehicle[161].gets(0.0);      // tintercept
	vehicle[162].gets_int(0);    // intercept_flag
	vehicle[164].gets(0.0);      // dvta_prev
}

///////////////////////////////////////////////////////////////////////////////
//Intercept module
//Member function of class 'Vehicle'
//
// Detects point of closest approach (PCA)
// Records miss distance
//
// xxxxxx Created for component library
///////////////////////////////////////////////////////////////////////////////
void Vehicle::intercept(double int_step)
{
	//local module-variables
	double miss(0);
	double tintercept(0);
	int intercept_flag(0);
	double dvta_prev(0);

	//localizing module-variables
	//input data
	double detect_range=vehicle[163].real();
	//input from other modules
	double time=vehicle[0].real();
	double dta=vehicle[80].real();
	double dvta=vehicle[81].real();
	//saved values
	dvta_prev=vehicle[164].real();
	miss=vehicle[160].real();
	tintercept=vehicle[161].real();
	intercept_flag=vehicle[162].integer();

	//-------------------------------------------------------------------------
	//Intercept detection logic

	//Only detect if within detection sphere and not already detected
	if(dta < detect_range && intercept_flag == 0) {

		//Point of closest approach (PCA) occurs when closing velocity
		//changes from positive (closing) to negative (opening)
		//This happens when dvta crosses zero

		if(dvta_prev > 0 && dvta <= 0) {
			//Intercept detected!
			intercept_flag = 1;
			miss = dta;           // Freeze miss distance
			tintercept = time;    // Record time of intercept

			//Display results (optional - can be removed for batch runs)
			cout << "\n*** INTERCEPT DETECTED ***\n";
			cout << "Time: " << tintercept << " sec\n";
			cout << "Miss distance: " << miss << " m\n\n";
		}
	}

	//Save closing velocity for next time step
	dvta_prev = dvta;

	//-------------------------------------------------------------------------
	//loading module-variables
	//output to other modules
	vehicle[160].gets(miss);
	vehicle[161].gets(tintercept);
	vehicle[162].gets_int(intercept_flag);
	//saved values
	vehicle[164].gets(dvta_prev);
}
