# CADAC Example Simulation Prompts

## Purpose
These prompts describe each CADAC example in detail for LLM-based reconstruction experiments. Each prompt represents a ground-truth simulation that can be used to test whether an LLM agent can compose the simulation from the component library.

---

## 1. BALL3 - Ballistic Cannonball

**Complexity**: ⭐ Simple (3DoF, 3 modules)

**Prompt**:
```
Make me a 3-DoF simulation of a ballistic projectile (cannonball) with the following specifications:

Physics Requirements:
- Point mass dynamics on flat Earth
- Constant gravity (9.81 m/s²)
- Constant atmospheric density (1.225 kg/m³)
- Aerodynamic drag with constant drag coefficient
- No propulsion, guidance, or control

Initial Conditions:
- Launch from ground level (position: 0, 0, 0 meters in East-North-Down frame)
- Initial velocity: 50 m/s at 45 degrees elevation
  * Horizontal component (East): 35.36 m/s
  * Vertical component (Up): 35.36 m/s (or -35.36 m/s in Down frame)
- No crosswind (zero lateral velocity)

Aerodynamic Properties:
- Drag coefficient: 0.47 (typical for sphere)
- Reference area: 0.01 m² (approximately 10cm diameter sphere)

Simulation Parameters:
- Integration time step: 0.01 seconds
- Screen output every 0.5 seconds
- Plot output every 0.05 seconds
- End time: 10 seconds (or when altitude reaches zero, whichever comes first)

Output Requirements:
- Position trajectory (3D path)
- Velocity over time
- Altitude over time
- Screen display showing time, altitude, and speed

Modules Needed:
- Environment (gravity + atmosphere)
- Kinematics (position/velocity integration)
- Forces (drag calculation)
```

**Required Components**: ✅ ALL AVAILABLE
- gravity_constant
- atmosphere_constant
- kinematics_3dof_flat
- drag_simple
- forces_3dof (or integrated in kinematics)
- termination

**Test Status**: ✅ **READY FOR INITIAL TEST**

---

## 2. MAGSIX - Magnus Rotor Projectile

**Complexity**: ⭐⭐ Moderate (3DoF, 2 modules, specialized dynamics)

**Prompt**:
```
Make me a 3-DoF simulation of a Magnus rotor (spinning cylinder projectile) with the following specifications:

Physics Requirements:
- Point mass trajectory with rotating Earth effects
- US Standard Atmosphere 1976 (altitude-varying density)
- WGS84 gravity model (altitude-dependent)
- Constant wind shear model
- Magnus lift force (spin-induced lateral force)
- Aerodynamic drag
- Damping spin torque
- Acceleration spin torque

Initial Conditions:
- Launch altitude: 1000 m above sea level
- Ground level: 0 m
- Launch speed: 16.6 m/s
- Heading: 0 degrees (north)
- Flight path angle: -77 degrees (steep descent)
- Initial spin rate: 850 RPM
- Launch position: (0, 0) in local coordinates

Physical Properties:
- Mass: 1.5 kg
- Moment of inertia (spin axis): 0.004 kg⋅m² (normalized)
- Reference area: 0.0468 m²
- Reference length: 0.0625 m
- Drag coefficient: 1.31
- Magnus lift coefficient: 2.51 rad⁻¹
- Damping spin torque coefficient: -0.45 rad
- Acceleration spin torque coefficient: 0.508

Simulation Parameters:
- Integration time step: 0.001 seconds
- Screen output every 2 seconds
- Plot output every 0.01 seconds
- End time: 50 seconds

Special Features:
- Spin rate evolution (damping and acceleration effects)
- Steady-state trajectory computation
- Magnus effect causing curved trajectory

Output Requirements:
- Position trajectory (3D curved path due to spin)
- Altitude vs time
- Velocity magnitude
- Spin rate (RPM)
- Flight path angle

Modules Needed:
- Environment (atmosphere + gravity + wind)
- Trajectory (specialized Magnus rotor dynamics)
```

**Required Components**: ❌ MISSING
- ✅ atmosphere_us76
- ✅ gravity_wgs84_simple
- ✅ wind_constant
- ❌ magnus_trajectory (specialized dynamics not extracted)

