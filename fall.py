"""A falling body with drag: the identifiability question at 3 unknowns.

``main`` answered whether the gray-box split can identify CADAC's aerodynamics --
it can, to 0.5% at max-Q -- and then ran into the question it could not answer.
The recovered matrix put 4.48 m/s^2 of aerodynamic force in the *position* block
and 0.27 in the velocity block where drag actually lives, and raising
``lambda_reg`` collapsed the disagreement between seeds. But seeds agreeing with
each other is not the same as agreeing with the truth, and CADAC's aerodynamics
have no closed form to check against.

Here they do. The system is::

    state  x = [y, v]                  n = 2

    ydot = v
    vdot = -g  -  (k/m)|v| v
            ^        ^
          known    unknown

which in SDC form is::

        A = [ 0        1        ]        c = [  0  ]
            [ 0   -(k/m)|v|     ]            [ -g  ]
              ^         ^                       ^
           must be 0  the drag model        gravity

Same split as CADAC -- gravity is a constant specific force and lands in ``c``,
drag is proportional to velocity and lands in ``A``. :class:`KinematicsModule`
freezes row 0, so what is left free is:

===========  ==========================================
``dA[1,0]``  **0** -- drag does not depend on height
``dA[1,1]``  ``-(k/m)|v|`` -- the drag model
``dc[1]``    **0** -- gravity is already supplied
===========  ==========================================

Three unknowns against one equation. Underdetermined at every sample, exactly as
on ``main``, but small enough to print and check against a closed form.

What is known and what is not
-----------------------------
``k`` -- the lumped ``0.5 rho Cd S`` -- is the unknown, fixed across runs the way
one vehicle has one set of aero tables. ``mass`` is dispersed and lives in ``p``,
because mass is something you can weigh: it is the direct analogue of CADAC's
``vmass``, and it forces the network to discover the ``1/m`` dependence rather
than memorise a single number.

``speed`` is also in ``p``. It is ``|v|``, redundant with the state and derived
from it exactly -- but it is the analogue of ``pdynmc``, which is likewise derived,
and it is what the regime-bucketed reports key off. It carries no information
about ``k``, which is the test in CLAUDE.md's "never put a force column in ``p``":
a column is safe when an analytical term could legitimately compute it, and unsafe
when it is derived from the unknown subsystem.

    python3 fall.py --generate -n 50 -o data/fall.npz
    python3 fall.py --train --data data/fall.npz --epochs 200 --lambda-reg 0.0
    python3 fall.py --sweep --data data/fall.npz --lambdas 0 1e-3 1e-2 1e-1 1.0
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import torch
from torch import Tensor

from dataset import build_loaders
from evaluate import evaluate
from model import GrayBoxSSM
from physics import PhysicsModel, PhysicsModule, StateLayout
from trainer import TrainConfig, Trainer

#: Standard gravity, m/s^2. Supplied analytically; never learned.
G = 9.80665

#: The unknown: lumped ``0.5 * rho * Cd * S``, kg/m. Fixed across runs -- one body
#: has one drag coefficient, just as one vehicle has one set of aero tables. The
#: network never sees this, and every check in this module compares against it.
K_DRAG = 0.306

DEFAULT_STATE: tuple[str, ...] = ("y", "v")

#: ``mass`` is known (weighable, dispersed per run); ``speed`` is ``|v|``, carried
#: only so the regime-bucketed reports have a column to key off. See the module
#: docstring for why neither leaks the answer.
DEFAULT_PARAMS: tuple[str, ...] = ("mass", "speed")

#: Speed edges (m/s) for regime-resolved reporting -- the analogue of
#: ``trainer.Q_BUCKETS``. Drag goes as ``|v| v``, so these span a 250x range in
#: the quantity being identified, from "nothing to see" to "dominates gravity".
V_BUCKETS: tuple[float, ...] = (0.0, 5.0, 20.0, 40.0, float("inf"))


# --------------------------------------------------------------------------- #
# truth
# --------------------------------------------------------------------------- #

def drag_acceleration(x: Tensor | np.ndarray, mass: Tensor | np.ndarray):
    """``-(k/m)|v| v``, the quantity to be identified. Closed form, exact.

    This is what ``fspb`` is on ``main``, except that there it had to be read out
    of the simulator and here it is simply true.
    """
    v = x[..., 1]
    return -(K_DRAG / mass) * abs(v) * v


def drag_coefficient(x: Tensor, p: Tensor) -> Tensor:
    """The true ``dA[1,1]``, i.e. ``-(k/m)|v|``.

    ``drag_acceleration`` divided by ``v`` -- but written separately because this
    is the *matrix entry*, and the whole point of the branch is that the entry and
    the force it produces are different claims. A model can get the force right and
    the entry wrong; that is precisely what happened on ``main``.
    """
    return -(K_DRAG / p[:, 0]) * x[:, 1].abs()


# --------------------------------------------------------------------------- #
# analytical modules
# --------------------------------------------------------------------------- #

class KinematicsModule(PhysicsModule):
    """``dy/dt = v``. Exact and complete -- a definition, not a model."""

    name = "kinematics"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        A[:, layout.s("y"), layout.s("v")] += 1.0

    def exact_blocks(self, layout):
        row = layout.s("y")
        return [(slice(row, row + 1), slice(0, layout.n_state))]


class ConstantGravityModule(PhysicsModule):
    """``-g`` on the velocity row, in the affine term.

    Gravity does not vanish as ``x -> 0``, so forcing it through ``A x`` would need
    an entry scaling like ``g/|x|`` -- unbounded exactly where the body is slow.
    Same argument that puts thrust in ``c`` on ``main``.

    No J2, no latitude, no rotation: at these altitudes the flat-Earth constant is
    not an approximation being papered over, it is what the simulator integrates.
    That is the property the CADAC work had to fight for and this branch gets free.
    """

    name = "gravity"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        c[:, layout.s("v")] += -G


class DragModule(PhysicsModule):
    """Placeholder for the learned subsystem -- contributes nothing.

    It exists so ``known={"drag": False}`` reads as a statement about physics
    rather than an absence, and so switching drag to analytical later is a change
    of one flag.
    """

    name = "drag"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        return


class DragStructureModule(PhysicsModule):
    """Claims ``dA[1,0] = 0``: drag depends on velocity, not on height.

    Contributes no force -- it contributes *knowledge*, which is what
    ``exact_blocks`` is for. Freezing an entry is a statement that the physics
    determines it, and "the drag on a body does not depend on how high the body
    is" qualifies exactly as much as ``dy/dt = v`` does.

    This exists because of what :func:`minimum_norm_split` measures: the L2
    penalty cannot pick the physical factorisation, and no value of
    ``lambda_reg`` will make it. Structure can. Enable it together with
    ``learn_delta_c=False`` -- with row 0 already frozen, that switch removes the
    remaining offset channel ``dc[1]`` -- and the three-way ambiguity collapses to
    a single free entry, which is then forced to be the drag model.

    Note it is *false* under experiment 4, where density varies with height. That
    is the point of experiment 4: the correct SDC form still puts the height
    dependence in ``dA[1,1]``'s coefficient, so the claim survives. If it did not,
    freezing this would be the same sin as "correcting" CADAC's J2 sign.
    """

    name = "drag_structure"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        return

    def exact_blocks(self, layout):
        row, col = layout.s("v"), layout.s("y")
        return [(slice(row, row + 1), slice(col, col + 1))]


MODULES: dict[str, type[PhysicsModule]] = {
    "kinematics": KinematicsModule,
    "gravity": ConstantGravityModule,
    "drag": DragModule,
    "drag_structure": DragStructureModule,
}

#: Everything analytical except drag. ``drag_structure`` is off by default so the
#: unconstrained result stays the baseline the structured one is compared against.
DEFAULT_KNOWN: dict[str, bool] = {
    "kinematics": True, "gravity": True, "drag": False, "drag_structure": False,
}

#: Experiment 5: the same problem with one structural claim added.
STRUCTURED_KNOWN: dict[str, bool] = DEFAULT_KNOWN | {"drag_structure": True}


def make_physics(layout: StateLayout, known: dict[str, bool] | None = None) -> PhysicsModel:
    return PhysicsModel(layout, known=known or DEFAULT_KNOWN, modules=MODULES)


def minimum_norm_split(npz_path: str | Path) -> dict[str, float]:
    """Why ``lambda_reg`` cannot recover the true matrix. Closed form, no training.

    :func:`model.graybox_loss` penalises ``||a_tilde||^2 + ||c_tilde||^2``. On the
    velocity row the model must satisfy::

        F = r * ( a0 * (y/s0) + a1 * (v/s1) + c )

    with ``r = residual_scale[1]`` and ``s = state_scale``. Writing
    ``u = (y/s0, v/s1, 1)``, that is ``r * u.w = F``, whose minimum-norm solution is
    ``w = (F/r) u / ||u||^2`` -- so each channel carries ``F * u_i^2 / ||u||^2``.

    Two consequences, both measured below and neither fixable by tuning:

    - The conditioning sandwich normalises every channel to O(1). That is its job,
      and it is why ``dA`` is interpretable at all. But it also makes the three
      channels **equally cheap**, so nothing in the penalty prefers the physical one.
    - ``||(F/3, F/3, F/3)||^2 < ||(F, 0, 0)||^2``. Minimum-norm therefore *prefers*
      spreading the force over concentrating it. The regulariser is not failing to
      pick the truth; it is pushing away from it.

    So raising ``lambda_reg`` makes every seed spread the same way, which reads as
    "identified" on :mod:`identifiability` and is still wrong. Measured on the
    50-run set: seeds agree to 2.5% at lambda=0.1 while ``dA[1,1]`` is 51.8% off.
    """
    data = np.load(npz_path, allow_pickle=True)
    x, p = data["x"], data["p"]
    y, v = x[:, 0], x[:, 1]

    s0 = np.sqrt(np.mean(y ** 2))
    s1 = np.sqrt(np.mean(v ** 2))
    force = drag_acceleration(x, p[:, 0])
    r = np.sqrt(np.mean(force ** 2))

    u = np.stack([y / s0, v / s1, np.ones_like(y)], axis=1)
    carried = np.abs(force)[:, None] * u ** 2 / (u ** 2).sum(axis=1, keepdims=True)

    med = int(np.argsort(np.abs(force))[len(force) // 2])
    concentrated = np.mean((force / r / (v / s1)) ** 2)
    spread = np.mean(((force / r)[:, None] * u / (u ** 2).sum(axis=1, keepdims=True)) ** 2) * 3

    print(f"state_scale = [{s0:.3f}, {s1:.3f}]   residual_scale[1] = {r:.3f}")
    print("\ncost of carrying the whole force through one channel alone, median sample")
    print("  (equal means the penalty has no preference at all)")
    for name, col in zip(["dA[1,0] via y", "dA[1,1] via v", "dc[1]        "], range(3)):
        print(f"  {name}   {abs(force[med] / r / u[med, col]):.4f}")

    print("\nminimum-norm split of the true force (mean |.|, m/s^2)")
    for name, col in zip(["dA[1,0] * y", "dA[1,1] * v", "dc[1]      "], range(3)):
        print(f"  {name}   {carried[:, col].mean():7.4f}")
    print(f"  {'|true drag|':<12}  {np.abs(force).mean():7.4f}")

    print(f"\n  ||w||^2, all force in the true block : {concentrated:.4f}")
    print(f"  ||w||^2, minimum-norm spread         : {spread:.4f}")
    print(f"  the penalty prefers the wrong answer by {concentrated / spread:.2f}x")
    return {
        "concentrated_cost": float(concentrated),
        "spread_cost": float(spread),
        "preference": float(concentrated / spread),
    }


# --------------------------------------------------------------------------- #
# simulation
# --------------------------------------------------------------------------- #

def _deriv(state: np.ndarray, beta: float) -> np.ndarray:
    y, v = state
    return np.array([v, -G - beta * abs(v) * v])


def simulate(
    y0: float, v0: float, mass: float,
    int_step: float = 0.002, plot_step: float = 0.02, max_time: float = 200.0,
) -> dict[str, np.ndarray]:
    """One drop, RK4, recorded every ``plot_step``. Stops at ground contact.

    ``int_step`` is ten times finer than ``plot_step`` for the same reason CADAC's
    is: the recorded trajectory has to be right at the recorded times, and the
    rollout in :mod:`evaluate` compares against a forward-Euler floor taken at
    ``plot_step``. Integrating and recording at the same step would put the truth
    and the floor on the same footing and make the floor meaningless.

    ``xdot`` is stored from the closed form rather than finite-differenced. On
    ``main`` differencing was unavoidable and cost a precision patch; here exactness
    is free, so the only error left in the whole pipeline is the model's.
    """
    beta = K_DRAG / mass
    every = max(1, round(plot_step / int_step))

    state = np.array([y0, v0], dtype=np.float64)
    rows, times = [], []
    step = 0
    t = 0.0
    while t < max_time:
        if step % every == 0:
            rows.append(state.copy())
            times.append(t)
        if state[0] <= 0.0:
            break

        k1 = _deriv(state, beta)
        k2 = _deriv(state + 0.5 * int_step * k1, beta)
        k3 = _deriv(state + 0.5 * int_step * k2, beta)
        k4 = _deriv(state + int_step * k3, beta)
        state = state + (int_step / 6.0) * (k1 + 2 * k2 + 2 * k3 + k4)
        t += int_step
        step += 1

    x = np.asarray(rows)
    xdot = np.stack([_deriv(s, beta) for s in x])
    p = np.stack([np.full(len(x), mass), np.abs(x[:, 1])], axis=1)
    return {"x": x, "xdot": xdot, "p": p, "t": np.asarray(times)}


def generate(
    n_runs: int, out: Path, seed: int = 0,
    int_step: float = 0.002, plot_step: float = 0.02,
) -> Path:
    """Monte Carlo over drop height, initial velocity and mass.

    ``k`` is *not* dispersed. Dispersing it would make the drag acceleration a
    function of something the model cannot see: the network is memoryless and gets
    one ``(x, p)`` at a time, so a per-run coefficient it has no column for is not
    underdetermined, it is unlearnable. Mass is dispersed instead, and is in ``p``.
    """
    rng = np.random.default_rng(seed)
    xs, xdots, ps, ts, ids = [], [], [], [], []

    for run in range(n_runs):
        y0 = float(rng.uniform(300.0, 1500.0))
        v0 = float(rng.uniform(-30.0, 30.0))   # negative = thrown downward
        mass = float(rng.uniform(50.0, 150.0))
        traj = simulate(y0, v0, mass, int_step=int_step, plot_step=plot_step)

        xs.append(traj["x"])
        xdots.append(traj["xdot"])
        ps.append(traj["p"])
        ts.append(traj["t"])
        ids.append(np.full(len(traj["x"]), run))
        print(f"  run {run:3d}  y0={y0:7.1f} m  v0={v0:+6.1f} m/s  m={mass:6.1f} kg  "
              f"{len(traj['x']):5d} samples  {traj['t'][-1]:5.1f} s")

    data = {
        "x": np.concatenate(xs),
        "xdot": np.concatenate(xdots),
        "p": np.concatenate(ps),
        "t": np.concatenate(ts),
        "run_id": np.concatenate(ids),
        "state_names": np.array(DEFAULT_STATE),
        "param_names": np.array(DEFAULT_PARAMS),
        "dt": np.array(plot_step),
        # Stored so the file is self-describing, but never read back: every check
        # below recomputes it from the closed form.
        "truth": drag_acceleration(np.concatenate(xs), np.concatenate(ps)[:, 0]),
    }
    out = Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(out, **data)
    print(f"\n[generate] {len(data['x'])} samples from {n_runs} runs -> {out}")
    return out


# --------------------------------------------------------------------------- #
# gates and reports
# --------------------------------------------------------------------------- #

def residual_report(npz_path: str | Path, known: dict[str, bool] | None = None) -> Tensor:
    """What the analytical modules leave behind, bucketed by speed.

    Two checks, and the second is the one CADAC cannot offer:

    - the residual must be zero at rest and grow with ``|v|``, the same shape as
      the ``pdynmc`` gate on ``main``;
    - it must equal ``-(k/m)|v|v`` **exactly**, not merely correlate with speed.
      On ``main`` the equivalent check needed the simulator's own ``FSPB``, and it
      was the only thing that caught the thrust-vectoring error.
    """
    data = np.load(npz_path, allow_pickle=True)
    layout = StateLayout(list(data["state_names"]), list(data["param_names"]))
    x = torch.tensor(data["x"])
    p = torch.tensor(data["p"])
    t = torch.tensor(data["t"])
    xdot = torch.tensor(data["xdot"])

    model = make_physics(layout, known)
    residual = xdot - model.known_dynamics(x, p, t)
    accel = residual[:, layout.s("v")]
    truth = drag_acceleration(x, p[:, 0])

    print(model)
    print(f"  kinematic row    max abs error {residual[:, layout.s('y')].abs().max():.3e} m/s")
    print(f"  accel residual   median {accel.abs().median():.4f}  "
          f"p99 {accel.abs().quantile(0.99):.4f} m/s^2")
    print(f"  vs closed form   max abs error {(accel - truth).abs().max():.3e} m/s^2")

    speed = p[:, layout.p("speed")]
    for lo, hi in zip(V_BUCKETS[:-1], V_BUCKETS[1:]):
        sel = (speed >= lo) & (speed < hi)
        if sel.sum() > 10:
            print(f"  |v| {lo:>5.0f}-{hi:<5.0f} m/s  n={int(sel.sum()):6d}  "
                  f"median {accel[sel].abs().median():8.4f} m/s^2")
    return residual


@torch.no_grad()
def matrix_report(model: GrayBoxSSM, x: Tensor, p: Tensor) -> dict[str, float]:
    """Where the identified force sits, and whether the entry itself is right.

    Three numbers decide it, and they are the reason this branch exists::

        dA[1,0] * y   should be 0      -- drag does not depend on height
        dA[1,1] * v   should be all of it
        dc[1]         should be 0      -- gravity is already supplied

    On the 50-run CADAC dataset the equivalent split was 4.4832 / 0.2727 / 0.4061:
    the model predicted beautifully with 95% of the force in the wrong block. That
    could only be called suspicious there. Here ``dA[1,1]`` has a closed form, so
    "wrong block" is a measurement.
    """
    dA, dc = model.delta_matrices(x, p)
    y, v = x[:, 0], x[:, 1]

    a_pos = dA[:, 1, 0] * y
    a_vel = dA[:, 1, 1] * v
    a_off = dc[:, 1]
    truth = drag_acceleration(x, p[:, 0])
    coeff_true = drag_coefficient(x, p)

    total = a_pos + a_vel + a_off
    out = {
        "a_pos": float(a_pos.abs().mean()),
        "a_vel": float(a_vel.abs().mean()),
        "a_off": float(a_off.abs().mean()),
        "force_error": float((total - truth).abs().median()),
        "coeff_error": float((dA[:, 1, 1] - coeff_true).abs().median()),
        "coeff_rel": float(
            ((dA[:, 1, 1] - coeff_true).abs() / coeff_true.abs().clamp_min(1e-9)).median()
        ),
        "frozen_row": float(dA[:, 0, :].abs().max().item() + dc[:, 0].abs().max().item()),
    }

    print(f"\n{'='*70}\nWhere the identified force sits (mean |.|, m/s^2)\n{'='*70}")
    print(f"  dA[1,0] * y   (height-shaped)   {out['a_pos']:9.4f}   should be 0")
    print(f"  dA[1,1] * v   (drag-shaped)     {out['a_vel']:9.4f}   should be everything")
    print(f"  dc[1]         (offset)          {out['a_off']:9.4f}   should be 0")
    print(f"  |true drag|                     {float(truth.abs().mean()):9.4f}")
    print(f"\n  force error   median            {out['force_error']:9.4f} m/s^2")
    print(f"  dA[1,1] error median            {out['coeff_error']:9.3e} 1/s "
          f"({100 * out['coeff_rel']:.2f}% of truth)")
    print(f"  frozen row 0  max |dA|+|dc|     {out['frozen_row']:9.3e}   must be 0")

    speed = x[:, 1].abs()
    print(f"\n  {'|v| bucket':<16}{'|true dA11|':>13}{'|error|':>12}{'rel':>9}")
    for lo, hi in zip(V_BUCKETS[:-1], V_BUCKETS[1:]):
        sel = (speed >= lo) & (speed < hi)
        if sel.sum() > 10:
            err = (dA[sel, 1, 1] - coeff_true[sel]).abs().median()
            ref = coeff_true[sel].abs().median()
            rel = f"{100 * err / ref:.1f}%" if ref > 1e-9 else "--"
            print(f"  {lo:>5.0f}-{hi:<10.0f}{float(ref):>13.5f}{float(err):>12.5f}{rel:>9}")
    return out


# --------------------------------------------------------------------------- #
# entry points
# --------------------------------------------------------------------------- #

def train(args: argparse.Namespace) -> int:
    torch.manual_seed(args.seed)

    print()
    residual_report(args.data)

    train_loader, val_loader, test, _ = build_loaders(
        args.data, batch_size=args.batch_size, seed=args.seed
    )
    layout = StateLayout(test.state_names, test.param_names)
    physics = make_physics(layout, STRUCTURED_KNOWN if args.structured else None)
    vel = slice(layout.s("v"), layout.s("v") + 1)
    print(physics, "| free dA entries:", int(physics.free_mask().sum()))

    model = GrayBoxSSM.from_data(
        train_loader, physics, n_param=len(test.param_names), hidden=args.hidden,
        # With row 0 already frozen, this removes dc[1] -- the last channel that
        # can carry force without going through the drag entry.
        learn_delta_c=not args.structured,
    )
    print(f"[model] {sum(q.numel() for q in model.parameters())} parameters")

    cfg = TrainConfig(
        epochs=args.epochs, lr=args.lr, lambda_reg=args.lambda_reg,
        ckpt_dir=args.ckpt_dir, run_name=args.run_name,
    )
    trainer = Trainer(
        model, cfg, q_index=layout.p("speed"), vel_slice=vel, buckets=V_BUCKETS
    )
    trainer.fit(train_loader, val_loader)

    matrix_report(model, test.x, test.p)
    evaluate(
        test, model, physics,
        history_path=args.ckpt_dir / f"{args.run_name}_history.json",
        fig_dir=args.fig_dir,
        n_pos=1, truth_name="closed form",
        pos_label="height (m)", pos_scale=1.0,
    )
    return 0


def sweep(args: argparse.Namespace) -> int:
    from identifiability import print_table, run_sweep

    args.workdir.mkdir(parents=True, exist_ok=True)
    results = run_sweep(
        args.data, args.lambdas, args.seeds, args.epochs, args.workdir,
        q_name="speed", q_threshold=40.0, vel_slice=slice(1, 2),
        buckets=V_BUCKETS, modules=MODULES, known=DEFAULT_KNOWN,
    )
    print_table(results, len(args.seeds))
    print("\nOn this system the sweep is only half the answer: seed agreement says "
          "the matrices are consistent,\nwhile matrix_report says whether they are "
          "correct. Run --train at the chosen lambda for that.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--generate", action="store_true")
    mode.add_argument("--train", action="store_true")
    mode.add_argument("--sweep", action="store_true")
    mode.add_argument("--min-norm", action="store_true",
                      help="print why lambda_reg cannot recover the matrix")

    parser.add_argument("--structured", action="store_true",
                        help="experiment 5: freeze dA[1,0] and dc[1], leaving drag "
                             "as the only channel that can carry the force")

    parser.add_argument("--data", type=Path, default=Path("data/fall.npz"))
    parser.add_argument("-n", "--runs", type=int, default=50)
    parser.add_argument("-o", "--out", type=Path, default=Path("data/fall.npz"))
    parser.add_argument("--seed", type=int, default=0)

    parser.add_argument("--epochs", type=int, default=200)
    parser.add_argument("--batch-size", type=int, default=2048)
    parser.add_argument("--lr", type=float, default=2e-3)
    parser.add_argument("--lambda-reg", type=float, default=0.1)
    parser.add_argument("--hidden", type=int, nargs="+", default=[128, 128])
    parser.add_argument("--run-name", type=str, default="fall")
    parser.add_argument("--ckpt-dir", type=Path, default=Path("checkpoints"))
    parser.add_argument("--fig-dir", type=Path, default=Path("figures"))

    parser.add_argument("--lambdas", type=float, nargs="+",
                        default=[0.0, 1e-3, 1e-2, 1e-1])
    parser.add_argument("--seeds", type=int, nargs="+", default=[0, 1, 2])
    parser.add_argument("--workdir", type=Path, default=Path("checkpoints/fall_sweep"))
    args = parser.parse_args(argv)

    if args.generate:
        generate(args.runs, args.out, seed=args.seed)
        print()
        residual_report(args.out)
        return 0
    if args.min_norm:
        minimum_norm_split(args.data)
        return 0
    if args.sweep:
        return sweep(args)
    return train(args)


if __name__ == "__main__":
    raise SystemExit(main())
