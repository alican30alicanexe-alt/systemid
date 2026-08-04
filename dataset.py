"""Dataset helpers for the gray-box rocket identification workflow.

This module turns a saved trajectory dataset into PyTorch-ready samples and a
train/validation/test split that is stable and reproducible for Colab runs.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

import numpy as np
import torch
from torch.utils.data import DataLoader, Dataset


@dataclass
class TrajectoryDataset(Dataset):
    """Simple dataset backed by a NumPy array of stacked trajectory samples."""

    x: torch.Tensor
    p: torch.Tensor
    t: torch.Tensor
    xdot: torch.Tensor
    run_id: torch.Tensor
    state_names: list[str]
    param_names: list[str]
    #: CADAC's own non-gravitational specific force, body axes, m/s^2. Ground truth
    #: for the identified aerodynamics via ``physics.aerodynamic_truth``; carried
    #: through so analysis can score ``dA x + dc`` against it. Never an input to the
    #: model -- it contains the force the model is supposed to identify.
    #:
    #: Optional, because it is CADAC's name for a CADAC quantity. A domain whose
    #: truth is known in closed form has no simulator to read it from and leaves
    #: this ``None``; everything except the truth-attribution analysis works without it.
    fspb: torch.Tensor | None = None

    def __len__(self) -> int:
        return len(self.x)

    def __getitem__(self, index: int) -> dict[str, torch.Tensor]:
        item = {
            "x": self.x[index],
            "p": self.p[index],
            "t": self.t[index],
            "xdot": self.xdot[index],
            "run_id": self.run_id[index],
        }
        if self.fspb is not None:
            item["fspb"] = self.fspb[index]
        return item

    def trajectories(self) -> Iterator[dict[str, torch.Tensor]]:
        """Yield each trajectory as one contiguous dict of tensors."""
        unique = sorted(self.run_id.unique().tolist())
        for rid in unique:
            mask = self.run_id == rid
            idx = mask.nonzero(as_tuple=False).flatten()
            traj = {
                "x": self.x[idx],
                "p": self.p[idx],
                "t": self.t[idx],
                "xdot": self.xdot[idx],
                "run_id": torch.full((len(idx),), rid, dtype=self.run_id.dtype),
            }
            if self.fspb is not None:
                traj["fspb"] = self.fspb[idx]
            yield traj


def _split_run_ids(
    run_ids: np.ndarray,
    seed: int,
    train_fraction: float = 0.7,
    val_fraction: float = 0.15,
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Partition whole trajectories into train/val/test.

    Splitting is by run, never by sample: consecutive samples within a trajectory
    are one plot step apart and are near-duplicates, so a per-sample split puts
    each validation point beside a training point taken milliseconds earlier and
    reports a validation loss close to the training loss however badly the model
    generalises to an unseen launch.

    Runs are shuffled with ``seed`` first. Monte Carlo runs are ordered by chunk,
    so an unshuffled slice would hand each partition a different contiguous block
    of dispersion draws.
    """
    unique = np.unique(run_ids)
    n = len(unique)
    if n < 3:
        raise ValueError(
            f"need at least 3 runs to split, got {n}. Generate more with "
            "`python3 generator.py -n <N>`."
        )

    unique = np.random.default_rng(seed).permutation(unique)
    train_n = min(max(1, round(train_fraction * n)), n - 2)
    val_n = min(max(1, round(val_fraction * n)), n - train_n - 1)

    return (
        unique[:train_n],
        unique[train_n:train_n + val_n],
        unique[train_n + val_n:],
    )


def _filter_by_run(data: dict[str, np.ndarray], run_ids: np.ndarray) -> dict[str, np.ndarray]:
    keep = np.isin(data["run_id"], run_ids)
    keys = {"x", "p", "t", "xdot", "run_id", "fspb"}
    return {k: v[keep] for k, v in data.items() if k in keys}


def build_loaders(
    data_path: str | Path,
    batch_size: int = 128,
    seed: int = 0,
    train_fraction: float = 0.7,
    val_fraction: float = 0.15,
    num_workers: int = 0,
    dtype: torch.dtype = torch.float32,
) -> tuple[DataLoader, DataLoader, TrajectoryDataset, dict[str, list[str]]]:
    """Load a .npz dataset and return train/val loaders plus a held-out test set.

    ``dtype`` defaults to float32 to match the model's parameters. The ``.npz``
    itself is float64 and should stay that way: float32 spacing at
    ``|SBII| ~ 6.4e6`` m is 0.5 m, so anything that re-differences the stored
    positions in float32 picks up ~10 m/s of quantisation noise -- the failure the
    generator's plot-file precision patch exists to prevent. Nothing in the
    training path re-differences (``xdot`` is precomputed), and float32 relative
    precision costs ~1e-6 m/s^2 in the gravity term against a residual whose
    median is 0.113 m/s^2, so casting down here is safe. Pass ``torch.float64``
    for analysis that touches the raw magnitudes.
    """
    data = np.load(data_path, allow_pickle=True)
    state_names = list(data["state_names"].tolist())
    param_names = list(data["param_names"].tolist())

    # A CADAC dataset without 'fspb' predates the truth column and is a real
    # failure -- the stale-clone trap in CLAUDE.md produced exactly that, and it
    # cost a full 50-run generation. Datasets from other domains never had one, so
    # the check keys off the state names rather than rejecting everything.
    has_fspb = "fspb" in data
    if not has_fspb and any(n.startswith("SBII") for n in state_names):
        raise KeyError(
            f"{data_path} has no 'fspb' array -- it predates the truth column. "
            "Delete the chunk directory and regenerate; see the chunk-cache trap "
            "in CLAUDE.md."
        )

    keys = ("x", "p", "t", "xdot", "run_id") + (("fspb",) if has_fspb else ())
    raw = {k: data[k] for k in keys}
    train_ids, val_ids, test_ids = _split_run_ids(
        raw["run_id"], seed, train_fraction, val_fraction
    )

    def make(run_ids: np.ndarray) -> TrajectoryDataset:
        part = _filter_by_run(raw, run_ids)
        return TrajectoryDataset(
            x=torch.tensor(part["x"], dtype=dtype),
            p=torch.tensor(part["p"], dtype=dtype),
            t=torch.tensor(part["t"], dtype=dtype),
            xdot=torch.tensor(part["xdot"], dtype=dtype),
            run_id=torch.tensor(part["run_id"], dtype=torch.long),
            state_names=state_names,
            param_names=param_names,
            fspb=torch.tensor(part["fspb"], dtype=dtype) if has_fspb else None,
        )

    train_dataset, val_dataset, test_dataset = map(make, (train_ids, val_ids, test_ids))
    print(
        f"[data] split by run: train={len(train_ids)} val={len(val_ids)} "
        f"test={len(test_ids)} runs ({len(train_dataset)} train samples)"
    )

    common = {"batch_size": batch_size, "num_workers": num_workers}
    return (
        DataLoader(train_dataset, shuffle=True, **common),
        DataLoader(val_dataset, shuffle=False, **common),
        test_dataset,
        {"state_names": state_names, "param_names": param_names},
    )


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--data", type=Path, default=Path("data/rocket6g.npz"))
    parser.add_argument("--batch-size", type=int, default=128)
    args = parser.parse_args()
    train_loader, val_loader, test, meta = build_loaders(args.data, batch_size=args.batch_size)
    batch = next(iter(train_loader))
    print({k: tuple(v.shape) for k, v in batch.items()})
    print("states:", meta["state_names"])
    print("params:", meta["param_names"])