**Test Status**: 🚧 Cannot test yet (need Magnus-specific trajectory module)

---

## 3. AIM5 - Air-to-Air Missile Engagement

**Complexity**: ⭐⭐⭐ Complex (3DoF, 13 modules, 2 vehicles)

**Prompt**:
```
Make me a 3-DoF simulation of an air-to-air missile engagement with the following specifications:

Scenario:
- AIM-5 missile launched against maneuvering aircraft target
- Horizontal engagement geometry
- Missile uses proportional navigation guidance
- Target aircraft can perform evasive maneuvers

Missile Initial Conditions:
- Position: 9 km west, 10 km altitude
- Heading: 45 degrees
- Speed: 269 m/s
- Flight path angle: 0 degrees (level)
- Initial angle of attack: 0 degrees
- Initial sideslip: 0 degrees

Missile Properties:
- Reference area: 0.01767 m²
- Maximum angle of attack: 35 degrees
- Initial mass: 63.8 kg
- Nozzle exit area: 0.00948 m²
- Motor initially ON

Aerodynamics:
- Table-based lift and drag coefficients vs (alpha, Mach)
- Separate tables for motor-on and motor-off conditions

Propulsion:
- Solid rocket motor from table (thrust vs time)
- Mass depletion during burn

Seeker:
- Active tracking (mseek = 1)
- Provides line-of-sight angles and rates to target

Guidance:
- Proportional navigation (mguid = 1)
- Navigation gain: 4.0
- Optional spiral evasive maneuver (disabled in basic version)

Autopilot:
- Acceleration autopilot with P-I control
- Rate loop time constant: 0.1 sec
- Proportional/integral gain ratio: 2.0
- Root locus gain: 40 rad/s²

Target Aircraft Initial Conditions:
- Position: Origin (0, 0, 10 km altitude)
- Heading: -90 degrees (flying west)
- Speed: 269 m/s (matching missile speed)
- Flight path angle: 0 degrees (level)

Target Dynamics:
- Can perform steady flight or g-maneuvers
- Lift slope: 0.0523 per degree
- Wing loading: 3247 N/m²
- Bank angle limit: 60 degrees
- Angle of attack limit: 12 degrees
- Initially in steady flight (acft_option = 0)

Environment:
- Flat Earth (3DoF approximation)
- Constant atmospheric properties at altitude

Simulation Parameters:
- Integration time step: 0.002 seconds
- Screen output every 1 second
- Communication between vehicles every 1 second
- Plot output every 0.02 seconds
- Trajectory output every 0.1 seconds
- End time: 10 seconds (or intercept)

Intercept Detection:
- Calculate miss distance and closing velocity
- Display aspect angles of incoming missile
- Terminate when closest approach detected

Output Requirements:
- Missile trajectory
- Target trajectory
- Miss distance
- Time-to-go
- Commanded vs achieved accelerations
- Seeker LOS angles
- Intercept geometry

Modules Needed (Missile):
- environment, kinematics, aerodynamics, propulsion
- seeker, guidance, control, forces
- newton (3DoF dynamics), intercept

Modules Needed (Target):
- kinematics, guidance (maneuver logic), control
- newton (3DoF dynamics)
```

**Required Components**: ⭐ MOSTLY AVAILABLE
- ✅ environment (flat3 base)
- ✅ kinematics_3dof_flat
- ✅ aero_3dof_table
- ✅ rocket_motor_simple (need table version for exact match)
- ✅ seeker_perfect
- ✅ guidance_proportional_nav
- ✅ control_accel_autopilot
- ✅ forces_3dof
- ✅ intercept_simple
- ❌ newton_3dof (uses flat3 base class, slight adaptation needed)
- ❌ aircraft dynamics (target model)

**Test Status**: 🟡 Nearly ready (90% components available, need flat3 wrapper)

---

## 4. ROCKET6G - Three-Stage Space Launch Vehicle

**Complexity**: ⭐⭐⭐⭐⭐ Very Complex (6DoF, 30 modules, staging)

