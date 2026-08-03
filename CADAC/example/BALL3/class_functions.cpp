///////////////////////////////////////////////////////////////////////////////
//FILE: 'class_functions.cpp'
//
// Member functions of 'Cadac' class hierarchy
// Member functions of class 'Variable'
// 
//010628 Created by Peter H Zipfel
//130522 Adapted to BALL3 simulation, PZi
///////////////////////////////////////////////////////////////////////////////

#include "class_hierarchy.hpp"
#include "global_header.hpp"

///////////////////////////////////////////////////////////////////////////////
//////////////// Member functions of 'Cadac' class hierarchy //////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//Constructor initializing the modules and the module-variable arrays
//Reading input data from 'input.asc' and putting into 'ball' arrays 
//Writing banners to screen, 'tabout.asc' and to 'traj.asc' files  
//
//Module-variable arrays:
//	ball[NBALL]		contains variables of modules under class 'Ball'
//	ball3[nball3]	contains variables of all modules with empty slots removed
//	scrn_ball3[nscrn_ball3]	contains variables to be displayed at screen and 'tabout.asc'
//	plot_ball3[nplot_ball3] contains variables to be plotted, i.e., written to 'traj.asc'
//	com_ball3[ncom_ball3] contains variables for communication among vehicles
//  event_ptr_list[NEVENT] event pointer list
//
//Index pointer arrays:	
//	ball_scrn_ind[ball_scrn_count];
//	ball_plot_ind[ball_plot_count];
//	ball_com_ind[ball_com_count];
//
//				  
//001220 Created by Peter H Zipfel
//011129 Adapted to SRAAM6 simulation, PZi
//130522 Adapted to BALL3 simulation, PZi
///////////////////////////////////////////////////////////////////////////////

Ball::Ball(Module *module_list,int num_modules)
{
	int i(0),j(0);
	//creating module-variable array
	ball=new Variable[NBALL];
	if(ball==0){cerr<<"*** Error: ball[] allocation failed ***\n";system("pause");system("pause");exit(1);}

	//zeroeing module-variable array
	for(i=0;i<NBALL;i++)ball[i].init("empty",0," "," "," "," ");

	//calling initializer modules to build 'ball' arrays
	//and make other initial calculations in the following sequence

	//call the module definitions -MOD
	for (j=0;j<num_modules;j++)
		{
			if((module_list[j].name=="environment")&&(module_list[j].definition=="def"))
				def_environment();
			else if((module_list[j].name=="kinematics")&&(module_list[j].definition=="def"))
				def_kinematics();
			else if((module_list[j].name=="forces")&&(module_list[j].definition=="def"))
				def_forces();
		}

	//sizing module-variable arrays 'ball3','scrn_ball3','plot_ball3' arrays
	//their dimensions are the protected data:'nball3','nscrn_ball3','nplot_ball3'
	sizing_arrays();

	//allocating dynamic memory to the module-variable arrays
	ball3=new Variable[nball3];		
	if(!ball3){cerr<<"*** Error: ball3[] allocation failed *** \n";system("pause");system("pause");exit(1);}

	scrn_ball3=new Variable[nscrn_ball3];
	if(!scrn_ball3){cerr<<"*** Error: scrn_ball3[] allocation failed *** \n";system("pause");exit(1);}

	plot_ball3=new Variable[nplot_ball3];
	if(!plot_ball3){cerr<<"*** Error: plot_ball3[] allocation failed *** \n";system("pause");exit(1);}

	com_ball3=new Variable[ncom_ball3];
	if(!com_ball3){cerr<<"*** Error: com_ball3[] allocation failed *** \n";system("pause");exit(1);}

	// allocating memory for the screen index arrays
	ball_scrn_ind=new int[ball_scrn_count];

	// allocating memory for the plot index arrays
	ball_plot_ind=new int[ball_plot_count];

	// allocating memory for the com index arrays
	ball_com_ind=new int[ball_com_count];

	//allocating memory to each event object in event object list
	for (i=0;i<NEVENT;i++)
		event_ptr_list[i]=new Event;

	//initializing the event array index
	nevent=0;
	event_total=0;

	//building 'ball3' array (compacting 'ball' arrays)
	vehicle_array();

	//building 'scrn_ball3' array from 'ball3' array
	scrn_array();

	//building 'plot_ball3' array from 'ball3' array
	plot_array();

	//building the index arrays of the data to be written to the screen
	scrn_index_arrays();

	//building the index arrays of the data to be written to the 'ploti.asc' files
	plot_index_arrays();

	//building the index arrays of the data to be loaded into the packets of 'combus'
	com_index_arrays();
}
///////////////////////////////////////////////////////////////////////////////
//Destructor deallocating dynamic memory
//				  
//010115 Created by Peter H Zipfel
//011129 Adapted to SRAAM6 simulation, PZi
///////////////////////////////////////////////////////////////////////////////

Ball::~Ball()
{
	delete [] ball;
	delete [] ball3;
	delete [] scrn_ball3;
	delete [] plot_ball3;
	delete [] com_ball3;
	delete [] ball_scrn_ind;
	delete [] ball_plot_ind;
	delete [] ball_com_ind;
	delete [] &event_ptr_list;
}

///////////////////////////////////////////////////////////////////////////////
//////////////////// Members of class 'Vehicle' ///////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Constructor of class 'Vehicle'
//allocating dynamic memory for the array of pointers of type 'Cadac'
//and returning the pointer to array
//
//010626 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Vehicle::Vehicle(int number)
{
	capacity=number;
	vehicle_ptr=new Cadac *[capacity];
	if(vehicle_ptr==0){cerr<<"*** Error:'vehicle_ptr' allocation failed *** \n";system("pause");exit(1);}
	howmany=0;
//	cerr<<">>> inside constructor of 'Vehicle' <<<\n";
}

///////////////////////////////////////////////////////////////////////////////
//Destructor of class 'Vehicle'
//de-allocating dynamic memory of the pointer-array of type 'Cadac'
//
//010626 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Vehicle::~Vehicle()
{
//	cerr<<">>> inside destructor of 'Vehicle' <<<\n";
	delete [] vehicle_ptr;
}

///////////////////////////////////////////////////////////////////////////////
//Adding a vehicle pointer to the vehicle list
//
//010626 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
void Vehicle::add_vehicle(Cadac &pt)
{
	if(howmany<capacity)
		vehicle_ptr[howmany++]=&pt;
	double dum=0;//!
}
///////////////////////////////////////////////////////////////////////////////
//Overloading operator [] so that it returns a 'Cadac' pointer
//
//010626 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
Cadac * Vehicle::operator[](int position)
{
	if(position>=0 && position<howmany)
		return vehicle_ptr[position];
	else
	{
		cout<<"*** Bad value: "<<position<<'\n';
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
//Obtaining size of vehicle list (total number of vehicles)
//
//010626 Created by Peter H Zipfel
///////////////////////////////////////////////////////////////////////////////
int Vehicle::size()
{
	return howmany;
}

