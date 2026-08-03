///////////////////////////////////////////////////////////////////////////////
//FILE: 'class_hierarchy.hpp'
//
// BALL3 - 3DoF Ballistic Projectile Simulation
// Simplest possible CADAC simulation to demonstrate framework
//
// Contains the classes of the hierarchy of base class 'Cadac'
//
///////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_DEPRECATE
#ifndef cadac_class_hierarchy__HPP
#define cadac_class_hierarchy__HPP

#include "global_header.hpp"

///////////////////////////////////////////////////////////////////////////////
//Abstract base class: Cadac
///////////////////////////////////////////////////////////////////////////////
class Cadac
{
private:
	char name[CHARN]; //vehicle object name

protected:
	//array of module-variables as defined in class 'Ball'
	Variable *ball;

public:
	//flag indicating an 'event' has occured
	bool event_epoch;

	virtual~Cadac(){};

	///////////////////////////////////////////////////////////////////////////
	//Constructor of class 'Cadac'
	///////////////////////////////////////////////////////////////////////////
	Cadac(){}

	///////////////////////////////////////////////////////////////////////////
	//Setting vehicle object name
	///////////////////////////////////////////////////////////////////////////
	void set_name(char *vehicle_name) {strcpy(name,vehicle_name);}

	///////////////////////////////////////////////////////////////////////////
	//Getting vehicle object name
	///////////////////////////////////////////////////////////////////////////
	char *get_vname() {return name;}

	//////////////////////////executive functions /////////////////////////////
	virtual void sizing_arrays()=0;
	virtual void vehicle_array()=0;
	virtual void scrn_array()=0;
	virtual void plot_array()=0;
	virtual void scrn_banner()=0;
	virtual void tabout_banner(ofstream &ftabout,char *title)=0;
	virtual void tabout_data(ofstream &ftabout)=0;
	virtual void vehicle_data(fstream &input)=0;
	virtual void read_tables(char *file_name,Datadeck &datatable)=0;
	virtual void scrn_index_arrays()=0;
	virtual void scrn_data()=0;
	virtual void plot_banner(ofstream &fplot,char *title)=0;
	virtual void plot_index_arrays()=0;
	virtual void plot_data(ofstream &fplot,bool merge)=0;
	virtual void event(char *options)=0;
	virtual void document(ostream &fdoc,char *title,Document *doc_ball3)=0;
	virtual void com_index_arrays()=0;
	virtual Packet loading_packet_init(int num_ball)=0;
	virtual Packet loading_packet(int num_ball)=0;

	//module functions
	virtual void def_environment()=0;
	virtual void environment(double int_step)=0;
	virtual void def_kinematics()=0;
	virtual void init_kinematics()=0;
	virtual void kinematics(double int_step)=0;
	virtual void def_forces()=0;
	virtual void forces(double int_step)=0;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Derived class: Ball
//
// 3DoF ballistic projectile
// Modules: environment, kinematics, forces
//
///////////////////////////////////////////////////////////////////////////////
class Ball:public Cadac
{
protected:
	//name of BALL vehicle object
	char ball3_name[CHARL];

	//event list
	Event *event_ptr_list[NEVENT];int nevent;
	int event_total;

	//compacted array of all module-variables of vehicle object 'Ball'
	Variable *ball3;int nball3;

	//screen ouput array
	Variable *scrn_ball3;int nscrn_ball3;

	//plot ouput array
	Variable *plot_ball3;int nplot_ball3;

	//communications ouput array
	Variable *com_ball3;int ncom_ball3;

	//packet of data for each ball vehicle
	Packet packet;

	//indicator array pointing to the module-variable which are to be written to screen
	int *ball_scrn_ind; int ball_scrn_count;

	//indicator array pointing to the module-variable which are to be written to 'ploti.asc'
	int *ball_plot_ind; int ball_plot_count;

	//indicator array pointing to the module-variable which are to be written to 'combus'
	int *ball_com_ind; int ball_com_count;

	//declaring Table pointer as temporary storage of a single table
	Table *table;
	//declaring Datadeck 'aerotable' that stores all aero tables
	Datadeck aerotable;

public:
	Ball(){};
	Ball(Module *module_list,int num_modules);
	virtual~Ball();

	//executive functions
	virtual void sizing_arrays();
	virtual void vehicle_array();
	virtual void scrn_array();
	virtual void plot_array();
	virtual void scrn_banner();
	virtual void tabout_banner(ofstream &ftabout,char *title);
	virtual void tabout_data(ofstream &ftabout);
	virtual void vehicle_data(fstream &input);
	virtual void read_tables(char *file_name,Datadeck &datatable);
	virtual void scrn_index_arrays();
	virtual void scrn_data();
	virtual void plot_banner(ofstream &fplot,char *title);
	virtual void plot_index_arrays();
	virtual void plot_data(ofstream &fplot,bool merge);
	virtual void event(char *options);
	virtual void document(ostream &fdoc,char *title,Document *doc_ball3);
	virtual void com_index_arrays();
	virtual Packet loading_packet_init(int num_ball);
	virtual Packet loading_packet(int num_ball);

	//module functions
	virtual void def_environment();
	virtual void environment(double int_step);
	virtual void def_kinematics();
	virtual void init_kinematics();
	virtual void kinematics(double int_step);
	virtual void def_forces();
	virtual void forces(double int_step);
};

///////////////////////////////////////////////////////////////////////////////
////////////////////////// Global class 'Vehicle'//////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Vehicle
{
private:
	int capacity;	//max number of vehicles permitted in vehicle list
	int howmany;	//actual number of vehicles in vehicle list
	Cadac **vehicle_ptr;
public:
	Vehicle(int number=1);
	virtual ~Vehicle();
	void add_vehicle(Cadac &ptr);
	Cadac *operator[](int position);
	int size();
};

#endif
