# CADAC Component Catalog

## Quick Reference Guide

This catalog lists all extracted components from the CADAC examples, organized by function.

**Last Updated**: 2025-11-24
**Extracted**: 36 of ~55 components (72% complete)

## Symbol Key

- ✅ **Extracted** - Component is in library, ready to use
- 📝 **Documented** - Source identified, extraction pending
- 🔄 **Complex** - Needs multiple files or dependencies
- 🎯 **Recommended** - Good starting point for new users

---

## Actuators (2/4 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `tvc_simple` | ✅ | ROCKET6G | 6 | Thrust vector control with 2nd-order dynamics |
| `rcs_simple` | ✅ | ROCKET6G | 6 | Reaction control system for attitude control |
| `actuator_first_order` | ✅ | AIM5 | 3/6 | First-order actuator lag |
| `control_surface_actuator` | 📝 | FALCON6 | 6 | Aileron, elevator, rudder dynamics |

**Variable Assignments:**
```cpp
// tvc_simple
vehicle[900] = mtvc (int) - TVC mode
vehicle[902] = tvclimx (double) - Nozzle deflection limit deg
vehicle[904] = dtvclimx (double) - Nozzle rate limit deg/s
vehicle[905] = wntvc (double) - TVC natural frequency rad/s
vehicle[906] = zettvc (double) - TVC damping
vehicle[908] = gtvc (double) - TVC gain
vehicle[909] = parm (double) - Propulsion moment arm m
vehicle[910] = FPB (Matrix 3x1) - Thrust vector in body N
vehicle[911] = FMPB (Matrix 3x1) - Thrust moments N·m

// rcs_simple
vehicle[50] = mrcs_moment (int) - RCS moment mode
vehicle[51] = mrcs_force (int) - RCS force mode
vehicle[55-57] = roll/pitch/yaw_mom_max (double) - Max moments N·m
vehicle[58] = rcs_zeta (double) - RCS damping
vehicle[59] = rcs_freq (double) - RCS natural frequency rad/s
vehicle[64] = FMRCS (Matrix 3x1) - RCS moments N·m
vehicle[84] = FARCS (Matrix 3x1) - RCS forces N
```

---

## Aerodynamics (3/6 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `drag_simple` | ✅ | BALL3 | 3 | 🎯 Point mass drag only |
| `aero_3dof_table` | ✅ | AIM5 | 3 | Lift+drag from tables |
| `aerodynamics_6dof` | ✅ | ROCKET6G | 6 | Full forces + moments from tables |
| `aero_hypersonic` | 📝 | GHAME6 | 6 | High-speed aerodynamics |
| `aero_aircraft` | 📝 | FALCON6 | 6 | Aircraft-specific aero tables |
| `aero_missile` | 📝 | SRAAM6 | 6 | Missile-specific aero tables |

**Variable Assignments:**
```cpp
// aerodynamics_6dof
vehicle[100] = maero (int) - Aero mode (11=1-stage, 12=2-stage, 13=3-stage)
vehicle[104] = refa (double) - Reference area m²
vehicle[105] = refd (double) - Reference length m
vehicle[108] = xcg_ref (double) - Reference CG location m
vehicle[112] = cy (double) - Side force coefficient
vehicle[113] = cll (double) - Roll moment coefficient
vehicle[114] = clm (double) - Pitch moment coefficient
vehicle[115] = cln (double) - Yaw moment coefficient
vehicle[116] = cx (double) - Axial force coefficient
vehicle[117] = cz (double) - Normal force coefficient
vehicle[123] = cla (double) - Lift slope derivative 1/deg
vehicle[133] = cma (double) - Pitch moment derivative 1/deg
```

---

## Control (5/8 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `control_none` | ✅ | BALL3 | 3/6 | 🎯 No control (ballistic) |
| `control_rate_damping` | ✅ | AIM5 | 3/6 | Simple rate feedback |
| `control_accel_autopilot` | ✅ | AIM5 | 3 | 3DoF acceleration autopilot with P-I |
| `control_accel_6dof` | ✅ | ROCKET6G | 6 | 6DoF dual-channel acceleration autopilot |
| `actuator_first_order` | ✅ | AIM5 | 3/6 | First-order actuator lag |
| `control_pid_aircraft` | 📝 | FALCON6 | 6 | 3-axis PID autopilot for aircraft |
| `control_lqr` | 📝 | GHAME6 | 6 | Linear Quadratic Regulator |
| `control_flight_path` | 📝 | CRUISE5 | 3/6 | Flight path angle hold |

