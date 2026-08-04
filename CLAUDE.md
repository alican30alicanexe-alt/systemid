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

Median over a whole ascent is 0.113 m/s².

### Trained: prediction is finished, interpretation is not

A 50-run dataset (948450 samples, 35 train / 8 val / 7 test) trained 100 epochs
identifies the aerodynamics essentially perfectly, and produces a matrix that
means nothing. Both halves are measured.

| pdynmc | \|a_true\| | \|error\| | rel |
|---|---|---|---|
| 0–10 Pa | 0.0000 | 0.0032 | — |
| 10–1000 Pa | 0.0727 | 0.0056 | 7.8% |
| 1000–10000 Pa | 0.7105 | 0.0122 | 1.7% |
| 10000+ Pa | 5.1913 | 0.0275 | **0.5%** |

`cos(a_ident, a_true)` at max-Q is **+1.000**. Full-trajectory rollout RMS is
**79.6 m against a 65.3 m integrator floor** — 1.22× the best any model could
achieve with forward Euler at this step, versus 24295 m for analytical alone.

But at `lambda_reg = 1e-4` the force sits in the wrong block:

```
dA[3:6,0:3] @ r  (gravity-shaped)   4.4832 m/s²
dA[3:6,3:6] @ v  (drag-shaped)      0.2727        <- ~5% of the force
dc[3:6]          (offset)           0.4061
```

Going from 4 runs to 50 made prediction 16× better and this split **worse** (the
drag block was 13% at 4 runs). That is not a bug: 21 unknowns against 3 equations
is underdetermined at every sample independently, so no amount of data resolves it.

### The sweep: `lambda_reg` fixes it, and 1e-4 was ~1000× too weak

3 seeds × 4 λ, on a 10-run subset at 15 epochs — 8.9 min total, because
identifiability is structural and does not need the full dataset.

| λ | pred disagr | matrix disagr | err maxQ |
|---|---|---|---|
| 0 | 0.0107 | 0.4179 | 0.1230 |
| 1e-3 | 0.0213 | 0.3998 | 0.1241 |
| 1e-2 | 0.0143 | **0.1941** | 0.1205 |
| 1e-1 | 0.0259 | **0.0540** | 0.1429 |

Three findings:

- **The λ=0 row is the textbook non-identified signature** — seeds agree on
  predictions to 1.1% and disagree on the matrix by 42%.
- **Ambiguity and physics live in different directions.** 0 → 1e-2 halves matrix
  disagreement at *no* accuracy cost. The feared outcome — accuracy collapsing
  before the ambiguity resolves — did not happen.
- **`val_mse` is blind to all of this**: 0.52561 / 0.52559 / 0.52556 / 0.52557
  while matrix disagreement moved 8×. Tuning λ on validation loss would conclude it
  does not matter. Only a multi-seed comparison sees it.

`λ = 1e-2` is free; `λ = 1e-1` buys the largest collapse in seed disagreement, for
16% accuracy on a model already at the integrator floor.

> ⚠️ **This section used to say λ=1e-1 is "where the matrix becomes trustworthy".
> That was wrong, and the `falling-body` branch measured it.**
>
> Low matrix disagreement proves the seeds agree **with each other**, and nothing
> more. On the falling body, where the true matrix *is* known in closed form, λ=0.1
> gives seeds that agree to 2.5% and a `dA[1,1]` that is **51.8% wrong** — barely
> better than the 52.5% at λ=0, for 10× worse accuracy.
>
> It is not bad luck. The penalty is `‖a_tilde‖² + ‖c_tilde‖²`, and the conditioning
> sandwich normalises every channel to O(1) — which is what makes `dA` readable at
> all, and also what makes every channel equally cheap. Since
> `‖(F/3,F/3,F/3)‖² < ‖(F,0,0)‖²`, minimum-norm **prefers spreading the force** over
> concentrating it, by a measured 2.54×. λ does not fail to find the physical
> factorisation; it points away from it, and raising λ only makes every seed miss
> the same way.
>
> Every number measured on `main` stands. The interpretation of this one row did
> not. See [FALLING_BODY.md](FALLING_BODY.md) for the closed-form derivation and
> `fall.py --min-norm` to reproduce it in seconds.

The consequence for `main`: the identifiability sweep is a real gate against
*inconsistency*, but it cannot certify correctness.

**What does work is structural, and the branch measured it.** Freezing the offending
block via `exact_blocks` — rather than penalising it — took the falling body's
coefficient error from **52.51% to 0.05%**, and *improved* accuracy at the same time
(median acceleration error 0.0068 → 0.0038 m/s², and 0.0060 → 0.0034 in the strongest
bucket) at λ=0, with nothing penalising anything. Constraining the model made it fit
better, because the frozen channels were carrying factorisation slack rather than
physics.

