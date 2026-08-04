# Branch: falling body with drag — the same problem at 3 unknowns instead of 21

## Context

The CADAC work on `main` is finished and should stay that way. It answered its
question:

| | |
|---|---|
| analytical physics vs CADAC | 8e-06 m/s² at max-Q |
| aero identification error | 0.5% at max-Q |
| full-trajectory rollout | 79.6 m vs a 65.3 m integrator floor |
| identifiability at λ=0 | preds agree to 1.1%, **matrices disagree by 42%** |
| identifiability at λ=0.1 | matrix disagreement **5.4%**, costs 16% accuracy |

The last two rows are the finding worth building on. Prediction was never hard;
making the recovered matrix *mean* something is. And a three-stage orbital launch
vehicle with J2 gravity, thrust vectoring and staging is a punishing place to study
that — every experiment costs a Colab session, and the matrix has 21 free entries
you cannot inspect by eye.

This branch reruns the same investigation on a system small enough to check by
hand. It is not a rewrite and it does not replace anything: `main` keeps working,
and the branch reuses most of its code unchanged.

**Branch name:** `falling-body`.

---

## The system

A body dropped through air, one dimension:

```
state  x = [y, v]              n = 2

ydot = v
vdot = -g  -  (k/m)|v| v
        ^        ^
      known    unknown
```

In SDC form, with the exact answer written out:

```
        A = [ 0        1        ]        c = [  0  ]
            [ 0   -(k/m)|v|     ]            [ -g  ]
              ^         ^                       ^
           must be 0  the drag model        gravity
```

The split matches CADAC exactly: gravity is a constant force and lands in `c`;
drag is proportional to velocity and lands in `A`. Same structure, two states.

### Why this is the right size

`KinematicsModule` freezes row 0 (`ydot = v` is a definition), leaving:

| free | true value |
|---|---|
| `dA[1,0]` | **0** — drag does not depend on height |
| `dA[1,1]` | `-(k/m)\|v\|` — the drag model |
| `dc[1]` | **0** — gravity is already supplied |

**3 unknowns, 1 equation.** Underdetermined, exactly as on `main`, but you can
print all three numbers and compare them against a closed form. On CADAC the
equivalent is 21 numbers against 3 equations and no closed form to check.

The failure mode we measured should reproduce here: at low λ the model should push
force into `dA[1,0]` (multiplying a large `y`) or into `dc[1]`, rather than into
`dA[1,1]` where drag actually lives. That is the identical "position term dominates"
result from section 9, at a size where it is provable rather than inferred.

---

## What is reused unchanged

Confirmed by grep — the CADAC coupling is only in four places, and none of it is in
the core:

| already generic | why |
|---|---|
| `GrayBoxSSM` ([model.py](model.py)) | every shape derives from `layout.n_state` and `n_param`. Runs at n=2 as-is. |
| `graybox_loss` | no dimension assumptions. |
| `PhysicsModule`, `StateLayout` ([physics.py](physics.py)) | name→index only. |
| the conditioning sandwich | per-state scaling, dimension-free. |
| `Trainer` loop, checkpointing, scheduling | generic apart from one line. |

## What changed — six small edits (done)

| file | edit |
|---|---|
| [physics.py](physics.py) | `PhysicsModel.__init__` takes `modules=None`; a domain supplies its own registry. `known=None` then defaults to all-analytical rather than to CADAC's `DEFAULT_KNOWN`, which names subsystems a different registry does not have. |
| [trainer.py](trainer.py) | `Trainer(..., vel_slice=None)` — `s_slice("VBII")` was hardcoded in `bucket_metrics`. |
| [trainer.py](trainer.py) | `Trainer(..., buckets=Q_BUCKETS)`. Not in the original plan, but required: `Q_BUCKETS` is in pascals, so every falling-body sample lands in one bucket and the report says nothing. |
| [dataset.py](dataset.py) | `fspb` is now the last field, defaulting to `None`. `build_loaders` still **raises** for a dataset whose states start with `SBII` and has no `fspb` — that is the stale-clone trap and must stay loud — but merely skips it otherwise. |
| [evaluate.py](evaluate.py) | `n_pos`, `pos_label`, `pos_scale`, `truth_name` threaded through `evaluate` → `plot_evaluation`. The label matters: "geocentric radius (km)" is wrong, not just unhelpful, for a 1 km fall. |
| [identifiability.py](identifiability.py) | `run_sweep(..., q_name, q_threshold, vel_slice, buckets, modules, known)`. |