**Variable Assignments:**
```cpp
// control_accel_6dof
vehicle[500] = maut (int) - Autopilot mode |mauty|mautp|
vehicle[504] = waclp (double) - Pitch nat freq rad/s
vehicle[505] = zaclp (double) - Pitch damping
vehicle[506] = paclp (double) - Pitch real pole 1/s
vehicle[509] = delimx (double) - Pitch command limit deg
vehicle[510] = drlimx (double) - Yaw command limit deg
vehicle[520] = delecx (double) - Pitch command deg
vehicle[521] = delrcx (double) - Yaw command deg
vehicle[568-570] = wacly, zacly, pacly - Yaw parameters
vehicle[575] = alcomx (double) - Lateral accel command g
vehicle[576] = ancomx (double) - Normal accel command g
```

---

## Dynamics (4/6 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `forces_3dof` | ✅ | BALL3 | 3 | 🎯 Force summation for 3DoF |
| `newton_6dof` | ✅ | ROCKET6G | 6 | 6DoF translational dynamics with WGS84 |
| `euler_6dof` | ✅ | ROCKET6G | 6 | 6DoF rotational dynamics with gyroscopic coupling |
| `forces_6dof` | ✅ | ROCKET6G | 6 | 6DoF force and moment summation |
| `newton_3dof` | 📝 | BALL3 | 3 | Point mass dynamics F=ma |
| `quaternion_6dof` | 📝 | GHAME6 | 6 | Quaternion-based attitude dynamics |

**Variable Assignments:**
```cpp
// newton_6dof
vehicle[219] = lonx (double) - Longitude deg
vehicle[220] = latx (double) - Latitude deg
vehicle[221] = alt (double) - Altitude m
vehicle[225] = dvbe (double) - Geographic speed m/s
vehicle[226] = dvbi (double) - Inertial speed m/s
vehicle[228] = psivdx (double) - Heading angle deg
vehicle[229] = thtvdx (double) - Flight path angle deg
vehicle[235] = SBII (Matrix 3x1) - Inertial position m
vehicle[236] = VBII (Matrix 3x1) - Inertial velocity m/s
vehicle[237] = ABII (Matrix 3x1) - Inertial acceleration m/s²

// euler_6dof
vehicle[160] = ppx (double) - Roll rate deg/s
vehicle[161] = qqx (double) - Pitch rate deg/s
vehicle[162] = rrx (double) - Yaw rate deg/s
vehicle[164] = WBIB (Matrix 3x1) - Body rates wrt inertial rad/s
vehicle[166] = WBII (Matrix 3x1) - Angular velocity in inertial rad/s

// forces_6dof
vehicle[200] = FAPB (Matrix 3x1) - Applied force body frame N
vehicle[201] = FMB (Matrix 3x1) - Applied moment body frame N·m
```

---

## Environment (6/8 extracted)

### Gravity Models (2/3)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `gravity_constant` | ✅ | BALL3 | 3/6 | 🎯 Simple g=9.81 m/s² |
| `gravity_wgs84_simple` | ✅ | ROCKET6G | 6 | Altitude-varying gravity |
| `gravity_wgs84_full` | 📝 | ROCKET6G | 6 | Full WGS84 ellipsoidal with J2 |

### Atmosphere Models (2/3)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `atmosphere_constant` | ✅ | BALL3 | 3/6 | 🎯 Fixed ρ=1.225 kg/m³ |
| `atmosphere_us76` | ✅ | ROCKET6G | 3/6 | US Standard Atmosphere 1976 |
| `atmosphere_table` | 📝 | ROCKET6G | 3/6 | Tabular from weather deck |

### Wind Models (2/2)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `wind_none` | ✅ | BALL3 | 3/6 | 🎯 No wind (calm air) |
| `wind_constant` | ✅ | ROCKET6G | 3/6 | Constant wind vector |

**Variable Assignments:**
```cpp
// Standard environment indices
vehicle[0] = time (double) - Simulation time sec
vehicle[50] = grav (double) - Gravity magnitude m/s²
vehicle[51] = GRAVL (Matrix 3x1) - Gravity vector local m/s²
vehicle[52] = press (double) - Pressure Pa
vehicle[53] = rho (double) - Density kg/m³
vehicle[54] = vsound (double) - Speed of sound m/s
vehicle[56] = vmach (double) - Mach number
vehicle[57] = pdynmc (double) - Dynamic pressure Pa
vehicle[58] = tempk (double) - Temperature K
vehicle[62] = GRAVG (Matrix 3x1) - Gravity geocentric m/s²
vehicle[63] = grav (double) - Gravity magnitude m/s²
```

---