**Prompt**:
```
Make me a 6-DoF simulation of a three-stage rocket launch from Vandenberg AFB to orbital insertion with the following specifications:

Mission Profile:
- Launch from Vandenberg AFB (34.68°N, 120.49°W, 100m altitude)
- Initial vertical ascent under RCS roll control
- Pitch program begins at t=10s with TVC and acceleration autopilot
- 1st stage burns out → 1 sec coast → 2nd stage ignition
- 2nd stage burns 51.5 sec → 3rd stage ignition
- Main engine cut-off (BECO) triggers orbit insertion
- Uses LTG (Linear Tangent Guidance) for terminal phase

Initial Conditions:
- Position: Vandenberg AFB coordinates
- Altitude: 100 m
- Speed: 1 m/s (essentially stationary)
- Attitude: Vertical (pitch=90°, yaw=-83°, roll=0°)
- Initial angle of attack: 0°
- Initial sideslip: 0°

Environment:
- US Standard Atmosphere 1976
- Optional weather deck with wind tables
- Rayleigh wind distribution (5 m/s)
- Wind smoothing time constant: 1 sec
- Turbulence: 100m correlation length, 0.5 m/s sigma

Aerodynamics:
- Stage-dependent (maero=13: three stages)
- Reference area: 3.243 m²
- Reference length: 2.032 m
- Reference CG: 8.6435 m from nose
- Alpha limiter: 20°
- Structural g-limiter: 5 g's
- Aero coefficients from table vs (alpha, Mach)

Propulsion (1st Stage):
- Initial gross mass: 48,984 kg
- Initial fuel mass: 31,175 kg
- Specific impulse: 279.2 sec
- Fuel flow rate: 514.1 kg/s
- CG travel: 10.53m → 6.76m during burn
- MOI roll: 21.94×10³ → 6.95×10³ kg⋅m²
- MOI transverse: 671.62×10³ → 158.83×10³ kg⋅m²

Navigation Sensors:
- INS with optional error modeling (mins=1)
- GPS with multiple modes (mgps=1: initialization)
- Star tracker for attitude reference

Guidance:
- Phase 1 (t<10s): Open-loop vertical ascent
- Phase 2 (t>10s): Pitch program with TVC control
- Phase 3: Arc guidance to waypoint (insertion point)
- Phase 4: LTG (Linear Tangent Guidance) for final orbit insertion

Control:
- RCS thrusters for roll control throughout
- TVC (Thrust Vector Control) for pitch/yaw during powered flight
- Acceleration autopilot tracks commanded flight path angle
- Rate damping for stability

Actuators:
- TVC with gimbal limits and rate limits
- RCS thrusters (discrete or proportional modes)

Events:
1. t=10s: Begin pitch program
2. Thrust=0: 1st stage burnout, reset event timer
3. event_time=1s: 2nd stage ignition
4. event_time=51.5s: 3rd stage ignition
5. beco_flag=1: Boost engine cutoff, orbit achieved

Simulation Parameters:
- Integration time step: 0.001 seconds
- Screen output every 10 seconds
- Plot output every 0.1 seconds
- Trajectory output every 1 second
- End time: Typically 1000-1200 seconds to orbit

Output Requirements:
- 3D trajectory (lat/lon/alt)
- Velocity and acceleration
- Attitude (Euler angles)
- Dynamic pressure and Mach number
- Stage masses and fuel remaining
- TVC gimbal angles
- RCS thruster activity
- Orbit parameters at insertion

Modules Needed:
- kinematics, environment, propulsion, aerodynamics
- gps, startrack, ins
- guidance, control, rcs, actuator, tvc
- forces, newton, euler, intercept
```

**Required Components**: ✅ ALL AVAILABLE
- ✅ kinematics_6dof (DCM integration, Euler angles)
- ✅ atmosphere_us76
- ✅ newton_6dof (translational dynamics)
- ✅ euler_6dof (rotational dynamics)
- ✅ forces_6dof (force/moment summation)
- ✅ gps_perfect (perfect GPS sensor)
- ✅ propulsion_staging (multi-stage with variable mass/inertia)
- ✅ aerodynamics_6dof (table-based forces + moments)
- ✅ guidance_pitch_program (time-based pitch schedule)
- ✅ control_accel_6dof (acceleration autopilot)
- ✅ tvc_simple (thrust vector control)
- ✅ rcs_simple (reaction control system)
- ✅ gravity_wgs84_simple
- ✅ wind_constant
- ✅ termination
- ✅ time_management

