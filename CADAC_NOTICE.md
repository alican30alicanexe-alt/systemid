# CADAC (third-party, bundled)

`CADAC/` is a checkout of <https://github.com/yakaboskic/CADAC>, which repackages
the CADAC — Computer Aided Design of Aerospace Concepts — simulations by
**Peter H. Zipfel**, supplemental material to *Modeling and Simulation of
Aerospace Vehicle Dynamics*. MIT licensed (`CADAC/LICENSE`, Copyright (c) 2019
clark3493).

Bundled rather than fetched so generation needs no network: Colab runtimes are
ephemeral, and cloning a third-party repository every session is slow and depends
on that repository staying reachable and unchanged. `generator.py` uses it
automatically via `VENDORED_CADAC` and falls back to cloning upstream if absent.

Committed as cloned, minus `.git/` and the `pycas/` Python package. pyCAS is not
used: its `compile()` ignores the component list entirely and rebuilds a hardcoded
BALL3 example, so `add_component()` has no effect on what runs. The CADAC
simulations it ships under `example/` are what this project needs.

## What this project uses

`CADAC/example/ROCKET6G` — a three-stage launch vehicle to orbital insertion, with
propulsion staging, mass depletion, US76 atmosphere, table-based aerodynamics,
GPS/INS and LTG guidance. The other ten (BALL3, FALCON6, SRAAM6, ADS6, AGM6,
AIM5, CRUISE5, GHAME3, GHAME6, MAGSIX) are unused but kept — FALCON6 is the
aircraft counterpart, and the framework is meant to cover both.

## Verified against upstream

This copy differs cosmetically from `missiondesignsolutions/CADAC`: the MSVC
`Matrix Matrix::` extra-qualification declarations are fixed and `<cstring>` is
included, so it builds with a plain modern g++ and ships per-example Makefiles.
Upstream needs `-fpermissive -include cstring` instead.

Those are compiler-visible changes, so the two were diff-checked rather than
assumed equivalent: identical `input.asc`, one built here via its Makefile at
`-std=c++11`, one from upstream at `-fpermissive -std=c++03`. The resulting `x`,
`p`, `xdot` and `t` arrays are **bit-identical**.

## Kept pristine

These files are not modified. `generator.py` copies ROCKET6G into a work directory
and patches that copy — plot flags, output precision 6 → 14 digits, column width
16 → 26. See its module docstring for why each is required.
