# CADAC Quick Start Guide

## 30-Second Start

```bash
git clone <this-repo>
cd CADAC
make test
```

That's it! The MAGSIX simulation will compile and run.

## 5-Minute Tutorial

### Step 1: Build a simulation

```bash
make ROCKET6G
```

### Step 2: Run it

```bash
cd example/ROCKET6G
./rocket6g
```

### Step 3: View results

```bash
head -20 plot1.asc
```

### Step 4: Plot (Python)

```python
import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt('plot1.asc', skiprows=2)
plt.plot(data[:,0], data[:,7])  # time vs altitude
plt.xlabel('Time (s)')
plt.ylabel('Altitude (m)')
plt.grid(True)
plt.show()
```

## Available Simulations

Try these commands:

```bash
make MAGSIX     # Spinning rotor (simplest)
make ROCKET6G   # Three-stage rocket
make FALCON6    # F-16 aircraft
make AIM5       # Air-to-air missile
```

## Customize Input

Edit `input.asc` in any example directory:

```bash
cd example/MAGSIX
nano input.asc   # or vim, emacs, etc.
./magsix         # run with new parameters
```

### Key parameters to try changing:

**MAGSIX (Magnus Rotor)**
- `dvbe` - Initial velocity (m/s)
- `omega_rpm` - Spin rate (RPM)
- `hbe` - Initial altitude (m)
- `mass` - Mass (kg)

**ROCKET6G**
- `lonx`, `latx` - Launch coordinates (deg)
- `psiblx`, `thtblx` - Launch azimuth/elevation (deg)
- Multiple stage parameters

## Output Files

After running any simulation:

| File | What to look at |
|------|----------------|
| `plot1.asc` | Full trajectory data (columns are in header) |
| Screen output | Real-time progress during run |
| `doc.asc` | Complete list of all variables and their meanings |

## Common Tasks

### Build all simulations
```bash
make
```

### Clean everything
```bash
make clean
```

### Run with different input
```bash
cd example/MAGSIX
./magsix                          # uses input.asc
cp input_trajectoryMR1.asc input.asc
./magsix                          # now uses trajectory test case
```

### Extract specific data columns

```bash
# Get time (col 1) and altitude (col 8)
awk 'NR>2 {print $1, $8}' plot1.asc > altitude_vs_time.dat
```

### Quick plot with gnuplot
```bash
gnuplot -p -e "set grid; plot 'plot1.asc' skip 2 using 1:8 with lines title 'Altitude'"
```

## Understanding Output

Plot file format:
```
Line 1: Header (title, vehicle name, timestamp)
Line 2: Column count and names
Line 3+: Data (space-separated)
```

Column headers tell you what each column contains:
- `time` - Simulation time (s)
- `hbe` - Altitude above sea level (m)
- `dvbe` - Velocity magnitude (m/s)
- `psivlx` - Heading angle (deg)
- `thtvlx` - Flight path angle (deg)
- etc.

## Tips

1. **Start Simple**: Try MAGSIX first - it's the simplest simulation
2. **Read the readme**: Each `example/*/readme.asc` has specific info
3. **Check doc.asc**: Auto-generated after first run, documents all variables
4. **Small time steps**: For accuracy, keep `int_step` small (e.g., 0.001-0.01)
5. **Plot data**: The simulations generate ASCII data - easy to plot with any tool

## Troubleshooting

**Problem**: "Cannot find input.asc"
**Solution**: Make sure you're in the simulation directory when you run it

**Problem**: No output files
**Solution**: Check that OPTIONS line includes `y_plot` in input.asc

**Problem**: Strange results
**Solution**: Check units in input.asc (degrees vs radians, etc.)

## Next Steps

1. Read [BUILD_GUIDE.md](BUILD_GUIDE.md) for detailed documentation
2. Explore different `input_*.asc` files in each example directory
3. Try Monte Carlo simulations (ADS6, GHAME6, ROCKET6G, SRAAM6)
4. Modify parameters and observe effects
5. Create custom simulations based on templates

## One-Liner Examples

```bash
# Build and run MAGSIX
make MAGSIX && cd example/MAGSIX && ./magsix

# Build everything
make -j4  # parallel build with 4 jobs

# See what ROCKET6G does
cd example/ROCKET6G && cat readme.asc

# Quick altitude plot (requires gnuplot)
cd example/MAGSIX && ./magsix > /dev/null && gnuplot -p -e "plot 'plot1.asc' skip 2 using 1:8 w l"
```

## Learning Path

1. **Day 1**: Build and run MAGSIX, understand input/output format
2. **Day 2**: Try ROCKET6G, experiment with different inputs
3. **Day 3**: Explore other simulations (FALCON6, AIM5)
4. **Week 2**: Read Zipfel's books, understand the physics
5. **Month 2**: Modify code, create custom simulations

Happy simulating! 🚀
