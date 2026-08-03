# CADAC Simulations - Build and Run Guide

## Overview

This repository contains Peter Zipfel's CADAC (Computer Aided Design of Aerospace Concepts) simulations, originally designed for Microsoft Visual Studio 2013. The code has been modernized with cross-platform Makefiles for building with GNU Make and g++.

**Note for Aerospace Engineers**: This guide assumes no prior experience with compiling software. All commands are provided - just copy and paste them into your terminal.

## Installation Instructions

### Prerequisites

You need two things:
1. **C++ Compiler**: g++ with C++11 support (GCC 4.8.1 or later)
2. **Build Tool**: GNU Make

### Linux Installation (Recommended)

#### Ubuntu / Debian / Linux Mint / Pop!_OS

Open a terminal and run these commands:

```bash
# Update package list
sudo apt update

# Install C++ compiler and Make
sudo apt install -y build-essential

# Verify installation
g++ --version
make --version
```

You should see output like:
```
g++ (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0
GNU Make 4.3
```

#### Fedora / RHEL / CentOS / AlmaLinux

```bash
# Install development tools
sudo dnf groupinstall "Development Tools"

# Or on older systems:
sudo yum groupinstall "Development Tools"

# Verify installation
g++ --version
make --version
```

#### Arch Linux / Manjaro

```bash
# Install base development tools
sudo pacman -S base-devel

# Verify installation
g++ --version
make --version
```

### macOS Installation

#### Using Homebrew (Recommended)

