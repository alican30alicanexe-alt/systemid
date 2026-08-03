"""Is the recovered state-space model identified, or merely a good fit?

An SDC factorisation is non-unique: for ``n=6`` there are 36 free entries producing
6 outputs, so infinitely many ``A`` satisfy ``A x = f(x)`` at any sample. A model
can therefore fit perfectly while the matrix it reports is arbitrary among the
equally-good solutions -- and the loss curve gives no warning at all.

The test is to train several seeds and compare, at matched ``(x, p)``, two things:

``prediction disagreement``
    Spread of ``dA x + dc`` across seeds. Expected to be small whenever the models
    fit -- this measures agreement on the dynamics, not on the factorisation.

``matrix disagreement``
    Spread of the conditioned ``A_tilde`` itself. **This is the identifiability
    question.** Comparison is done on ``A_tilde`` rather than the physical ``dA``
    because the scaling sandwich gives ``dA`` entries wildly different magnitudes,
    which would make a Frobenius norm report mostly the scaling.

Low prediction disagreement with high matrix disagreement is the diagnostic
signature of a non-identified model: the seeds agree on the physics and disagree
on the matrices, which is exactly the failure that makes "interpretable" hollow.
Raising ``lambda_reg`` selects the minimum-norm correction and collapses the
ambiguity -- the sweep finds the largest lambda that buys that without costing
accuracy.

    python3 identifiability.py --data data/rocket6g.npz
"""

from __future__ import annotations

import argparse
import itertools
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import torch
from torch import Tensor

from dataset import build_loaders
from model import GrayBoxSSM, graybox_loss
from physics import PhysicsModel, StateLayout
from trainer import TrainConfig, Trainer


@dataclass
class SweepResult:
    lambda_reg: float
    pred_disagreement: float   # relative, across seeds
    matrix_disagreement: float # relative, across seeds
    median_error_maxq: float   # m/s^2, held-out
    median_error_all: float    # m/s^2, held-out
    val_mse: float


@torch.no_grad()
def probe(model: GrayBoxSSM, x: Tensor, p: Tensor) -> tuple[Tensor, Tensor]:
    """``(A_tilde, correction)`` at fixed probe points."""
    model.eval()
    a_tilde, c_tilde = model.corrections(x, p)
    correction = model.residual_scale * (
        torch.bmm(a_tilde, (x / model.state_scale).unsqueeze(-1)).squeeze(-1) + c_tilde
    )
    return a_tilde, correction


def _relative_spread(values: list[Tensor]) -> float:
    """Mean pairwise distance across seeds, normalised by the typical magnitude.

    0 means the seeds are identical; 1 means they differ as much as they are large.
    """
    scale = torch.stack([v.pow(2).mean().sqrt() for v in values]).mean().clamp_min(1e-12)
    pairs = [
        (a - b).pow(2).mean().sqrt()
        for a, b in itertools.combinations(values, 2)
    ]
    return float(torch.stack(pairs).mean() / scale)


def run_sweep(
    data: Path, lambdas: list[float], seeds: list[int],
    epochs: int, workdir: Path,
) -> list[SweepResult]:
    train_loader, val_loader, test, _ = build_loaders(data, batch_size=2048, seed=0)
    layout = StateLayout(test.state_names, test.param_names)
    q_index = layout.p("pdynmc")

    # A single fixed probe set, so every model is interrogated at identical points.
    probe_x, probe_p = test.x[:4096], test.p[:4096]
    q = probe_p[:, q_index]

    results: list[SweepResult] = []
    for lam in lambdas:
        matrices, corrections, errors, val_mses = [], [], [], []

        for seed in seeds:
            torch.manual_seed(seed)
            physics = PhysicsModel(layout)
            model = GrayBoxSSM(physics, n_param=len(test.param_names))
            model.fit_scalers(train_loader)

            cfg = TrainConfig(
                epochs=epochs, lambda_reg=lam, patience=epochs,
                ckpt_dir=workdir, run_name=f"lam{lam:g}_seed{seed}", log_every=10**9,
            )
            trainer = Trainer(model, cfg, q_index=q_index)
            history = trainer.fit(train_loader, val_loader)

            a_tilde, correction = probe(model, probe_x, probe_p)
            matrices.append(a_tilde)
            corrections.append(correction)
            val_mses.append(min(history.val_mse))

            with torch.no_grad():
                xdot, _ = model(test.x, test.p, test.t)
            errors.append((xdot - test.xdot)[:, layout.s_slice("VBII")].norm(dim=1))

        error = torch.stack(errors).median(dim=0).values
        maxq = test.p[:, q_index] >= 1e4
        results.append(SweepResult(
            lambda_reg=lam,
            pred_disagreement=_relative_spread(corrections),
            matrix_disagreement=_relative_spread(matrices),
            median_error_maxq=float(error[maxq].median()) if maxq.sum() > 10 else float("nan"),
            median_error_all=float(error.median()),
            val_mse=float(np.mean(val_mses)),
        ))
        print(f"  -> lambda={lam:<8g} pred_dis={results[-1].pred_disagreement:.4f} "
              f"matrix_dis={results[-1].matrix_disagreement:.4f} "
              f"err_maxQ={results[-1].median_error_maxq:.4f}")

    return results


def print_table(results: list[SweepResult], n_seeds: int) -> None:
    print(f"\n{'='*82}\nSeed consistency across {n_seeds} seeds\n{'='*82}")
    print(f"{'lambda':>10} {'pred disagr':>13} {'matrix disagr':>15} "
          f"{'err maxQ':>11} {'err all':>10} {'val mse':>10}")
    print("-" * 82)
    for r in results:
        print(f"{r.lambda_reg:>10g} {r.pred_disagreement:>13.4f} "
              f"{r.matrix_disagreement:>15.4f} {r.median_error_maxq:>11.4f} "
              f"{r.median_error_all:>10.4f} {r.val_mse:>10.5f}")
    print("-" * 82)
    print("pred disagr   : spread of dA x + dc across seeds (agreement on dynamics)")
    print("matrix disagr : spread of A_tilde across seeds (agreement on the factorisation)")
    print("Low pred + high matrix = fits well, not identified.")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--data", type=Path, default=Path("data/rocket6g.npz"))
    parser.add_argument("--lambdas", type=float, nargs="+",
                        default=[0.0, 1e-4, 1e-2, 1e-1])
    parser.add_argument("--seeds", type=int, nargs="+", default=[0, 1, 2])
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--workdir", type=Path, default=Path("checkpoints/sweep"))
    args = parser.parse_args(argv)

    args.workdir.mkdir(parents=True, exist_ok=True)
    results = run_sweep(args.data, args.lambdas, args.seeds, args.epochs, args.workdir)
    print_table(results, len(args.seeds))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())