Every default reproduces `main`'s behaviour, and the gate confirms it: after all
six edits `python3 physics.py data/smoke4.npz` still prints
**0.0000 / 0.0719 / 0.5840 / 5.2489**.

## One new file

`fall.py` — simulator, two modules, gate, train and sweep. No CADAC, no
compilation, no Drive; 50 runs generate in about a minute on the Raspberry Pi.

**One deliberate change from the plan.** The plan said to disperse `k/m`. That is
wrong, and the reason is worth keeping: the network is memoryless and sees one
`(x, p)` at a time, so a coefficient that varies per run with no column to read it
from is not underdetermined — it is *unlearnable*, and the model would be scored on
a quantity no model could recover.

So `k` is **fixed** across runs (one body has one drag coefficient, as one vehicle
has one set of aero tables) and **mass** is dispersed instead and carried in `p`.
That is the exact analogue of CADAC, where `vmass` is a known parameter and the aero
coefficients are the unknown, and it keeps the problem non-trivial: the network has
to discover the `1/m` dependence rather than memorise one number.

`p = (mass, speed)`. `speed` is `|v|` — redundant with the state, and there only so
the bucketed reports have a column to key off, exactly as `pdynmc` serves on `main`.
It passes CLAUDE.md's test for what may enter `p`: it is derived from the state, not
from the unknown subsystem, so it carries no information about `k`.

`xdot` is stored from the closed form rather than finite-differenced. On `main`
differencing was unavoidable and cost a precision patch and a float64 discipline;
here exactness is free, so the only error left anywhere in the pipeline is the
model's.

---

## Experiments

| # | Question | Passes when |
|---|---|---|
| 1 | Does it recover drag at all? | `dA[1,1]` matches `-(k/m)\|v\|` pointwise to a few percent. Closed form, so this is exact — not a plausibility check. |
| 2 | Does the 21-for-3 pathology reproduce at 3-for-1? | At λ=0, print `dA[1,0]·y`, `dA[1,1]·v`, `dc[1]`. Expect the wrong terms to carry force, mirroring section 9's `SUSPECT`. |
| 3 | Does λ fix it here too? | Sweep λ. Expect a knee, and `dA[1,1]` converging on the closed form as `dA[1,0]`, `dc[1]` → 0. On CADAC we could only measure seed *agreement*; here we can check agreement **with the truth**. |
| 4 | Density varying with height | Replace drag with `-(k/m)ρ(y)\|v\|v`, `ρ = ρ0·exp(-y/H)`. Now `y` genuinely influences drag — but the correct SDC form still puts it in `dA[1,1]`'s coefficient, **not** in `dA[1,0]`. Tests whether the model attributes correctly when tempted. |

Experiment 4 is the direct analogue of the rocket: air density falls with altitude
there too, and `dA[3:6,0:3]` — the block that wrongly dominated — is exactly where
that temptation lives.

Stop at the first failure; later experiments assume the earlier ones hold.

### Experiments 1–2, measured (λ=0, 200 epochs, early stop at 146)

**Prediction is finished, and interpretation fails — the same split as `main`, now
provable.**

Prediction, on 7 held-out runs:

| | median accel error |
|---|---|
| analytical alone | 8.5316 m/s² |
| gray-box, all | **0.0068** |
| gray-box, \|v\| 40+ | **0.0060** against a 9.37 m/s² signal — 0.06% |

Rollout RMS **0.639 m against a 0.467 m Euler floor** (1.37×), versus 745.759 m for
analytical alone.

The matrix, at the same checkpoint:

```
dA[1,0] * y   (height-shaped)      1.1379   should be 0
dA[1,1] * v   (drag-shaped)        3.6560   should be everything
dc[1]         (offset)             2.7811   should be 0
|true drag|                        7.5428
force error   median               0.0092 m/s^2
dA[1,1] error median            8.001e-02 1/s (52.51% of truth)
frozen row 0  max |dA|+|dc|     0.000e+00   must be 0
```

**52% of the force sits in two blocks whose true value is exactly zero**, and
`dA[1,1]` — the drag model itself — is wrong by 50.2% even in the top speed bucket
where the signal is strongest:

