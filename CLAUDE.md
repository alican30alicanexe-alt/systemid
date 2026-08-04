# systemid — gray-box state-space identification on CADAC ROCKET6G

Identify the translational dynamics of CADAC's three-stage launch vehicle in
state-dependent-coefficient form, with gravity, thrust and kinematics supplied
analytically and **only the aerodynamics learned**:

    xdot = (A_known + dA) x + (c_known + dc)

The point is not prediction accuracy. It is that `dA`/`dc` are interpretable and
that anything the analytical modules get wrong is silently absorbed into them —
which makes fidelity to CADAC the central correctness property of this repo.

---

## Current state (2026-08-04)

**The analytical physics is verified against CADAC and the residual gate passes.**
That was the open question; it is now closed by measurement rather than argument.

Measured on 4 local runs (75876 samples), gravity + propulsion reproduce
`newton.cpp`'s `~TBI*FSPB + ~TGI*GRAVG` to a median **4.0e-05 m/s²**, and to
**8e-06 m/s²** at max-Q. The residual left for the network is genuinely
aerodynamic:

| pdynmc | aero residual |
|---|---|
| 0–10 Pa | 0.0000 m/s² |
| 10–1000 Pa | 0.0725 |
| 1000–10000 Pa | 0.5465 |
| 10000+ Pa | **5.2530** |

Median over a whole ascent is 0.113 m/s². A 30-epoch run on 2 training
trajectories already takes max-Q error from 5.2654 to 0.5295 m/s², and
full-trajectory rollout RMS from 24363 m to 573 m against a 78 m integrator floor.

Two datasets exist locally, both gitignored: `data/smoke.npz` (2 runs) and
`data/smoke4.npz` (4 runs). Both are too small to train on — 4 runs splits to
2 train / 1 val / 1 test.

Next actions, in order:

1. Regenerate on Colab with [colab_generate_data.ipynb](colab_generate_data.ipynb).
   **Delete `MyDrive/systemid/data/chunks` first** — any existing chunk predates
   both the `fspb` array and the `etax`/`zetx` parameters, and the cache is keyed
   by index alone. `merge_datasets` now raises rather than merging such a chunk,
   but only for keys in `_STACKED`; a stale `param_names` is caught by the
   metadata check.
2. Confirm the log contains
   `[input] left at their deck values (later per-stage assignments): {'vmass0': 2, 'spi': 2}`.
   That line only exists in post-fix code; its absence means Colab cloned stale.
3. Confirm the mass plot falls monotonically, with **downward** steps at ~61 s
   and ~112 s (48984 → 15490 → 5024 kg). Verified 2026-08-04 on 8 ascents.
4. Train with [colab_train.ipynb](colab_train.ipynb), which enforces the residual
   gate before it will train. Or `python run.py --data <npz> --epochs 200`.
5. Sweep `lambda_reg`. At the default 1e-4 on 2 trajectories the identified `dA` is
   **not** yet interpretable: the position block dominates the velocity block at
   max-Q and `sym(dA[3:6,3:6])` is not negative-definite, i.e. the fit is good and
   the factorisation is arbitrary. This is the expected under-determined regime,
   not a bug — see `identifiability.py`.

A phone-readable version of this status lives at
<https://claude.ai/code/artifact/2cc52cac-e4be-4fc3-a997-accd26053f42>.

---

## Layout