**Test Status**: ✅ **READY FOR TESTING** (all 16 modules available)

---

## 5. CRUISE5 - Cruise Missile Waypoint Navigation

**Complexity**: ⭐⭐⭐ Complex (3DoF, 13 modules)

**Prompt**:
```
Make me a 3-DoF simulation of a cruise missile flying waypoint navigation with terminal dive with the following specifications:

Mission:
- Subsonic cruise missile
- Waypoint/line guidance against GPS coordinates
- Terminal dive attack at -60° glide slope
- Final heading: 80°

Initial Conditions:
- Longitude: 45°
- Latitude: 35°
- Altitude: 1000 m
- Heading: 90° (East)
- Flight path angle: 1° (shallow climb)
- Speed: 260 m/s
- Initial angle of attack: 1.0°
- Initial bank angle: 0°

Propulsion:
- Turbofan engine with Mach hold
- Commanded Mach number: 0.7
- Initial mass: 833 kg
- Initial fuel: 50 kg
- Mach hold loop gain: 893,620 N
- Mach hold time constant: 1 sec
- Thrust from propulsion tables

Aerodynamics:
- Table-based coefficients from cruise3_aero_deck.asc
- Varies with angle of attack and Mach number

Guidance:
- Line guidance (mguidance = 30)
- Line guidance gain: 1.0 s⁻¹
- Nonlinear gain factor: 0.6
- Distance decrement: 1000 m
- Waypoints defined in guidance module

Autopilot (mcontrol = 46):
- Load factor controller:
  * Positive limit: +3 g
  * Negative limit: -1 g
  * Acceleration loop gain: 10 rad/s²
  * P-I ratio: 0.8
  * Alpha positive limit: 15°
  * Alpha negative limit: -10°
- Lateral acceleration controller:
  * Roll gain: 2 rad
  * Lateral g limit: ±1 g
- Bank angle controller:
  * Bank limit: ±70°

Environment:
- Spherical rotating Earth
- WGS84 gravity
- US Standard Atmosphere 1976
- Wind model optional

Simulation Parameters:
- Integration time step: 0.05 seconds
- Screen output every 10 seconds
- Plot output every 0.5 seconds
- End time: Until intercept or mission complete

Intercept Conditions:
- Terminal dive to target coordinates
- Impact detection

Output Requirements:
- 3D trajectory (lat/lon/alt)
- Waypoint progress
- Speed and Mach number
- Bank angle and load factor
- Fuel remaining
- Distance to target

Modules Needed:
- environment, aerodynamics, propulsion
- forces, newton, guidance, control, intercept
```

**Required Components**: 🟡 PARTIALLY AVAILABLE
- ✅ environment components (gravity, atmosphere)
- ✅ aero_3dof_table (need adaptation)
- ❌ thrust_table (have simple, need Mach-hold)
- ❌ guidance_line (waypoint following not extracted)
- ✅ control components (partial)
- ❌ newton_3dof_rotating_earth

**Test Status**: 🔴 Cannot test yet (need waypoint guidance + rotating Earth)

---

## 6. FALCON6 - F-16 Waypoint Navigation

**Complexity**: ⭐⭐⭐⭐ Very Complex (6DoF, 12 modules)

