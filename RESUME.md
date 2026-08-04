# Where we stopped — 2026-08-04

Written to be read cold, months later. Numbers are not repeated here; they live in
[CLAUDE.md](CLAUDE.md) and [FALLING_BODY.md](FALLING_BODY.md), which are canonical.
This file only says *where we are* and *what to do next*.

## State of the two branches

| branch | state |
|---|---|
| `main` | Finished and correct. Analytical physics verified against CADAC, aerodynamics identified to 0.5% at max-Q, rollout at 1.22× the integrator floor. **One recommendation on it was wrong and is now corrected in place** — see below. |
| `falling-body` | Finished. Answered the question `main` could not. This is the branch to read first. |

## The one-line result

**`lambda_reg` buys agreement between seeds. `exact_blocks` buys agreement with
reality.** The repo has had `exact_blocks` since the beginning and was spending it
only on the kinematic rows, which are a definition rather than physics.

Three findings, all measured on `falling-body`, all written up in
[FALLING_BODY.md](FALLING_BODY.md):

1. The misattribution `main` found is **structural**, not a CADAC artefact. It
   reproduces at 3 unknowns against 1 equation with exact data and a two-state system.
2. **λ provably cannot fix it.** The conditioning sandwich normalises every channel
   to O(1) — which is what makes `dA` readable *and* what makes every channel equally
   cheap — so minimum-norm prefers spreading the force over concentrating it. Raising
   λ makes every seed miss the same way, which the identifiability sweep then reports
   as "identified". Reproduce in seconds with `python3 fall.py --min-norm`.
3. **One structural claim fixes it and is free**, improving accuracy rather than
   costing it. That was the surprise: the constraint was expected merely to be
   compatible with the data.

⚠️ Do not read experiment 5's coefficient error as a discovery. With one channel left,
`dA[1,1] = force/v` is forced by algebra. The informative rows are the accuracy ones,
because those were *not* forced. FALLING_BODY.md marks which is which.

## What to do next

**Freeze `dA[3:6,0:3]` on ROCKET6G.** This is next-action 3 in CLAUDE.md, stated
concretely there. In short: give `AerodynamicsModule` an `exact_blocks` returning
`[(layout.s_slice("VBII"), layout.s_slice("SBII"))]` — aerodynamic force depends on
velocity, density and attitude, **not on inertial position** — and run at
`lambda_reg = 0`. Free entries drop 18 → 9.

**Judge it on the max-Q bucket and the rollout, never on the matrix.** The matrix is
guaranteed to improve, because the mask forces it; that is not evidence. If accuracy
holds or improves, the 4.4832 m/s² that block was carrying was pure factorisation
slack, and ROCKET6G gets a readable drag model *plus* a better rollout from a change
that adds no parameters. If accuracy degrades, the structural claim is false and
something genuinely depends on inertial position — which is worth knowing too.

That is a `main` change. Branch off `main`, not off `falling-body`.

## Practicalities

- **Never train on the Raspberry Pi.** Use Colab. Data generation, the residual
  gates and `--min-norm` are all fine locally and take seconds.
- `colab_fall.ipynb` needs no Drive and no compilation — it generates its data in the
  notebook in under a minute. `colab_train.ipynb` (ROCKET6G) does need Drive.
- Colab keeps a stale clone *and* a stale notebook. Cell 2 does `fetch` +
  `reset --hard` and purges `sys.modules`, which fixes the repo — but it cannot
  refresh the cell you are reading. If a schema assert fails on code that is
  obviously current, the **notebook** is the stale copy: re-open it from GitHub.
- `data/` is gitignored. `data/fall.npz` regenerates with
  `python3 fall.py --generate -n 50 -o data/fall.npz` and is deterministic at seed 0.
- Local runs need the venv: `.venv/bin/python`. System python has no torch.

## Verification before trusting anything

Unchanged, and all still passing — see the gates section of CLAUDE.md.
The cheapest check that `main` is intact:

```bash
.venv/bin/python physics.py data/smoke4.npz
# must print 0.0000 / 0.0719 / 0.5840 / 5.2489
```
