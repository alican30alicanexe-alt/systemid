
// Auto-generated main for BALL3_test
#include "class_hierarchy.hpp"

int main()
{
    // Create simulation
    Vehicle vehicle;

    // Load input file
    vehicle.load_input("../input.asc");

    // Run simulation
    vehicle.run();

    return 0;
}