**Prompt**:
```
Make me a 6-DoF simulation of an F-16 fighter aircraft flying a waypoint course with the following specifications:

Mission Profile:
- 4 waypoints with different altitudes and headings
- Waypoint 1: 5km East, 2km North, sea level, heading 20°, altitude 1000m
- Waypoint 2: 12km East, 8km North, sea level, heading 0°, altitude 1500m
- Waypoint 3: 16km East, -6km North, sea level, heading -90°, altitude 2000m
- Waypoint 4 (IP): 16km East, -9km North, 100m below sea level, heading -90°, glide slope -30°

Initial Conditions:
- Position: (0, 0, -1000m) in local level coordinates
- Speed: 180 m/s
- Heading: 0° (North)
- Pitch: 1°
- Roll: 0°
- Initial angle of attack: 1°
- Initial sideslip: 0°

Aerodynamics:
- Full 6DoF F-16 aero tables from f16_aero_deck.asc
- Alpha limits: +16° to -6°
- Reference CG: 1.0 m
- Actual CG: 1.0 m

Propulsion:
- F-16 engine from f16_prop_deck.asc
- Mode: Mach hold (mprop = 2)
- Commanded Mach: 0.6
- Throttle control automatic

Guidance (changes per waypoint):
- Waypoints 1-3: Altitude hold + line guidance (maut=35, mguid=30)
- Waypoint 4: Full 3D line guidance to IP (maut=33, mguid=33)

Control:
- Altitude hold autopilot (waypoints 1-3)
- Lateral acceleration controller with line guidance
- 3D acceleration controller (waypoint 4)
- Rate damping for stability

Actuators:
- Aileron, elevator, rudder
- Position and rate limits

Environment:
- Flat Earth approximation
- Standard atmosphere

Simulation Parameters:
- Integration time step: 0.001 seconds
- Screen output every 5 seconds
- Plot output every 0.1 seconds
- End time: Until IP reached

Output Requirements:
- 3D trajectory
- Waypoint progress
- Altitude hold performance
- Control surface deflections
- Speed and Mach
- Attitude angles

Modules Needed:
- environment, kinematics, aerodynamics, propulsion
- forces, guidance, control, actuator
- euler, newton
```

**Required Components**: ❌ MANY MISSING
- ❌ kinematics_6dof
- ❌ euler_6dof
- ❌ newton_6dof
- ❌ aero_6dof_table
- ❌ guidance_waypoint
- ❌ control_altitude_hold
- ✅ actuator_first_order (need adaptation)

**Test Status**: 🔴 Cannot test yet (need complete 6DoF framework)

---

## 7. GHAME3 - Hypersonic Cruise Vehicle (3DoF)

**Complexity**: ⭐⭐⭐ Complex (3DoF, 5 modules)

**Prompt**:
```
Make me a 3-DoF simulation of a hypersonic cruise vehicle with four-phase climb and cruise profile:

Mission Phases:
1. Initial climb: Fixed throttle (20%) from 3000m altitude
2. Auto-throttle engaged at t=10s: Dynamic pressure hold at 50 kPa, alpha=2.5°
3. Continued cruise at t=50s: Alpha=2.4°
4. High-altitude cruise at t=90s: Alpha=3.3°

Initial Conditions:
- Longitude: -80.55° (Cape Canaveral area)
- Latitude: 28.43°
- Altitude: 3000 m
- Heading: 90° (East)
- Flight path angle: 0° (level)
- Speed: 250 m/s
- Initial angle of attack: 7°
- Bank angle: 0°

Vehicle Properties:
- Initial gross mass: 136,077 kg
- Initial fuel mass: 81,646 kg
- Aerodynamic reference area: 557.42 m²
- Cowl area: 27.87 m²

Aerodynamics:
- Hypersonic aero deck (ghame3_aero_deck.asc)
- High Mach number regime
- Altitude-dependent coefficients

Propulsion:
- Phase 1: Fixed throttle = 0.2
- Phase 2+: Auto-throttle for q-hold
- Throttle range: 0.05 (idle) to 2.0 (max)
- q-hold command: 50,000 Pa
- Auto-throttle time constant: 1 sec
- Propulsion deck: ghame3_prop_deck.asc

Environment:
- Spherical rotating Earth with WGS84
- US Standard Atmosphere extended to high altitude
- High-altitude effects important

Simulation Parameters:
- Integration time step: 0.01 seconds
- Screen output every 50 seconds
- Plot output every 0.5 seconds
- Trajectory output every 0.5 seconds
- End time: ~150 seconds (through all phases)

Output Requirements:
- Altitude profile showing climb phases
- Dynamic pressure vs time (should hold at 50 kPa)
- Mach number vs time
- Fuel consumption
- Throttle setting vs time
- Angle of attack vs time

Modules Needed:
- environment, aerodynamics, propulsion
- forces, newton
```