1. **Install Homebrew** (if you don't have it):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. **Install GCC and Make**:
   ```bash
   brew install gcc make

   # Verify installation
   g++ --version
   make --version
   ```

#### Using Xcode Command Line Tools (Alternative)

```bash
# Install Xcode command line tools
xcode-select --install

# Verify installation
g++ --version
make --version
```

### Windows Installation (Use WSL2)

**For Windows users**: We strongly recommend using Windows Subsystem for Linux (WSL2) rather than trying to compile natively on Windows. WSL2 provides a full Linux environment inside Windows.

#### Step 1: Install WSL2

1. **Open PowerShell as Administrator** (Right-click Start → Windows PowerShell (Admin))

2. **Run this command**:
   ```powershell
   wsl --install
   ```

3. **Restart your computer** when prompted

4. **After restart**, Ubuntu will automatically open and ask you to create a username and password

#### Step 2: Install Build Tools in WSL

Once Ubuntu is running in WSL:

```bash
# Update package list
sudo apt update

# Install C++ compiler and Make
sudo apt install -y build-essential

# Verify installation
g++ --version
make --version
```

#### Step 3: Access Your Windows Files

Your Windows C: drive is available at `/mnt/c/` in WSL:

```bash
# Example: If you cloned the repo to C:\Users\YourName\CADAC
cd /mnt/c/Users/YourName/CADAC

# Or create a Linux directory for projects
mkdir -p ~/projects
cd ~/projects
```

#### Using WSL Tips

- **Open WSL Terminal**: Search for "Ubuntu" in Windows Start menu
- **Copy/Paste**: Right-click in WSL terminal to paste
- **File Access**: You can access WSL files from Windows Explorer at `\\wsl$\Ubuntu\home\yourusername`
- **VS Code Integration**: Install "Remote - WSL" extension in VS Code to edit files easily

### Windows (Native) - Advanced Users Only

If you prefer not to use WSL, you can use MinGW or Cygwin:

#### MinGW-w64 (Minimal)

1. Download from: https://www.mingw-w64.org/downloads/#w64devkit
2. Extract to `C:\mingw64`
3. Add `C:\mingw64\bin` to your PATH
4. Open Command Prompt and verify:
   ```cmd
   g++ --version
   mingw32-make --version
   ```

Note: Use `mingw32-make` instead of `make` on Windows.

#### Cygwin (Full Unix Environment)

1. Download installer from: https://www.cygwin.com/
2. During installation, select these packages:
   - `gcc-g++`
   - `make`
   - `git` (optional, but recommended)
3. Use Cygwin terminal for all commands

### Verification

After installation, verify everything works:

```bash
# Check compiler
g++ --version

# Check make
make --version

# Create a test file
echo 'int main() { return 0; }' > test.cpp

# Compile it
g++ -std=c++11 test.cpp -o test

# Run it
./test

# Clean up
rm test test.cpp

# If all commands succeed, you're ready!
```

## Quick Start

### Clone or Download the Repository

```bash
# If you have git:
git clone https://github.com/yakaboskic/CADAC.git
cd CADAC

# Or download ZIP from GitHub and extract it
cd CADAC-main  # or wherever you extracted it
```

### Building All Simulations

From the root directory:

```bash
make
```

This will compile all 10 simulation examples.

### Building a Specific Simulation

```bash
make ROCKET6G    # Build the three-stage rocket simulation
make MAGSIX      # Build the Magnus rotor simulation
make FALCON6     # Build the F-16 aircraft simulation
```

### Running a Simulation

```bash
cd example/MAGSIX
./magsix
```

Each simulation reads its configuration from `input.asc` in the same directory.

## Detailed Build Instructions

### Individual Simulation Build

Navigate to any example directory and use the local Makefile:

```bash
cd example/ROCKET6G
make              # Build the simulation
make run          # Build and run
make clean        # Remove build artifacts
make cleanout     # Remove only output files
make help         # Display help
```

### Makefile Targets

Each simulation supports these targets:

- `make` or `make all` - Compile the simulation
- `make run` - Compile and execute with default input.asc
- `make clean` - Remove executables and object files
- `make cleanout` - Remove only output files (.asc outputs)
- `make help` - Display available targets

### Root Makefile Targets

From the root directory:

- `make` - Build all simulations
- `make <SIMULATION>` - Build specific simulation
- `make clean` - Clean all simulations
- `make test` - Build and test MAGSIX
- `make help` - Display help

## Available Simulations

| Directory | Simulation | Description |
|-----------|------------|-------------|
| **MAGSIX** | Magnus Rotor | 6DoF Magnus rotor dynamics (simplest example) |
| **ROCKET6G** | Rocket Booster | 6DoF three-stage solid rocket with GPS/INS and orbital insertion guidance |
| **AIM5** | AIM Missile | 5DoF short-range air-to-air missile |
| **ADS6** | Air Defense | 6DoF air defense against rockets and aircraft (Monte Carlo capable) |
| **AGM6** | AGM Missile | 6DoF air-to-ground missile with IR seeker |
| **CRUISE5** | Cruise Missile | 5DoF cruise missile with terminal sensor |
| **FALCON6** | F-16 Aircraft | 6DoF Falcon with flight control system |
| **GHAME3** | Hypersonic 3DoF | 3DoF NASA hypersonic vehicle |
| **GHAME6** | Hypersonic 6DoF | 6DoF NASA hypersonic vehicle with GPS/INS (Monte Carlo capable) |
| **SRAAM6** | SRAAM | 6DoF air-to-air missile with multiple engagements (Monte Carlo capable) |

## Input Files

Each simulation requires an `input.asc` file that defines:

### Input File Structure

```
TITLE <simulation description>
OPTIONS <output options>
MODULES
    <module_name>  def,init,exec,term
    ...
END
TIMING
    scrn_step <seconds>
    plot_step <seconds>
    int_step <seconds>
END
VEHICLES <count>
    <VEHICLE_TYPE> <NAME>
        // Initial conditions and parameters
        <parameter> <value>
        ...
    END
ENDTIME <seconds>
STOP
```

### Example: MAGSIX input.asc

```
TITLE input_trajectoryMR1 Trajectory test case
OPTIONS y_scrn n_events n_tabout y_plot n_merge y_doc n_comscrn n_traj
MODULES
    environment   def,exec
    trajectory    def,init,exec,term
END
TIMING
    scrn_step 2
    plot_step .01
    int_step 0.001
END
VEHICLES 1
    ROTOR RECT.MR1
        sbel1  0          // Initial north position - m
        sbel2  0          // Initial east position - m
        hbe  1000         // Height above sea level - m
        dvbe  16.6        // Rotor speed - m/s
        omega_rpm  850    // Rotor spin rate - RPM
        mass  1.5         // Rotor mass - kg
        cd  1.31          // Drag coefficient
    END
ENDTIME 50
STOP
```

### Input File Options

| Option | Description |
|--------|-------------|
| `y_scrn` | Enable screen output |
| `n_scrn` | Disable screen output |
| `y_plot` | Enable plot file output |
| `y_doc` | Generate documentation file |
| `n_events` | Disable event processing |
| `y_merge` | Merge plot files |

### Module Execution Stages

- `def` - Define module variables
- `init` - Initialize module
- `exec` - Execute module (called each time step)
- `term` - Terminate module

### Timing Parameters

- `scrn_step` - Time interval for screen output (seconds)
- `plot_step` - Time interval for plot data output (seconds)
- `int_step` - Integration time step (seconds)
- `com_step` - Communication bus output interval (seconds)
- `traj_step` - Trajectory output interval (seconds)

## Output Files

After running a simulation, several output files are generated:

### Output File Types

| File | Description |
|------|-------------|
| `plot1.asc` | Primary plot data file with time history of selected variables |
| `plot2.asc`, etc. | Additional plot files for multiple vehicles |
| `traj.asc` | Trajectory data for all vehicles |
| `tabout.asc` | Tabular output data |
| `doc.asc` | Auto-generated documentation of all module variables |
| `input_copy.asc` | Copy of input file with documentation annotations |

### Plot File Format

Plot files use a space-separated column format:

```
1 <title> '<vehicle_name>' <timestamp>
  <num_columns> <column_count>
<column_header_1> <column_header_2> ...
<time1> <value1> <value2> ...
<time2> <value1> <value2> ...
...
```

### Example Plot Data

```
time            dvbe            thtvlx          hbe             omega_rpm
0               16.6074         -76.9859        999.964         850.86
0.01            16.7536         -76.7006        999.233         868.236
0.02            16.8942         -76.4069        998.497         885.83
```

## Plotting Results

The original simulations used KPLOT (from CADAC Studio) for visualization. Modern alternatives:

### Python Plotting

```python
import numpy as np
import matplotlib.pyplot as plt

# Read plot file (skip first 2 header lines)
data = np.loadtxt('plot1.asc', skiprows=2)

# Extract columns
time = data[:, 0]
altitude = data[:, 7]  # hbe (altitude)
velocity = data[:, 4]  # dvbe (velocity)

# Plot
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
ax1.plot(time, altitude)
ax1.set_ylabel('Altitude (m)')
ax1.grid(True)

ax2.plot(time, velocity)
ax2.set_ylabel('Velocity (m/s)')
ax2.set_xlabel('Time (s)')
ax2.grid(True)

plt.tight_layout()
plt.savefig('trajectory.png')
```

### MATLAB/Octave Plotting

```matlab
% Read data
data = load('plot1.asc', '-ascii');

% Skip header rows (manually if needed)
data = data(3:end, :);

% Extract and plot
time = data(:, 1);
altitude = data(:, 8);
velocity = data(:, 5);

figure;
subplot(2,1,1);
plot(time, altitude);
ylabel('Altitude (m)');
grid on;

subplot(2,1,2);
plot(time, velocity);
ylabel('Velocity (m/s)');
xlabel('Time (s)');
grid on;
```

### gnuplot

```bash
gnuplot << EOF
set terminal png size 1200,800
set output 'trajectory.png'
set multiplot layout 2,1
set grid

set title 'Altitude vs Time'
set ylabel 'Altitude (m)'
plot 'plot1.asc' skip 2 using 1:8 with lines title 'Altitude'

set title 'Velocity vs Time'
set ylabel 'Velocity (m/s)'
set xlabel 'Time (s)'
plot 'plot1.asc' skip 2 using 1:5 with lines title 'Velocity'

unset multiplot
EOF
```

## CADAC Framework Architecture

### Core Components

All simulations follow the CADAC architecture:

```
Cadac (Base Class)
  └── Vehicle-Specific Class (e.g., Rotor, Rocket, Missile, Aircraft)
        ├── Environment Module
        ├── Kinematics Module
        ├── Dynamics Module
        ├── Aerodynamics Module
        ├── Propulsion Module
        ├── Guidance Module
        ├── Control Module
        └── Sensors Module (GPS, INS, etc.)
```

### Common Files in Each Simulation

| File | Purpose |
|------|---------|
| `execution.cpp` | Main program and simulation executive |
| `class_hierarchy.hpp` | Class definitions for vehicle objects |
| `global_header.hpp` | Global structures (Module, Variable, Packet) |
| `global_constants.hpp` | Physical constants and dimensions |
| `global_functions.cpp` | Global utility functions (I/O, parsing) |
| `utility_header.hpp` | Matrix class and math utilities |
| `utility_functions.cpp` | Matrix operations, integration, atmosphere |
| `class_functions.cpp` | Vehicle class member functions |
| `*_modules.cpp` | Module implementations (specific to simulation) |

### Matrix Class

CADAC uses a custom Matrix class for vector/matrix operations:

```cpp
Matrix VEC(3,1);              // 3x1 vector
VEC.build_vec3(1.0, 2.0, 3.0);

Matrix MAT(3,3);              // 3x3 matrix
MAT.identity();               // Identity matrix

Matrix RESULT = MAT * VEC;    // Matrix-vector multiplication
double magnitude = VEC.absolute();  // Vector magnitude
Matrix TRANS = MAT.trans();   // Transpose
Matrix INV = MAT.inverse();   // Inverse
```

## Troubleshooting

### Installation Issues

**Problem: "sudo: command not found"**
- You might not be logged in as a user with sudo access
- Try using `su` to become root: `su -` then run `apt install build-essential`
- On some systems, use `doas` instead of `sudo`

**Problem: "E: Unable to locate package build-essential"** (Ubuntu/Debian)
- Your package list might be out of date
- Run: `sudo apt update` then try again

**Problem: "bash: g++: command not found" after installation**
- The installation might have failed
- Try: `sudo apt install --reinstall build-essential`
- Or install manually: `sudo apt install gcc g++ make`

**Problem: "Permission denied" when running make**
- The executable bit might not be set
- Run: `chmod +x magsix` (or whatever your executable is called)

**Problem: WSL not available on Windows**
- You need Windows 10 version 2004+ or Windows 11
- Check your version: Press Win+R, type `winver`, press Enter
- If too old, update Windows or use MinGW instead

**Problem: "gcc version is too old"**
- Check version: `g++ --version`
- You need at least GCC 4.8.1 for C++11 support
- On Ubuntu/Debian: `sudo apt install gcc-11 g++-11`
- Then use: `make CXX=g++-11`

**Problem: "make: command not found"**
- Make wasn't installed
- Ubuntu/Debian: `sudo apt install make`
- macOS: `brew install make` or `xcode-select --install`

**Problem: Slow compilation on Windows with WSL**
- Don't compile from `/mnt/c/` (Windows filesystem)
- Copy to WSL filesystem: `cp -r /mnt/c/path/to/CADAC ~/CADAC`
- Then compile from `~/CADAC` (much faster)

### Common Build Errors

**Error: `strcpy` was not declared**
- **Solution**: Already fixed in all global_header.hpp files with `#include <cstring>`

**Error: extra qualification 'Matrix::'**
- **Solution**: Already fixed in utility_header.hpp files by removing extra qualifications

**Error: cannot find input.asc**
- **Solution**: Ensure you're running from the simulation directory, not root
  ```bash
  cd example/MAGSIX
  ./magsix  # NOT: ../MAGSIX/magsix from example/
  ```

**Error: "No such file or directory" when compiling**
- **Solution**: You might be in the wrong directory
- Make sure you're in the CADAC root directory or a specific example directory
- Run `pwd` to see where you are

**Warning: ignoring return value of 'system'**
- **Info**: Harmless warning from `system("pause")` calls (Windows legacy)
- These can be ignored - the code works fine

### Runtime Issues

**Simulation crashes immediately**
- Check that `input.asc` exists in the current directory
- Verify input.asc syntax matches expected format
- Check error messages for missing parameters

**No output files generated**
- Verify OPTIONS line includes `y_plot` or `y_tabout`
- Check that simulation completed (didn't exit early due to errors)

**Output files are empty**
- Ensure plot_step timing is reasonable (not too large)
- Verify ENDTIME is greater than 0
- Check that vehicle parameters are valid (no NaN/infinity)

## Monte Carlo Simulations

Some simulations support Monte Carlo analysis (ADS6, GHAME6, ROCKET6G, SRAAM6).

### Monte Carlo Input Format

```
MONTE 100  // Run 100 Monte Carlo cases
VEHICLES 1
    ROTOR MC_TEST
        // Parameters with stochastic variation
        mass  GAUSS 1.5 0.1    // Gaussian: mean=1.5, std=0.1
        cd    UNIFORM 1.2 1.4  // Uniform: min=1.2, max=1.4
    END
```

### Stochastic Functions

- `GAUSS <mean> <std>` - Gaussian distribution
- `UNIFORM <min> <max>` - Uniform distribution
- `RAYLEIGH <sigma>` - Rayleigh distribution
- `EXPONENTIAL <lambda>` - Exponential distribution

## Advanced Topics

### Custom Simulations

To create a custom simulation:

1. Copy an existing example as template
2. Modify `class_hierarchy.hpp` to define your vehicle class
3. Implement required modules in separate .cpp files
4. Update the Makefile SOURCES list
5. Create appropriate input.asc file

### Integration Methods

CADAC supports multiple integration methods (in utility_functions.cpp):

- Modified Euler (default)
- Runge-Kutta 4th order
- Custom integration schemes

### Coordinate Systems

- **I**: Inertial (Earth-centered)
- **E**: Earth-fixed (rotating with Earth)
- **G**: Geographic (local tangent plane)
- **B**: Body-fixed
- **V**: Velocity frame
- **S**: Stability frame

## References

1. Zipfel, Peter H, "Modeling and Simulation of Aerospace Vehicle Dynamics", AIAA Education Series, 3rd Ed. 2014
2. Zipfel, Peter H, "Building Aerospace Simulations in C++", AIAA Self-Study Series, 3rd Ed. 2014
3. Zipfel, Peter H, "Fundamentals of 6 DoF Aerospace Simulation and Analysis in C++", AIAA Self-Study Series, 2nd Ed. 2014
4. Zipfel, Peter H, "Advanced 6 DoF Aerospace Simulation and Analysis in C++", AIAA Self-Study Series, 2nd Ed 2014

## License

This code is academic software provided by Peter H. Zipfel and can be used and modified freely without warranty.

## Support

For questions about the CADAC framework, refer to the books listed above or the original documentation in each `example/*/documentation.asc` file.
