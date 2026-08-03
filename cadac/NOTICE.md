# CADAC simulations (third-party)

Simulation source for CADAC — Computer Aided Design of Aerospace Concepts — by
**Peter H. Zipfel**, supplemental material to *Modeling and Simulation of Aerospace
Vehicle Dynamics*. Redistributed here under the MIT license carried by the pyCAS
packaging (Copyright (c) 2019 clark3493).

Vendored so the framework builds without a network fetch: Colab runtimes are
ephemeral, and re-cloning CADAC every session is both slow and a dependency on a
third-party repository staying reachable. `generator.py` uses this directory by
default and falls back to cloning upstream if it is absent.

## What is here

The eleven example simulations, build artifacts removed
(`.sdf`, `.suo`, `.sln`, `.vcxproj*`, `.o`, `.exe`, `.ncb`, `.opt`, `.plg`).
8.6 MB of C++ sources, headers, `.asc` data decks and Makefiles.

`ROCKET6G` is the one this project uses — a three-stage launch vehicle flying to
orbital insertion, with propulsion staging, mass depletion, US76 atmosphere,
table-based aerodynamics, GPS/INS and LTG guidance. The others (BALL3, FALCON6,
SRAAM6, ADS6, AGM6, AIM5, CRUISE5, GHAME3, GHAME6, MAGSIX) are unused but kept
for later work.

## Provenance and verification

Taken from `github.com/yakaboskic/CADAC` (`example/`), which repackages Zipfel's
sources. That copy differs cosmetically from `github.com/missiondesignsolutions/CADAC`:
the MSVC `Matrix Matrix::` extra-qualification declarations are fixed and
`<cstring>` is included, so it compiles with a plain modern g++ and ships a
Makefile. Upstream needs `-fpermissive -include cstring` instead.

Those are compiler-visible changes, so the two were diff-checked rather than
assumed equivalent: identical `input.asc`, one built from this copy via its
Makefile at `-std=c++11`, one from upstream at `-fpermissive -std=c++03`. The
resulting `x`, `p`, `xdot` and `t` arrays are **bit-identical**. The cleanup is
cosmetic and either source can be used.

## Patched at build time, not here

These files are kept pristine. `generator.py` copies them into a work directory
and applies three patches to that copy (see its module docstring):

1. **Plot flags** — expose `SBII`/`VBII`/`ABII`/`FSPB`/`WBIB`/`FAPB`, taking the
   plot file from 108 to 126 columns. The stock output carries position and
   velocity only in polar form.
2. **Output precision**, 6 → 14 significant digits. `Hyper::plot_data` never sets
   stream precision, so `|SBII| ~ 6.37e6` m quantises to ±5 m — ±100 m/s of pure
   round-off once differenced.
3. **Column width**, 16 → 26. At 14 digits the numbers overflow the field and run
   together unparseably. The last two are a pair; neither works alone.