| File | Role |
|---|---|
| [generator.py](generator.py) | CADAC → `.npz`. Fetch, patch C++, compile, Monte Carlo, parse. Imports no torch. |
| [physics.py](physics.py) | Analytical modules → `(A, B, c)`. Must match CADAC exactly. |
| [model.py](model.py) | `GrayBoxSSM`: analytical physics + learned correction, plus `graybox_loss`. |
| [dataset.py](dataset.py) | `.npz` → loaders, split by trajectory. |
| [trainer.py](trainer.py) | Training loop, checkpoints, per-dynamic-pressure metrics. |
| [evaluate.py](evaluate.py) | Trajectory rollout against CADAC, with an integrator-matched floor. |
| [identifiability.py](identifiability.py) | Multi-seed sweep: does the recovered matrix mean anything? |
| [run.py](run.py) | End-to-end entry point. |
| [colab_train.ipynb](colab_train.ipynb) | Colab training: gate → train → inspect `dA` → rollout → sweep. |
| [CADAC/](CADAC/) | Vendored upstream checkout. See [CADAC_NOTICE.md](CADAC_NOTICE.md). |

Working dirs `cadac_work/`, `data/`, `checkpoints/`, `figures/` are gitignored.

---

## Traps

Each of these has already caused a real, silent failure. They do not announce
themselves — the pipeline runs, produces plausible numbers, and is wrong.

### physics.py must reproduce CADAC, not the textbook

The training targets are CADAC's own trajectories. Any deviation in an
analytical module is learned as aerodynamics, which is exactly the quantity the
project exists to measure.

Concretely: `GravityJ2Module` is a transcription of `cad_grav84` plus the
`~TGI * GRAVG` rotation from `newton.cpp`. **CADAC's tangential J2 term carries
the opposite sign to the standard inertial-Cartesian formula** — a real
disagreement worth 0.030 m/s² at the launch latitude. That is small against the
5.25 m/s² at max-Q, but most of a 190 s ascent is near-vacuum where the
aerodynamic signal is exactly zero and 0.030 m/s² would be the whole measurement.
Do not "correct" it toward the textbook.

Before changing any module, verify it against the C++ in
`CADAC/example/ROCKET6G/` and check the residual gate afterwards.

### Thrust does not point along the body x-axis

`forces.cpp` has two branches. With `mtvc == 0` it adds thrust to `FAPB[0]`; with
`mtvc` in 1..3 it adds the gimballed vector `FPB` from `tvc.cpp`. `input_insertion.asc`
sets `mtvc 2` at `time > 10` and back to 0 at second-stage ignition, so the second
branch is live for the **whole first-stage boost — exactly where dynamic pressure
peaks**. Modelling thrust as `(T, 0, 0)` there left 0.42 m/s² unaccounted for
against 5.25 m/s² of aerodynamic force: a 7% bias that `dA` absorbed and reported
as aerodynamics.

This was invisible to the residual gate, because TVC is active precisely when
`pdynmc` is highest — the error *grew with dynamic pressure* and so passed a check
designed to catch flat residuals. Only comparing against `FSPB` found it.

`PropulsionModule` now transcribes `tvc.cpp` lines 118–120. One unconditional
expression covers both branches: CADAC holds `etax`/`zetx` at exactly zero when TVC
is inactive (verified across all four flight phases), and at zero the direction
cosines collapse to `(1, 0, 0)`. No `mtvc` flag is needed and none is plotted.

### Never put a force column in the parameter vector

`p` is fed to the network as input. `FSPB` and `FAPB` are built in `forces.cpp` as
`pdynmc * refa * (cx, cy, cz)` — they *contain* the aerodynamic force the model
exists to identify. Adding them to `DEFAULT_PARAMS` would let the network copy the
answer instead of learning it, and the loss would look excellent.

`FSPB` is therefore stored under its own `fspb` key, outside `p`, as ground truth
for analysis only. `etax`/`zetx` are safe in `p` — they are angles, and carry no
information about the aerodynamic coefficients.

The general rule: a column belongs in `p` only if an analytical module needs it to
compute a *known* term. Anything derived from the unknown subsystem is truth data,
not a parameter.

### Simulation time must reach the modules

Every inertial↔earth-fixed rotation in CADAC carries `GW_CLONG + WEII3 * time`.
Over a 190 s ascent the Earth turns 0.79°, which misdirects thrust by ~0.36 m/s²
— an order of magnitude above the signal.