## Guidance (3/8 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `guidance_none` | ✅ | BALL3 | 3/6 | 🎯 Ballistic (no guidance) |
| `guidance_proportional_nav` | ✅ | AIM5 | 3/6 | PNG for homing missiles |
| `guidance_pitch_program` | ✅ | ROCKET6G | 6 | Time-based pitch angle guidance |
| `guidance_waypoint` | 📝 | CRUISE5 | 3/6 | Waypoint following |
| `guidance_ltg` | 📝 | ROCKET6G | 6 | Linear Tangent Guidance for ascent |
| `guidance_apn` | 📝 | SRAAM6 | 6 | Augmented PNG |
| `guidance_tpn` | 📝 | AGM6 | 6 | True PNG |
| `guidance_loft` | 📝 | CRUISE5 | 3 | Lofted trajectory |

**Variable Assignments:**
```cpp
// guidance_proportional_nav
vehicle[400] = mguide (int) - Guidance mode
vehicle[401] = gnav (double) - Navigation gain
vehicle[402] = gmax (double) - Max acceleration command g
vehicle[410] = APNA (Matrix 3x1) - Accel command m/s²
vehicle[411] = annx (double) - Normal accel command g
vehicle[412] = allx (double) - Lateral accel command g

// guidance_pitch_program
vehicle[485] = thtvdxcom (double) - Flight path angle command deg
vehicle[486] = npitch (int) - Number of pitch program points
vehicle[487-496] = time_table[10] - Time breakpoints sec
vehicle[497-506] = pitch_table[10] - Pitch breakpoints deg
```

---

## Kinematics (2/4 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `kinematics_3dof_flat` | ✅ | BALL3 | 3 | 🎯 Point mass, flat Earth |
| `kinematics_6dof` | ✅ | ROCKET6G | 6 | DCM integration with Euler angles |
| `kinematics_3dof_rotating` | 📝 | MAGSIX | 3 | Point mass, rotating Earth |
| `kinematics_6dof_quaternion` | 📝 | GHAME6 | 6 | Quaternion-based attitude |

**Variable Assignments:**
```cpp
// kinematics_3dof_flat
vehicle[20] = SBEL (Matrix 3x1) - Position in Earth frame m
vehicle[21] = VBEL (Matrix 3x1) - Velocity in Earth frame m/s
vehicle[22] = altitude (double) - Altitude above ground m
vehicle[23] = dvbe (double) - Speed m/s

// kinematics_6dof
vehicle[121] = TBI (Matrix 3x3) - DCM body to inertial
vehicle[130] = TBD (Matrix 3x3) - DCM body to geodetic
vehicle[131] = TDI (Matrix 3x3) - DCM geodetic to inertial
vehicle[137] = psibdx (double) - Yaw deg
vehicle[138] = thtbdx (double) - Pitch deg
vehicle[139] = phibdx (double) - Roll deg
vehicle[140] = alppx (double) - Total angle of attack deg
vehicle[141] = phipx (double) - Aerodynamic roll angle deg
vehicle[144] = alphax (double) - Angle of attack deg
vehicle[145] = betax (double) - Sideslip angle deg
vehicle[75] = dvba (double) - Airspeed m/s
```

---

## Navigation (3/5 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `target_fixed` | ✅ | AIM5 | 3/6 | 🎯 Stationary target |
| `target_const_velocity` | ✅ | AIM5 | 3/6 | Constant velocity target |
| `intercept_simple` | ✅ | AIM5 | 3/6 | Miss distance detection |
| `nav_gps_ins` | 📝 | ROCKET6G | 6 | GPS/INS Kalman filter |
| `nav_waypoint` | 📝 | CRUISE5 | 3/6 | Waypoint navigation |

**Variable Assignments:**
```cpp
// intercept_simple
vehicle[310] = miss (double) - Miss distance m
vehicle[311] = time_m (double) - Time at closest approach sec
vehicle[312] = tgo (double) - Time-to-go sec
vehicle[313] = hit_flag (int) - Hit detection flag
```

---

## Propulsion (4/5 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `thrust_constant` | ✅ | AIM5 | 3/6 | 🎯 Fixed thrust magnitude |
| `rocket_motor_simple` | ✅ | AIM5 | 3/6 | Constant thrust with burntime |
| `thrust_table` | ✅ | ROCKET6G | 3/6 | Thrust vs time from table |
| `propulsion_staging` | ✅ | ROCKET6G | 6 | Multi-stage with variable mass/inertia |
| `jet_engine` | 📝 | FALCON6 | 6 | Jet thrust vs alt/Mach |

