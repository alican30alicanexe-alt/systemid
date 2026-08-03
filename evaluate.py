"""Trajectory rollout and plotting against CADAC ground truth.

One-step derivative error is not the quantity of interest -- what matters is
whether the identified model reproduces a trajectory when integrated forward. This
module rolls out unseen CADAC runs with forward Euler and compares.

The integrator-matched floor
----------------------------
CADAC integrates at ``int_step`` with a second-order scheme; we roll out with
forward Euler at ``plot_step``. A **perfect** model therefore still diverges from
the CADAC trajectory, purely from discretisation. Reporting model error against
CADAC without accounting for that attributes integration error to the model.

So every comparison includes a floor: the same Euler stepper driven by CADAC's own
derivatives. Anything between a model's curve and the floor is model error;
the floor itself is the best any model could do with this integrator. If a model
sits on the floor, tightening the model cannot help -- only a better integrator or
a smaller step can.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Sequence

import numpy as np
import torch
from torch import Tensor

from dataset import TrajectoryDataset
from model import GrayBoxSSM
from physics import PhysicsModel

#: Horizons (seconds) at which rollout error is tabulated. A single end-of-run
#: number conflates a model that tracks well then loses lock with one that is
#: mediocre throughout.
HORIZONS: tuple[float, ...] = (1.0, 5.0, 20.0, 60.0, float("inf"))


@dataclass
class Rollout:
    """One integrated trajectory."""

    name: str
    t: np.ndarray
    x: np.ndarray

    def error(self, truth: "Rollout") -> np.ndarray:
        return np.linalg.norm(self.x - truth.x, axis=1)


def _euler(
    x0: Tensor, t: Tensor, derivative: Callable[[Tensor, int], Tensor]
) -> Tensor:
    """Forward Euler over the recorded time base.

    Steps are taken from consecutive ``t`` values rather than a constant ``dt``, so
    the gaps left by staging-transient masking are integrated at their true width
    instead of silently shortening the trajectory.
    """
    xs = [x0]
    x = x0
    for k in range(len(t) - 1):
        x = x + (t[k + 1] - t[k]) * derivative(x, k)
        xs.append(x)
    return torch.stack(xs)


@torch.no_grad()
def rollout_model(model: GrayBoxSSM, traj: dict[str, Tensor]) -> Tensor:
    """Integrate the learned model. ``p`` is a known schedule, taken from the run."""
    model.eval()

    def f(x: Tensor, k: int) -> Tensor:
        xdot, _ = model(x.unsqueeze(0), traj["p"][k:k + 1], traj["t"][k:k + 1])
        return xdot.squeeze(0)

    return _euler(traj["x"][0], traj["t"], f)


@torch.no_grad()
def rollout_analytical(physics: PhysicsModel, traj: dict[str, Tensor]) -> Tensor:
    """Integrate analytical physics alone -- the no-learning reference."""
    def f(x: Tensor, k: int) -> Tensor:
        return physics.known_dynamics(
            x.unsqueeze(0), traj["p"][k:k + 1], traj["t"][k:k + 1]
        ).squeeze(0)

    return _euler(traj["x"][0], traj["t"], f)


@torch.no_grad()
def rollout_floor(traj: dict[str, Tensor]) -> Tensor:
    """Euler driven by CADAC's own derivatives: the achievable floor.

    Uses the true derivative *at the true state* each step, so the only error
    accumulated is Euler truncation. No model can beat this at this step size.
    """
    return _euler(traj["x"][0], traj["t"], lambda x, k: traj["xdot"][k])


def evaluate_trajectory(
    traj: dict[str, Tensor], model: GrayBoxSSM, physics: PhysicsModel
) -> tuple[Rollout, list[Rollout]]:
    """Returns ``(truth, [floor, analytical, gray-box])`` for one run."""
    t = traj["t"].numpy()
    truth = Rollout("CADAC", t, traj["x"].numpy())
    return truth, [
        Rollout("Euler floor", t, rollout_floor(traj).numpy()),
        Rollout("analytical", t, rollout_analytical(physics, traj).numpy()),
        Rollout("gray-box", t, rollout_model(model, traj).numpy()),
    ]


def horizon_table(
    truth: Rollout, rollouts: Sequence[Rollout], n_pos: int = 3
) -> dict[str, dict[str, float]]:
    """RMS position error at each horizon in :data:`HORIZONS`."""
    elapsed = truth.t - truth.t[0]
    table: dict[str, dict[str, float]] = {}
    for roll in rollouts:
        err = np.linalg.norm(roll.x[:, :n_pos] - truth.x[:, :n_pos], axis=1)
        table[roll.name] = {
            ("full" if np.isinf(h) else f"{h:g}s"): float(
                np.sqrt(np.mean(err[elapsed <= h] ** 2))
            )
            for h in HORIZONS
            if (elapsed <= h).sum() > 1
        }
    return table


def print_horizon_table(table: dict[str, dict[str, float]]) -> None:
    horizons = list(next(iter(table.values())))
    print(f"\n{'RMS position error (m)':<22}" + "".join(f"{h:>13}" for h in horizons))
    for name, row in table.items():
        print(f"{name:<22}" + "".join(f"{row[h]:>13.3f}" for h in horizons))


def plot_evaluation(
    truth: Rollout,
    rollouts: Sequence[Rollout],
    history: dict | None = None,
    out_path: Path = Path("figures/evaluation.png"),
) -> Path:
    """Six-panel summary: position, velocity, errors, and loss history."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, axes = plt.subplots(2, 3, figsize=(18, 9))
    elapsed = truth.t - truth.t[0]
    colors = {"Euler floor": "0.55", "analytical": "tab:orange", "gray-box": "tab:blue"}
    radius = lambda x: np.linalg.norm(x[:, :3], axis=1) / 1e3
    speed = lambda x: np.linalg.norm(x[:, 3:6], axis=1)

    ax = axes[0, 0]
    ax.plot(elapsed, radius(truth.x), "k-", lw=2.5, label="CADAC", zorder=5)
    for r in rollouts:
        ax.plot(elapsed, radius(r.x), lw=1.4, color=colors.get(r.name), label=r.name)
    ax.set_ylabel("geocentric radius (km)")
    ax.set_title("Position")
    ax.legend()

    ax = axes[0, 1]
    ax.plot(elapsed, speed(truth.x), "k-", lw=2.5, label="CADAC", zorder=5)
    for r in rollouts:
        ax.plot(elapsed, speed(r.x), lw=1.4, color=colors.get(r.name), label=r.name)
    ax.set_ylabel("inertial speed (m/s)")
    ax.set_title("Velocity")

    ax = axes[0, 2]
    for r in rollouts:
        ax.semilogy(
            elapsed, np.linalg.norm(r.x[:, :3] - truth.x[:, :3], axis=1) + 1e-9,
            lw=1.4, color=colors.get(r.name), label=r.name,
        )
    ax.set_ylabel("|position error| (m)")
    ax.set_title("Position error vs CADAC")
    ax.legend()

    ax = axes[1, 0]
    for r in rollouts:
        ax.semilogy(
            elapsed, np.linalg.norm(r.x[:, 3:6] - truth.x[:, 3:6], axis=1) + 1e-9,
            lw=1.4, color=colors.get(r.name), label=r.name,
        )
    ax.set_ylabel("|velocity error| (m/s)")
    ax.set_title("Velocity error vs CADAC")

    ax = axes[1, 1]
    for r in rollouts:
        if r.name == "Euler floor":
            continue
        ratio = (
            np.linalg.norm(r.x[:, :3] - truth.x[:, :3], axis=1)
            / np.maximum(np.linalg.norm(rollouts[0].x[:, :3] - truth.x[:, :3], axis=1), 1e-9)
        )
        ax.semilogy(elapsed, ratio, lw=1.4, color=colors.get(r.name), label=r.name)
    ax.axhline(1.0, color="0.55", ls="--", label="floor")
    ax.set_ylabel("error / floor error")
    ax.set_title("Model error above the integrator floor")
    ax.legend()

    ax = axes[1, 2]
    if history:
        ax.semilogy(history["train_mse"], label="train")
        ax.semilogy(history["val_mse"], label="val")
        ax.set_xlabel("epoch")
        ax.set_ylabel("normalised MSE")
        ax.legend()
    ax.set_title("Loss history")

    for ax in axes.flat:
        ax.grid(alpha=0.3)
        if ax.get_xlabel() == "":
            ax.set_xlabel("time since launch (s)")

    fig.tight_layout()
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=130)
    plt.close(fig)
    print(f"[plot] {out_path}")
    return out_path