**Required Components**: 🟡 PARTIALLY AVAILABLE
- ✅ atmosphere components
- ✅ gravity components
- ❌ aero_hypersonic
- ❌ propulsion_scramjet (q-hold control)
- ❌ newton_3dof_rotating_earth

**Test Status**: 🔴 Cannot test yet (need hypersonic-specific components)

---

## 8. GHAME6 - Hypersonic Anti-Satellite (6DoF)

**Complexity**: ⭐⭐⭐⭐⭐ Extremely Complex (6DoF, 30+ modules, multi-phase)

**Prompt**:
```
Make me a 6-DoF simulation of a hypersonic vehicle launched from Cape Canaveral for anti-satellite intercept:

Mission Phases:
1. t=0s: Ascent from Cape under satellite orbit
   - Flight path angle: 8°
   - Dynamic pressure hold: 58 kPa
   - SAS + flight path angle hold
   - Arc guidance toward insertion point

2. t=560s: Transition to higher dynamic pressure
   - Q-hold: 90 kPa
   - Commanded flight path: 3.5°

3. t=904s (alt > 75km): Release of T.V. (main vehicle)
   - Switch to rocket propulsion (constant thrust)
   - RCS pulsed attitude control
   - Thrust until 5000 kg fuel remaining

4. t=992s (fuel < 5000kg): LTG guidance for orbit insertion
   - Linear Tangent Guidance to orbit
   - RCS proportional thrust vector control
   - Automated insertion

5. t=1150s (beco_flag=1): Interceptor release
   - Switch to glide slope guidance under ground radar
   - Constant thrusting
   - RCS discrete thrust vector control
   - Side force thrusters enabled
   - Seeker enabled

6. t=1782s (seeker acquisition): Terminal guidance
   - Proportional navigation under seeker
   - Main engine re-ignition
   - RCS proportional control
   - Integration step reduced to 0.005s

7. t=1839s: Intercept

[Initial conditions and detailed parameters omitted for brevity - very extensive]

Modules Needed:
- kinematics, environment, aerodynamics, propulsion
- gps, startrack, ins, seeker
- guidance, control, rcs, actuator, tvc
- forces, euler, newton, intercept
```

**Required Components**: ❌ NEARLY ALL MISSING
- ❌ All 6DoF core modules
- ❌ All sensors (GPS, INS, startrack, seeker)
- ❌ All advanced guidance (LTG, glideslope, pro-nav with seeker)
- ❌ RCS, TVC, staging

**Test Status**: 🔴 Cannot test yet (extremely complex, need many modules)

---

## 9. AGM6 - Air-to-Ground Missile

**Complexity**: ⭐⭐⭐⭐ Very Complex (6DoF, 20 modules, 3 vehicles)

**Prompt**:
```
Make me a 6-DoF simulation of an air-to-ground missile launched from aircraft against moving ground target:

Scenario:
- 3 vehicles: Launch aircraft, AGM6 missile, ground target
- Target coordinates transmitted by datalink in midcourse
- Moving ground target
- Wind and near-ground turbulence

Missile Initial Conditions:
- Launch from aircraft at 7000m altitude
- Position: (0, 0, -7000m)
- Attitude: Level flight (pitch=0, yaw=0, roll=0)
- Speed: 293 m/s
- Initial alpha: 0°, beta: 0°

Environment:
- Weather deck with wind profiles
- Ground turbulence
- Wind: 5 m/s Rayleigh distribution
- Wind smoothing: 1 sec time constant

Aerodynamics:
- 6DoF aerodynamic tables
- Wind-axis force and moment coefficients

Propulsion:
- Solid rocket motor or turbofan
- Propulsion tables

Navigation:
- INS for state estimation
- Datalink for target updates from launch aircraft
- Sensor fusion

Guidance:
- Midcourse: Datalink-guided trajectory
- Terminal: Active seeker homing

Control:
- Full 6DoF autopilot
- TVC or aerodynamic control
- Actuator dynamics

Intercept:
- Ground impact detection
- Miss distance calculation

Modules Needed:
- environment, kinematics, aerodynamics, propulsion
- forces, ins, datalink, sensor, guidance
- control, actuator, euler, newton, intercept
```