`lambda_reg` buys agreement between seeds; `exact_blocks` buys agreement with
reality. This repo has had the mechanism since the start and has been using it only
for the kinematic rows, which are a definition — when it could be carrying physics.

Two datasets exist locally, both gitignored: `data/smoke.npz` (2 runs) and
`data/smoke4.npz` (4 runs). Both are too small to train on — 4 runs splits to
2 train / 1 val / 1 test.

Next actions, in order:

1. ~~One 100-epoch run at `lambda_reg = 0.1`~~ — **no longer worth a Colab session.**
   The sweep predicted the force would move into the velocity block. On the falling
   body, where the same prediction can be checked against a closed form, it does not:
   λ=0.1 moved the drag block from 48.3% to 52.9% of the force while `dA[1,1]` stayed
   ~52% wrong. Running it on ROCKET6G would produce a number with no way to tell
   whether it means anything, which is the exact trap this project exists to avoid.
2. ✅ [FALLING_BODY.md](FALLING_BODY.md), experiment 5 — done, and it worked: one
   structural claim took the coefficient error from 52.51% to 0.05% while *improving*
   accuracy 1.8×.
3. **Freeze `dA[3:6,0:3]` on ROCKET6G.** Give `AerodynamicsModule` an `exact_blocks`
   returning `[(layout.s_slice("VBII"), layout.s_slice("SBII"))]` — aerodynamic force
   depends on velocity, density and attitude, **not on inertial position** — and
   re-run at `lambda_reg = 0`. That drops the free entries from 18 to 9 and removes
   the block that wrongly carried 4.4832 m/s².

   The matrix is *guaranteed* to look better, so it is not the test. The test is the
   max-Q bucket and the rollout: at n=2 the constraint improved both, and if that
   transfers, ROCKET6G gets a readable drag model *and* beats its own 79.6 m rollout
   from a change that adds no parameters. If accuracy degrades instead, the claim is
   false and something really does depend on inertial position — also worth knowing.

To regenerate data (only if the schema changes again): run
[colab_generate_data.ipynb](colab_generate_data.ipynb) and **delete
`MyDrive/systemid/data/chunks` first** — the cache is keyed by index alone, so a
schema change does not invalidate it. Cell 4 now asserts the schema at import; cell
14 runs both gates and prints an OK/FAIL verdict.

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
| [FALLING_BODY.md](FALLING_BODY.md) | Plan and results for the `falling-body` branch: the same identifiability question at n=2, where the true matrix is known in closed form. |
| [fall.py](fall.py) | **branch only.** Falling body with drag: simulator, two modules, gate, train, sweep. Imports no CADAC. |
| [colab_fall.ipynb](colab_fall.ipynb) | **branch only.** Colab for `fall.py`. No Drive, no compilation — data generates in the notebook. |
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

### Colab keeps a stale clone

Cell 4 of both notebooks used to clone only `if not (CODE/'*.py').exists()`. A
runtime that had cloned earlier — in a previous session, or before a push — kept
running old code while the notebook itself was current. Nothing said so: the cell
*printed* `DEFAULT_PARAMS` and no one compared it against what the code expected.

That produced a complete 50-run, 948450-sample dataset against the pre-`FSPB`
schema, which only surfaced at the validation cell after generation had finished.
The tell was `params:` listing 14 names with no `etax`/`zetx`.

Both notebooks now `fetch` + `reset --hard` unconditionally, purge the affected
entries from `sys.modules` (a re-run otherwise keeps the stale module objects and
the refresh achieves nothing), print the commit, and **assert the schema in cell 4**.
The clone costs seconds; the generation it protects costs hours.

The general rule: check the schema at the point of import, not at the point of use.

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
`lambda_reg` is the knob.

Run 2026-08-04 on the 50-run dataset; numbers in "Current state" above. It found
the non-identified signature at λ=0 (1.1% prediction spread, 42% matrix spread) and
showed λ=1e-1 collapses that to 5.4%. **Re-run it after any change that alters what
the network must explain** — a new analytical module, a state or parameter change,
or a different loss. It is cheap if you shrink the problem first: identifiability is
structural, so a 10-run subset at 15 epochs answers it in ~9 minutes where the full
dataset takes hours.

This gate is the only one that can see the difference between "fits well" and
"means something". `val_mse` cannot: it moved in the fourth decimal place across a
sweep where matrix disagreement changed 8×.

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
