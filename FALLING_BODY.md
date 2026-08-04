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

### Experiment 3, measured — **λ does not fix it, and provably cannot**

This is the finding the branch was built to get, and it goes against what `main`
concluded.

Sweep, 3 seeds × 5 λ, 40 epochs:

| λ | pred disagr | matrix disagr | err maxQ | val mse |
|---|---|---|---|---|
| 0 | 0.0030 | 0.1893 | 0.0452 | 0.00004 |
| 1e-3 | 0.0030 | 0.1133 | 0.0474 | 0.00004 |
| 1e-2 | 0.0033 | **0.0289** | 0.0468 | 0.00004 |
| 1e-1 | 0.0034 | **0.0249** | 0.1476 | 0.00017 |
| 1 | 0.0192 | 0.0491 | 1.3960 | 0.01319 |

That reproduces `main` exactly: a knee at 1e-2 where matrix disagreement collapses
6.5× at no accuracy cost. On `main` this was read as "λ=1e-1 is where the matrix
becomes trustworthy."

It isn't. Training at λ=0.1 for 200 epochs:

```
dA[1,0] * y   (height-shaped)      2.0703   should be 0
dA[1,1] * v   (drag-shaped)        3.9214   should be everything
dc[1]         (offset)             1.4165   should be 0
dA[1,1] error median            7.380e-02 1/s (51.79% of truth)
```

**Seeds agree to 2.5%. The truth disagrees by 51.8%.** The drag block carries 52.9%
of the force against 48.3% at λ=0 — λ bought 4.6 points of attribution for 10×
worse accuracy (rollout 0.639 m → 6.162 m). It did not move the force into the
right block; it made every seed put it in the *same wrong* place.

#### Why — closed form, no training (`fall.py --min-norm`)

The penalty is `‖a_tilde‖² + ‖c_tilde‖²`, and on the velocity row the model must
satisfy `F = r·(a0·(y/s0) + a1·(v/s1) + c)`. Measured on the 50-run set:

```
cost of carrying the whole force through one channel alone, median sample
  dA[1,0] via y   0.9142
  dA[1,1] via v   0.8777
  dc[1]           1.1386

  ||w||^2, all force in the true block : 0.9366
  ||w||^2, minimum-norm spread         : 0.3683
  the penalty prefers the wrong answer by 2.54x
```

Two facts, neither tunable:

- **The conditioning sandwich makes every channel equally cheap.** That is its job
  — normalising each to O(1) is what makes `dA` interpretable at all — but it means
  nothing in the penalty prefers the physical channel. 0.914 / 0.878 / 1.139.
- **Minimum-norm prefers spreading.** `‖(F/3,F/3,F/3)‖² < ‖(F,0,0)‖²`, so the
  penalty scores the true concentrated answer 2.54× *worse* than the spread.

λ therefore does not fail to find the truth. It points away from it. And the
predicted min-norm split (1.54 / 2.99 / 2.60) has the same character as the
measured one (2.07 / 3.92 / 1.42): force in all three channels, drag largest but
nowhere near all.

#### What this costs `main`