def evaluate(
    test: TrajectoryDataset,
    model: GrayBoxSSM,
    physics: PhysicsModel,
    history_path: Path | None = None,
    fig_dir: Path = Path("figures"),
) -> dict[str, dict[str, float]]:
    """Roll out every test trajectory, print horizon tables, plot the first."""
    history = (
        json.loads(Path(history_path).read_text())
        if history_path and Path(history_path).exists()
        else None
    )

    aggregate: dict[str, list[dict[str, float]]] = {}
    for i, traj in enumerate(test.trajectories()):
        truth, rollouts = evaluate_trajectory(traj, model, physics)
        table = horizon_table(truth, rollouts)
        print(f"\n=== test run {int(traj['run_id'][0])} "
              f"({len(truth.t)} steps, {truth.t[-1] - truth.t[0]:.1f} s) ===")
        print_horizon_table(table)
        for name, row in table.items():
            aggregate.setdefault(name, []).append(row)
        if i == 0:
            plot_evaluation(truth, rollouts, history, Path(fig_dir) / "evaluation.png")

    # Intersect rather than assume rows[0]'s keys cover every run: horizon_table
    # drops a horizon holding fewer than two samples, so a run short enough to
    # miss one has fewer columns than the first. That needs a nearly empty
    # trajectory at the default plot_step, but the assumption is silent when it
    # breaks and costs nothing to remove.
    shared = set.intersection(*(set(r) for rows in aggregate.values() for r in rows))
    order = [h for h in next(iter(aggregate.values()))[0] if h in shared]
    mean = {
        name: {h: float(np.mean([r[h] for r in rows])) for h in order}
        for name, rows in aggregate.items()
    }
    print(f"\n=== mean over {len(next(iter(aggregate.values())))} test runs ===")
    print_horizon_table(mean)
    return mean