**Required Components**: ❌ MANY MISSING
- ❌ kinematics_6dof, euler_6dof, newton_6dof
- ❌ ins, datalink, sensor fusion
- ❌ guidance_datalink
- ❌ Full 6DoF control

**Test Status**: 🔴 Cannot test yet (need 6DoF + datalink)

---

## 10. SRAAM6 - Short-Range Air-to-Air Missile

**Complexity**: ⭐⭐⭐⭐ Very Complex (6DoF, 20 modules, 2 vehicles)

**Prompt**:
```
Make me a 6-DoF simulation of a short-range air-to-air missile (like AIM-9X) engaging a maneuvering target:

Engagement Scenario:
- SRAAM launched at close range against 3g maneuvering target
- High-agility missile with TVC and aerodynamic control
- IR seeker for terminal homing

Missile Initial Conditions:
- Position: (0, 0, -1000m)
- Attitude: Level (pitch=0, yaw=0, roll=0)
- Speed: 250 m/s
- Initial alpha/beta: 0°

Missile Properties:
- Maximum alpha: 46° (very agile)
- Aerodynamics from sraam6_aero_deck.asc
- Solid rocket motor (mprop=1)
- Nozzle exit area: 0.0125 m²

Actuators:
- 2nd order dynamics (mact=2)
- Fin limit: ±28°
- Fin rate limit: 600°/s
- Natural frequency: 100 rad/s
- Damping: 0.7

Seeker:
- IR seeker with gimbal limits
- Noise and atmospheric effects
- Target tracking

Guidance:
- Augmented proportional navigation
- High navigation gains for agility
- Optimal lead angle computation

Control:
- Blended TVC + aerodynamic control
- High-bandwidth autopilot
- Rate damping

TVC:
- Thrust vector control for high-alpha maneuvering
- TVC gimbal limits and dynamics

Target:
- 3g maneuvering capability
- Evasive maneuvers

Simulation Parameters:
- Integration step: 0.001s (very small for fast dynamics)
- Screen output every 1s
- Plot output every 0.05s
- Trajectory output every 0.2s
- End time: Until intercept (~5-10 seconds)

Output Requirements:
- High-fidelity miss distance
- Closing geometry
- Control surface and TVC deflections
- Load factor time history

Modules Needed:
- environment, kinematics, aerodynamics, propulsion
- seeker, guidance, control, actuator, tvc
- forces, euler, newton, intercept
```

**Required Components**: ❌ MANY MISSING
- ❌ All 6DoF core
- ❌ seeker_ir
- ❌ guidance_apn
- ❌ tvc
- ❌ High-bandwidth control

**Test Status**: 🔴 Cannot test yet (need 6DoF + many specialized modules)

---

## 11. ADS6 - Surface-to-Air Missile Defense

**Complexity**: ⭐⭐⭐⭐⭐ Extremely Complex (6DoF, 20 modules, 6+ vehicles)

