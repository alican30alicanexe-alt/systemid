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

    def __len__(self) -> int:
        return len(self.x)

    def __getitem__(self, index: int) -> dict[str, torch.Tensor]:
        return {
            "x": self.x[index],
            "p": self.p[index],
            "t": self.t[index],
            "xdot": self.xdot[index],
            "run_id": self.run_id[index],
        }

    def trajectories(self) -> Iterator[dict[str, torch.Tensor]]:
        """Yield each trajectory as one contiguous dict of tensors."""
        unique = sorted(self.run_id.unique().tolist())
        for rid in unique:
            mask = self.run_id == rid
            idx = mask.nonzero(as_tuple=False).flatten()
            yield {
                "x": self.x[idx],
                "p": self.p[idx],
                "t": self.t[idx],
                "xdot": self.xdot[idx],
                "run_id": torch.full((len(idx),), rid, dtype=self.run_id.dtype),
            }


def _split_run_ids(run_ids: np.ndarray, seed: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    unique = np.unique(run_ids)
    rng = np.random.default_rng(seed)
    rng.shuffle(unique)
    n = len(unique)
    train_n = max(1, int(round(0.7 * n)))
    val_n = max(1, int(round(0.15 * n)))
    test_n = max(1, n - train_n - val_n)
    if train_n + val_n + test_n != n:
        train_n = n - val_n - test_n

    train_ids = unique[:train_n]
    val_ids = unique[train_n:train_n + val_n]
    test_ids = unique[train_n + val_n:train_n + val_n + test_n]
    if len(test_ids) == 0 and n > 1:
        test_ids = np.array([unique[-1]])
    if len(val_ids) == 0 and n > 1:
        val_ids = np.array([unique[-1]])
    return train_ids, val_ids, test_ids


def _filter_by_run(data: dict[str, np.ndarray], run_ids: np.ndarray) -> dict[str, np.ndarray]:
    keep = np.isin(data["run_id"], run_ids)
    return {k: v[keep] for k, v in data.items() if k in {"x", "p", "t", "xdot", "run_id"}}


def build_loaders(
    data_path: str | Path,
    batch_size: int = 128,
    seed: int = 0,
    train_fraction: float = 0.7,
    val_fraction: float = 0.15,
    num_workers: int = 0,
) -> tuple[DataLoader, DataLoader, TrajectoryDataset, dict[str, list[str]]]:
    """Load a .npz dataset and return train/val loaders plus a held-out test set."""
    data = np.load(data_path, allow_pickle=True)
    state_names = list(data["state_names"].tolist())
    param_names = list(data["param_names"].tolist())

    x = torch.tensor(data["x"], dtype=torch.float32)
    p = torch.tensor(data["p"], dtype=torch.float32)
    t = torch.tensor(data["t"], dtype=torch.float32)
    xdot = torch.tensor(data["xdot"], dtype=torch.float32)
    run_id = torch.tensor(data["run_id"], dtype=torch.long)

    unique = sorted(run_id.unique().tolist())
    if len(unique) < 3:
        train_ids = np.array(unique[: max(1, int(len(unique) * train_fraction))], dtype=np.int64)
        val_ids = np.array(unique[len(train_ids):len(train_ids) + 1], dtype=np.int64)
        test_ids = np.array(unique[-1:], dtype=np.int64)
    else:
        n = len(unique)
        train_n = max(1, int(round(train_fraction * n)))
        val_n = max(1, int(round(val_fraction * n)))
        test_n = max(1, n - train_n - val_n)
        if train_n + val_n + test_n != n:
            train_n = n - val_n - test_n
        train_ids = np.array(unique[:train_n], dtype=np.int64)
        val_ids = np.array(unique[train_n:train_n + val_n], dtype=np.int64)
        test_ids = np.array(unique[train_n + val_n:train_n + val_n + test_n], dtype=np.int64)

    train_data = _filter_by_run({"x": x.numpy(), "p": p.numpy(), "t": t.numpy(), "xdot": xdot.numpy(), "run_id": run_id.numpy()}, train_ids)
    val_data = _filter_by_run({"x": x.numpy(), "p": p.numpy(), "t": t.numpy(), "xdot": xdot.numpy(), "run_id": run_id.numpy()}, val_ids)
    test_data = _filter_by_run({"x": x.numpy(), "p": p.numpy(), "t": t.numpy(), "xdot": xdot.numpy(), "run_id": run_id.numpy()}, test_ids)

    train_dataset = TrajectoryDataset(
        x=torch.tensor(train_data["x"], dtype=torch.float32),
        p=torch.tensor(train_data["p"], dtype=torch.float32),
        t=torch.tensor(train_data["t"], dtype=torch.float32),
        xdot=torch.tensor(train_data["xdot"], dtype=torch.float32),
        run_id=torch.tensor(train_data["run_id"], dtype=torch.long),
        state_names=state_names,
        param_names=param_names,
    )
    val_dataset = TrajectoryDataset(
        x=torch.tensor(val_data["x"], dtype=torch.float32),
        p=torch.tensor(val_data["p"], dtype=torch.float32),
        t=torch.tensor(val_data["t"], dtype=torch.float32),
        xdot=torch.tensor(val_data["xdot"], dtype=torch.float32),
        run_id=torch.tensor(val_data["run_id"], dtype=torch.long),
        state_names=state_names,
        param_names=param_names,
    )
    test_dataset = TrajectoryDataset(
        x=torch.tensor(test_data["x"], dtype=torch.float32),
        p=torch.tensor(test_data["p"], dtype=torch.float32),
        t=torch.tensor(test_data["t"], dtype=torch.float32),
        xdot=torch.tensor(test_data["xdot"], dtype=torch.float32),
        run_id=torch.tensor(test_data["run_id"], dtype=torch.long),
        state_names=state_names,
        param_names=param_names,
    )

    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True, num_workers=num_workers)
    val_loader = DataLoader(val_dataset, batch_size=batch_size, shuffle=False, num_workers=num_workers)
    return train_loader, val_loader, test_dataset, {"state_names": state_names, "param_names": param_names}


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