**Variable Assignments:**
```cpp
// propulsion_staging
vehicle[10] = mprop (int) - Propulsion mode
vehicle[15] = vmass (double) - Vehicle mass kg
vehicle[17] = xcg (double) - CG location from nose m
vehicle[18] = IBBB (Matrix 3x3) - Inertia tensor kg·m²
vehicle[21] = fmass0 (double) - Initial fuel mass kg
vehicle[22] = fmasse (double) - Fuel mass expended kg
vehicle[24] = aexit (double) - Nozzle exit area m²
vehicle[25] = spi (double) - Specific impulse sec
vehicle[26] = thrust (double) - Thrust N
vehicle[28-29] = xcg_0, xcg_1 - Initial/final CG m
vehicle[38-41] = moi_roll/trans_0/1 - Initial/final MOI kg·m²
```

---

## Sensors (2/4 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `seeker_perfect` | ✅ | AIM5 | 3/6 | 🎯 Perfect target tracking |
| `gps_perfect` | ✅ | ROCKET6G | 3/6 | Perfect GPS sensor |
| `ins_simple` | 📝 | ROCKET6G | 6 | Inertial navigation system |
| `startrack_perfect` | 📝 | ROCKET6G | 6 | Star tracker for attitude |

**Variable Assignments:**
```cpp
// seeker_perfect
vehicle[250] = dta (double) - Range to target m
vehicle[251] = dvta (double) - Closing velocity m/s
vehicle[252] = UTAA (Matrix 3x1) - LOS unit vector
vehicle[253] = WOEA (Matrix 3x1) - LOS rate rad/s

// gps_perfect
vehicle[260] = SBELG (Matrix 3x1) - GPS position m
vehicle[261] = VBELG (Matrix 3x1) - GPS velocity m/s
vehicle[262] = altg (double) - GPS altitude m
```

---

## Utilities (2/2 extracted)

| Component | Status | Source | DoF | Description |
|-----------|--------|--------|-----|-------------|
| `time_management` | ✅ | ALL | 3/6 | 🎯 Simulation time tracking |
| `termination` | ✅ | ALL | 3/6 | Multi-criteria stop conditions |

**Variable Assignments:**
```cpp
// Standard time variables
vehicle[0] = time (double) - Simulation time sec
vehicle[1] = event_time (double) - Event timer sec
```

---

## Variable Index Allocation Guide

To avoid conflicts, use these index ranges:

| Range | Purpose | Components |
|-------|---------|-----------|
| 0-9 | Core | time, event_time, utility flags |
| 10-49 | Propulsion | mprop, thrust, mass, inertia, fuel |
| 50-99 | Environment & RCS | gravity, atmosphere, wind, RCS |
| 100-199 | Aerodynamics | coefficients, derivatives, modes |
| 200-249 | Dynamics | forces, moments, FAPB, FMB |
| 250-299 | Sensors | seeker, GPS, star tracker |
| 300-399 | Navigation | intercept, target, waypoint |
| 400-499 | Guidance | commands, modes, guidance law |
| 500-599 | Control | autopilot, control commands |
| 600-799 | Reserved | Future expansion |
| 800-999 | Actuators & Specialized | TVC, actuators, specialized |

---

## Status Summary

### Extraction Progress by Category

- ✅ Actuators: 2/4 (50%)
- ✅ Aerodynamics: 3/6 (50%)
- ✅ Control: 5/8 (63%)
- ✅ Dynamics: 4/6 (67%)
- ✅ Environment: 6/8 (75%)
- ✅ Guidance: 3/8 (38%)
- ✅ Kinematics: 2/4 (50%)
- ✅ Navigation: 3/5 (60%)
- ✅ Propulsion: 4/5 (80%)
- ✅ Sensors: 2/4 (50%)
- ✅ Utilities: 2/2 (100%)

**Total: 36 of ~55 components extracted (72% complete)**

### Ready-to-Build Simulations

- ✅ **BALL3**: Ballistic projectile (100%)
- ✅ **ROCKET6G**: Three-stage space launch vehicle (100%)
- 🟡 **Aircraft**: 6DoF aircraft (85% - needs waypoint guidance)
- 🟡 **Cruise Missile**: 6DoF cruise missile (90% - needs waypoint)

---

## Next Priority Extractions

1. guidance_waypoint (CRUISE5) - Enable autonomous flight paths
2. startrack_perfect (ROCKET6G) - Star tracker sensor
3. ins_simple (ROCKET6G) - Inertial navigation
4. control_autopilot_aircraft (FALCON6) - Aircraft autopilot
5. aerodynamics_aircraft (FALCON6) - Aircraft-specific aero
6. control_surface_actuator (FALCON6) - Aileron, elevator, rudder
7. guidance_ltg (ROCKET6G) - Linear tangent guidance
8. atmosphere_table (ROCKET6G) - Custom atmosphere
9. nav_gps_ins (ROCKET6G) - GPS/INS fusion
10. jet_engine (FALCON6) - Turbofan/turbojet model