`main`'s identifiability sweep is a real gate and it measures a real thing — but
it measures **consistency, not correctness**, and those come apart exactly where it
matters. CLAUDE.md already carried the caveat ("low matrix disagreement proves the
seeds agree with each other, not with the truth"). That caveat is now the headline:
the recommended λ=0.1 on ROCKET6G buys agreement between seeds and has no
established relationship to the true aerodynamics.

Nothing measured on `main` is wrong. The interpretation of one row of it was.

### Experiment 5 — structure instead of penalty ✅ **measured**

**One structural claim recovers the drag model exactly, and costs nothing. It is not
a trade of accuracy for readability — accuracy improved 1.8× over the best
unconstrained model.**

| | λ=0 unstructured | λ=0.1 unstructured | **structured, λ=0** |
|---|---|---|---|
| `dA[1,1]` rel error | 52.51% | 51.79% | **0.05%** |
| force off the drag block | 3.919 | 3.487 | 0.000 |
| force error, median | 0.0092 | 0.1470 | **0.0040** |
| accel error, all | 0.0068 | 0.1345 | **0.0038** |
| accel error, \|v\|>40 | 0.0060 | 0.1502 | **0.0034** |

```
dA[1,0] * y   (height-shaped)      0.0000   should be 0
dA[1,1] * v   (drag-shaped)        7.5398   should be everything
dc[1]         (offset)             0.0000   should be 0
|true drag|                        7.5428

  |v| bucket        |true dA11|     |error|      rel
      0-5               0.00688     0.00095    13.8%
      5-20              0.03855     0.00059     1.5%
     20-40              0.09543     0.00021     0.2%
     40-inf             0.16294     0.00007     0.0%
```

**Read the rows in the right order.** The top two are *forced*: with one channel,
`dA[1,1] = force/v`, and `0.0000` off-block is definitional, not evidence. The
bottom three are the measurement, and they are the surprise — the constraint was
predicted merely to be compatible with the data, and instead it **helps**. Removing
two of three degrees of freedom leaves the network one smooth scalar to represent
instead of three competing for the same force. It also stopped at `best epoch 200`,
still improving when it ran out of epochs, at λ=0 with nothing penalising anything.

The `|v| < 5` bucket keeps 13.8% relative error. That is honest weak determination
rather than misattribution: the true coefficient there is 0.00688 against a force of
~0, so `force/v` is nearly 0/0 and no method could do better.

#### What the branch establishes

1. **A magnitude penalty cannot recover the physical factorisation.** Not "does not
   in practice" — the conditioning sandwich makes every channel equally cheap by
   construction, and minimum-norm then prefers the spread over the truth by a
   measured 2.54×.
2. **A structural claim does, and is free.** 52% → 0.05% on the coefficient, with
   accuracy improving rather than degrading.
3. **Therefore interpretability of an SDC matrix comes from the mask, not the
   regulariser.** `lambda_reg` buys agreement between seeds; `exact_blocks` buys
   agreement with reality. `main` had the mechanism all along — it was freezing only
   the kinematic rows, which is a definition, when it could have been carrying
   physics.

### Experiment 5 as originally wired

If the penalty cannot pick the physical factorisation, the factorisation has to be
declared. `DragStructureModule` claims `dA[1,0] = 0` — *drag depends on velocity,
not on height* — through the same `exact_blocks` mechanism that freezes `dy/dt = v`.
Freezing an entry is a statement that physics determines it, and this claim
qualifies as much as the kinematic one. `learn_delta_c=False` removes `dc[1]`, the
last channel that can carry force without going through the drag entry.

That leaves **one free entry against one equation**, verified:

```
default    | free_mask = [[0, 0], [1, 1]] | free entries 2  (+ dc[1])
structured | free_mask = [[0, 0], [0, 1]] | free entries 1
```

Exactly determined. If the model fits at all, `dA[1,1]` *must* be `-(k/m)|v|`, and λ
should become nearly irrelevant.

⚠️ **This is algebra, not discovery, and the result must not be read as more.**
With one channel left, `dA[1,1] = force/v`, and `|force/v − (-(k/m)|v|)|` is
**1.49e-08** across the test set. Fitting the force and recovering the coefficient
are the same act. A clean experiment 5 therefore shows:

- **the structural claim is compatible with the data** — if drag genuinely depended
  on height, accuracy would collapse under the constraint, so the number to watch is
  `q40_inf` against the unstructured 0.1502, not `dA[1,1]`'s error;
- **the ambiguity is gone by construction**, leaving λ nothing to do.

It does *not* show that the network found the drag law. Nothing here could: the
network is being handed the only channel that exists.

```bash
python3 fall.py --train --data data/fall.npz --structured --lambda-reg 0.0
```

Section 10 of [colab_fall.ipynb](colab_fall.ipynb) runs it and prints the structured
and unstructured matrices side by side.

**The claim this branch is heading toward:** interpretability of an SDC matrix comes
from the mask, not the regulariser. On ROCKET6G the analogue is available and true —
aerodynamic force depends on velocity, density and attitude, not on inertial
position — so `dA[3:6,0:3]`, the block that wrongly carried 4.4832 m/s², is exactly
the block that should have been frozen.

Given the caveat above, the ROCKET6G experiment is **not** "does the matrix improve"
— freezing the block removes the misattribution by fiat, so it must. The question is
whether the 0.5%-at-max-Q accuracy survives the constraint. If it does, the block was
never carrying physics and the 4.4832 m/s² was pure factorisation slack. If accuracy
degrades, the constraint is false and something genuinely does depend on position —
which would itself be worth knowing.

At n=2 the answer was better than "survives": accuracy **improved 1.8×**. If that
transfers, ROCKET6G should get a readable drag model *and* beat its own 79.6 m
rollout, from a change that adds one `exact_blocks` entry and no parameters.

Concretely, on `main`: give `AerodynamicsModule` an `exact_blocks` returning
`[(s_slice("VBII"), s_slice("SBII"))]` — aerodynamic force depends on velocity,
density and attitude, not on inertial position — and set `learn_delta_c=False` if the
offset also proves unnecessary. Then re-run the max-Q bucket and the rollout. That is
the whole experiment.

### Two known weaknesses in the measurement

- **`residual_scale[0] = 1e-6` multiplies the frozen row by 1e6.** It is safe only
  because `dy/dt = v` is exactly representable in float32, verified: the row-0 error
  is `0.000e+00` and its share of the MSE is `0.000000e+00`, not merely small. Noisy
  `xdot` or an approximate kinematics module would detonate the loss. `GATE_OK`
  requires the row-0 residual below 1e-9, so it is caught — but the margin is zero by
  construction rather than by slack.
- **Checkpoints are selected on `val_mse`, which is blind to the matrix.** At low λ
  many epochs are indistinguishable in loss and quite different in factorisation, so
  the 2.0703 / 3.9214 / 1.4165 split carries an unquantified epoch-selection
  component. The conclusion is unaffected — 52% misattribution is not an epoch
  artefact, and it reproduced bit-identically across two runs — but the third decimal
  is not meaningful.

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