`PhysicsModule.contribute()` takes `t` as a parameter for this reason. It was
once computed in `PhysicsModel.__call__` and then dropped, and nothing failed
loudly. Any new module must use it.

### Dispersion targets appear once per stage

`input_insertion.asc` assigns `vmass0` and `spi` **four times each**, once per
stage. `_disperse_line` therefore disperses only the *first* occurrence of a
name. Matching on the name alone rewrote stage 2's 15490 kg and stage 3's
5024 kg with stage 1's dispersed 48984 kg, replacing both upper stages with
copies of the booster — visible only as vehicle mass increasing after staging.

`write_input` now prints the skipped occurrences. Dispersing an upper stage
would require stage-scoped keys, which are not implemented.

### The chunk cache is keyed only by index

`generate_chunked` skips any `chunk_NNN.npz` that already exists, and
`merge_datasets` only checks state names, parameter names and `dt`. Changing the
seed, dispersions or `int_step` and re-running **silently merges stale chunks**.
Delete the chunk directory whenever the config changes.

### Never add `*.asc` to .gitignore

Upstream CADAC ships a `.gitignore` containing `*.asc`, which silently drops all
126 data decks — including the aero tables ROCKET6G cannot run without. The
defence is that `CADAC/.gitignore` stays deleted. See the long comment in
[.gitignore](.gitignore) for the verification commands.

### The three C++ patches are load-bearing and inseparable

`patch_source` does three things, all required:

- **Plot flags** expose the Cartesian state, which stock CADAC computes but does
  not write. Do not add 3×3 matrices — `Hyper::plot_data` calls `.vec()` on any
  uppercase name and would emit three silent zeros (`UNPLOTTABLE`).
- **Precision 6 → 14 digits.** At 6 significant digits `SBII` (~6.4e6 m)
  quantises to ±5 m, which differenced is ±100 m/s of pure round-off.
- **Width 16 → 26.** At 14 digits the columns overflow and run together with no
  separator. Precision without width produces an unparseable file; the two are a
  pair.

### Positions stay float64

float32 spacing at 6.4e6 m is 0.5 m. Anything that re-differences stored
positions in float32 reintroduces the quantisation noise the precision patch
exists to remove. The `.npz` is float64; `build_loaders` casts to float32 only
because nothing downstream re-differences.

### Split by trajectory, never by sample

Consecutive samples are one plot step apart and are near-duplicates. A
per-sample split puts each validation point beside a training point taken
milliseconds earlier and reports a flattering loss regardless of real
generalisation.

---

## Verification gates

Run these before trusting anything downstream.

**FD vs CADAC ABII** (printed per chunk by `build_dataset`). Compares our finite
differences against CADAC's own computed acceleration. Measured median 2.3e-06
m/s²; orders of magnitude worse means the precision patch did not take.

**Residual report** (`physics.residual_report`, also cell 4 of the training
notebook and the top of `run.py`). With aerodynamics the only unknown, the
acceleration residual must be near zero in near-vacuum and **grow with `pdynmc`**.
A residual flat across dynamic-pressure buckets means a physics module is wrong.
Measured values are in "Current state" above.

**FSPB comparison** — the gate that the residual report cannot be. `newton.cpp`
computes `ABII = ~TBI*FSPB + ~TGI*GRAVG`, so with gravity and thrust modelled
correctly the residual must equal `~TBI * (FSPB - FPB/vmass)` exactly. Measured
median 4.0e-05 m/s², 8e-06 at max-Q.

This is strictly stronger than the residual report, which only checks that the
leftover *correlates* with dynamic pressure. The TVC error grew with `pdynmc` and
passed the residual gate for that reason; only this comparison caught it. Run it
after touching any analytical module.