| \|v\| | true `dA[1,1]` | error | rel |
|---|---|---|---|
| 0–5 | 0.00688 | 0.03176 | 461.6% |
| 5–20 | 0.03855 | 0.04855 | 125.9% |
| 20–40 | 0.09543 | 0.07237 | 75.8% |
| 40+ | 0.16294 | 0.08187 | **50.2%** |

Three things this settles that `main` could not:

- **The pathology is structural, not a CADAC artefact.** It reproduces at 3
  unknowns against 1 equation, with exact data, exact gravity and a two-state
  system. Nothing about launch vehicles, J2 or thrust vectoring was ever required.
- **It is now a measurement, not a suspicion.** On `main` the split was
  4.4832 / 0.2727 / 0.4061 and could only be called *suspect*, because no closed
  form said which block was right. Here the answer is written down, and the model
  is wrong by 50% while predicting to 0.06%.
- **The frozen row works exactly.** `max |dA[0,:]| + |dc[0]|` is `0.000e+00`, not
  small — the mask is doing precisely what it claims.

One incidental contrast worth keeping: `residual_scale` is `[1e-6, 7.85]`, and
because kinematics is exact in float32 the frozen row contributes *exactly* zero to
the MSE. So the "half the reported MSE is a constant" problem in CLAUDE.md's open
list does not exist here — `val_mse` is readable (0.00000 at best epoch). It is
still blind to the 52% misattribution above, which is the point.

Experiment 3 (the λ sweep) is next and belongs on Colab, not the Pi.

---

## Verification

- **Closed form.** Every quantity has an analytic value. `|dA[1,1] - (-(k/m)|v|)|`
  is the headline number, and CADAC can never provide its equivalent.
- **Frozen rows.** `dA[0,:]` and `dc[0]` exactly zero.
- **Truth attribution.** Learned drag vs the stored `truth` column, bucketed by
  `|v|` the way `main` buckets by `pdynmc`.
- **Seed agreement.** Reuse `identifiability._relative_spread` unchanged.
- **`main` unaffected.** Re-run `python3 physics.py data/smoke4.npz` and confirm the
  residual gate still prints 0.0000 / 0.0719 / 0.5840 / 5.2489 after the six edits.
  ✅ confirmed.

```bash
git checkout -b falling-body
python3 fall.py --generate -n 50 -o data/fall.npz
python3 fall.py --train --data data/fall.npz --epochs 200 --lambda-reg 0.0
python3 fall.py --train --data data/fall.npz --epochs 200 --lambda-reg 0.1
python3 fall.py --sweep --data data/fall.npz --lambdas 0 1e-3 1e-2 1e-1 1.0
```

### The gate, measured

`--generate` runs `residual_report` itself. On the 50-run set (55018 samples):

```
PhysicsModel(analytical=['kinematics', 'gravity'], learned=['drag'])
  kinematic row    max abs error 0.000e+00 m/s
  accel residual   median 8.9484  p99 9.8066 m/s^2
  vs closed form   max abs error 5.329e-15 m/s^2
  |v|     0-5     m/s  n=  1516  median   0.0197 m/s^2
  |v|     5-20    m/s  n=  4802  median   0.5022 m/s^2
  |v|    20-40    m/s  n=  9441  median   3.6198 m/s^2
  |v|    40-inf   m/s  n= 39259  median   9.5679 m/s^2
```

The third line is the one `main` cannot print. There, "the residual is
aerodynamics" rested on `FSPB`, an output of the same simulator; here it is checked
against a closed form and holds to **5.3e-15 m/s²** — machine precision. Every
later disagreement is therefore the model's, with nothing else left to blame.

Note the skew is the *opposite* of `main`'s: most of a fall is spent near terminal
velocity, so 71% of samples sit in the top bucket, where drag is 9.6 m/s² against
gravity's 9.8. On CADAC most of an ascent is near-vacuum where the signal is zero.
The unknown here is large and almost always present.

---

## Deliberately not in scope

- **Per-term matrices** (a matrix each for gravity, drag, thrust). Worth doing, but
  it is a different investigation and should wait until the 3-unknown case is
  understood.
- **Removing the module concept / a domain-agnostic core.** The five edits above
  are the useful 10% of that idea; the rest can follow if a third domain appears.
- **More states.** `n=2` until experiments 1–4 are settled.
- **Anything on `main`.** It is finished. The one open item there is a single
  confirming run at λ=0.1 on the full dataset, which does not block this branch.
