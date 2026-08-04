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

## What changes — five small edits

| file | edit |
|---|---|
| [physics.py:394](physics.py#L394) | `PhysicsModel.__init__` takes `modules: Mapping[str, type] = MODULES` so a domain can supply its own registry. One argument. |
| [trainer.py:139](trainer.py#L139) | `s_slice("VBII")` is hardcoded. Take the slice from config, defaulting to current behaviour. |
| [dataset.py:132](dataset.py#L132) | `fspb` is required; make it optional so a non-CADAC dataset loads. |
| [evaluate.py:156-199](evaluate.py#L156-L199) | `[:, :3]` / `[:, 3:6]` hardcoded in `plot_evaluation`. `horizon_table` already takes `n_pos`; thread it through. |
| [identifiability.py:87](identifiability.py#L87) | `p("pdynmc")` and `s_slice("VBII")` hardcoded. Make both parameters. |

None of these change behaviour on `main` — every default reproduces what happens
today.

## One new file

`fall.py`, self-contained, roughly 200 lines:

- **Simulator.** RK4 on `[y, v]`, sampled at a fixed step. Monte Carlo over drop
  height, initial velocity, and `k/m`. Writes the existing `.npz` schema plus a
  `truth` array holding the exact drag acceleration per sample — the same role
  `fspb` plays on `main`, computed analytically instead of read from a simulator.
- **Two modules** using the existing `PhysicsModule` base: `KinematicsModule`
  (`A[0,1]=1`, claims row 0 as exact) and `ConstantGravityModule` (`c[1]=-g`).
  Drag is the placeholder, i.e. left to the network.
- **`main`** that runs gate → train → report, reusing `GrayBoxSSM`, `Trainer` and
  `graybox_loss`.

No CADAC, no compilation, no Drive. Runs on the Raspberry Pi in seconds.

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

---

## Verification

- **Closed form.** Every quantity has an analytic value. `|dA[1,1] - (-(k/m)|v|)|`
  is the headline number, and CADAC can never provide its equivalent.
- **Frozen rows.** `dA[0,:]` and `dc[0]` exactly zero.
- **Truth attribution.** Learned drag vs the stored `truth` column, bucketed by
  `|v|` the way `main` buckets by `pdynmc`.
- **Seed agreement.** Reuse `identifiability._relative_spread` unchanged.
- **`main` unaffected.** Re-run `python3 physics.py data/smoke4.npz` and confirm the
  residual gate still prints 0.0000 / 0.0719 / 0.5840 / 5.2489 after the five edits.

```bash
git checkout -b falling-body
python3 fall.py --generate -n 50 -o data/fall.npz
python3 fall.py --train --data data/fall.npz --epochs 200 --lambda-reg 0.0
python3 fall.py --train --data data/fall.npz --epochs 200 --lambda-reg 0.1
python3 fall.py --sweep --data data/fall.npz --lambdas 0 1e-3 1e-2 1e-1 1.0
```

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