**Identifiability sweep** (`identifiability.py`). An SDC factorisation is
non-unique — 18 free entries and 3 offsets producing 3 accelerations. Low
prediction disagreement with high matrix disagreement across seeds means the model
fits well but is not identified, and the recovered matrices are arbitrary.
`lambda_reg` is the knob. **Not yet run on a real dataset**; the 4-run smoke test
shows the under-determined signature clearly, so expect this to matter.

---

## Reading CADAC names

Uppercase names are four concatenated fields: **quantity**, **body**,
**reference frame**, **coordinate frame**. `SBII` = displacement of Body wrt
Inertial origin, in Inertial coordinates. `TBI` = transformation of Body wrt
Inertial, i.e. maps inertial → body.

Quantities: `S` displacement, `V` velocity, `A` acceleration, `W` angular
velocity, `F` force, `T` transformation matrix.
Frames: `I` inertial (ECI), `E` earth-fixed, `D` geodetic local-level,
`G` geocentric local-level, `B` body.

Trailing digit = vector component. Trailing `x` on a lowercase scalar = the
degrees version (`lonx` deg, `lon` rad).

State is `SBII1..3` (m) and `VBII1..3` (m/s) — trajectory dynamics only, so
`A` is 6×6. `KinematicsModule` freezes the three position rows via `free_mask`,
leaving **18 free `dA` entries plus 3 `dc` offsets against 3 equations** per
sample. That ratio is the identifiability problem; `lambda_reg` is what makes the
answer unique.

Attitude is carried in the 16-element parameter vector, not the state: the
rotational subsystem is out of scope and Euler-rate kinematics are singular at
θ = 90°, which is the launch attitude.

---

## Open, deliberately unfixed

- **Half the reported MSE is a constant.** `residual_scale`'s position rows are
  the RMS of finite-difference noise, and those rows are frozen by `free_mask`,
  so they contribute ~1.0 each forever and the mean sits near 0.5. Gradients are
  unaffected (frozen rows have none), but the epoch MSE is nearly unreadable.
  Judge progress from the `[train] analytical:` vs `[train] gray-box:` lines,
  which report median acceleration error per dynamic-pressure regime.
- **Rollout is a batch-size-1 Python loop** (`evaluate._euler`): ~19,000 model
  calls per rollout at `plot_step=0.01`, three rollouts per test trajectory.
  Budget tens of minutes. Batching across trajectories would fix it.
- **`FSPB` is deliberately not in `trainer.py`.** It is carried through
  `dataset.py` and scored in section 9 of the training notebook, but adding it to
  `bucket_metrics` would be redundant: that metric is
  `(xdot_pred - xdot_true)[:, vel]`, and since aerodynamics is the only unknown,
  `xdot_true - xdot_known = aero_true` and `xdot_pred - xdot_known = aero_pred`.
  The existing number *is* the aero identification error. `FSPB` earns its keep
  where the decomposition matters — direction agreement, per-bucket relative
  error — not as a second name for the same scalar.
- **`FAPB` is plotted but unused**, and now redundant — `FSPB` is `FAPB/vmass`
  and is the form `newton.cpp` actually integrates. Dropping `FAPB` from
  `PLOT_FLAGS` would shrink `plot1.asc` by three columns at no cost.

The "~0.04 m/s² aerodynamic residual" figure that used to appear throughout the
docstrings was wrong by two orders of magnitude and has been replaced with
measured values everywhere (`physics.py`, `model.py`, `dataset.py`). It predated
the dispersion fix and the vehicle it was measured on had upper stages several
times too heavy.

---

## Conventions

- Prose in comments and docstrings explains **why**, especially why a
  non-obvious choice is the correct one. Do not strip it; it is the record of
  which failures have already been paid for.
- Physical constants are reproduced from `CADAC/example/ROCKET6G/global_constants.hpp`
  exactly, not from memory or a textbook.
- Numbers quoted in comments (error magnitudes, sample counts) are measured, not
  estimated. If you cannot measure it, do not quote it.