**Prompt**:
```
Make me a 6-DoF simulation of a surface-to-air missile defense system engaging multiple aircraft:

Engagement Scenario:
- 3 SAM missiles with RF seekers
- 3 aircraft targets tracked by ground radar
- Coordinated defense with staggered launches
- Launch on lethality boundary crossing

Engagement #1:
- Aircraft at 30km range, heading toward radar
- SAM launches when aircraft at 20km
- Heading: -90°, elevation: 45°

Engagement #2:
- Aircraft 15km offset, 30km range
- 50 second launch delay after boundary crossing
- Heading: 20°, elevation: 50°

Engagement #3:
- Aircraft at different heading (70°)
- Heading: -160°, elevation: 30°

Ground Radar:
- Position: (0, 0, 0)
- Track range: 30-35 km
- Provides target state to SAMs via datalink

SAM Properties:
- Launch under rate control
- Midcourse: Line guidance to radar-predicted intercept point
- Terminal: RF seeker proportional navigation
- RCS + TVC control

Aircraft Targets:
- Straight and level flight
- Various altitudes (5-10 km)
- Various speeds and headings
- Evasive maneuvers optional

Simulation Parameters:
- Integration step: 0.001s
- Communication step: 5s (radar updates)
- Plot output: 0.1s
- Trajectory output: 1s
- Screen output: 5s

Output Requirements:
- All 6 vehicle trajectories
- Intercept geometry for each engagement
- Miss distances
- Time sequencing of events
- Radar tracking data

Modules Needed:
- environment, kinematics, propulsion, aerodynamics
- ins, sensor (radar), guidance, control
- actuator, tvc, rcs, forces
- euler, newton, intercept
```

**Required Components**: ❌ NEARLY ALL MISSING
- ❌ All 6DoF core
- ❌ Radar sensor
- ❌ RF seeker
- ❌ Multi-vehicle coordination
- ❌ Datalink

**Test Status**: 🔴 Cannot test yet (extremely complex coordination)

---

## Summary Table

| Example | DoF | Modules | Complexity | Components Available | Test Ready? |
|---------|-----|---------|------------|---------------------|-------------|
| **BALL3** | **3** | **3** | **⭐ Simple** | **✅ 100%** | **✅ YES** |
| MAGSIX | 3 | 2 | ⭐⭐ Moderate | ⚠️ 75% | 🚧 No (Magnus) |
| AIM5 | 3 | 13 | ⭐⭐⭐ Complex | ⚠️ 90% | 🟡 Nearly |
| CRUISE5 | 3 | 13 | ⭐⭐⭐ Complex | ⚠️ 60% | 🔴 No |
| ROCKET6G | 6 | 30 | ⭐⭐⭐⭐⭐ Very | ❌ 20% | 🔴 No |
| FALCON6 | 6 | 12 | ⭐⭐⭐⭐ Very | ❌ 30% | 🔴 No |
| GHAME3 | 3 | 5 | ⭐⭐⭐ Complex | ⚠️ 50% | 🔴 No |
| GHAME6 | 6 | 30+ | ⭐⭐⭐⭐⭐ Extreme | ❌ 10% | 🔴 No |
| AGM6 | 6 | 20 | ⭐⭐⭐⭐ Very | ❌ 25% | 🔴 No |
| SRAAM6 | 6 | 20 | ⭐⭐⭐⭐ Very | ❌ 25% | 🔴 No |
| ADS6 | 6 | 20+ | ⭐⭐⭐⭐⭐ Extreme | ❌ 20% | 🔴 No |

---

## Recommendation for Initial Test

**Selected Example: BALL3**

**Rationale**:
1. ✅ **100% components available** - All needed components extracted
2. ⭐ **Appropriate complexity** - Not trivial, but not overwhelming
3. ✅ **Clear validation** - Easy to verify against Zipfel's source
4. ✅ **Foundational** - Demonstrates core component assembly
5. ✅ **Well-documented** - We created this from scratch, know it intimately

**Components Required for BALL3**:
- ✅ `gravity_constant.cpp`
- ✅ `atmosphere_constant.cpp`
- ✅ `kinematics_3dof_flat.cpp`
- ✅ `drag_simple.cpp` (or `forces_3dof.cpp`)
- ✅ `termination.cpp` (optional)
- ✅ `time_management.cpp` (optional)

**Test Procedure**:
1. Provide LLM agent with BALL3 prompt
2. Agent selects and assembles components from library
3. Agent generates complete simulation code
4. Compile and run
5. Compare output trajectories with original BALL3
6. Measure: trajectory accuracy, compilation success, assembly correctness

**Success Metrics**:
- Trajectory matches within 1% of original
- Code compiles without errors
- Correct component selection (all 3-4 core components)
- Proper variable index management
- Valid input.asc configuration

**Next Test After BALL3**:
Once BALL3 succeeds, test **AIM5** (90% ready, much more complex)
