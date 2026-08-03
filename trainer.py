"""Training loop for the gray-box state-space model.

Checkpointing, LR scheduling, early stopping and loss history. Metrics are
reported per dynamic-pressure regime as well as in aggregate, because the
aggregate alone is misleading here -- see :meth:`Trainer.bucket_metrics`.
"""

from __future__ import annotations

import json
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Iterable

import torch
from torch import Tensor
from torch.utils.data import DataLoader

from model import GrayBoxSSM, graybox_loss

#: Dynamic-pressure edges (Pa) for regime-resolved reporting.
Q_BUCKETS: tuple[float, ...] = (0.0, 1e1, 1e3, 1e4, float("inf"))


@dataclass
class TrainConfig:
    epochs: int = 200
    lr: float = 2e-3
    weight_decay: float = 0.0

    #: Identifiability knob. Zero leaves the SDC factorisation non-unique; too
    #: large suppresses genuine missing physics along with the ambiguity.
    lambda_reg: float = 1e-4

    #: Epochs without validation improvement before stopping.
    patience: int = 25
    lr_patience: int = 8
    lr_factor: float = 0.5
    min_lr: float = 1e-6
    grad_clip: float = 1.0

    ckpt_dir: Path = Path("checkpoints")
    run_name: str = "graybox"
    device: str = "cpu"
    log_every: int = 10

    def __post_init__(self) -> None:
        self.ckpt_dir = Path(self.ckpt_dir)


@dataclass
class History:
    train_loss: list[float] = field(default_factory=list)
    train_mse: list[float] = field(default_factory=list)
    val_mse: list[float] = field(default_factory=list)
    lr: list[float] = field(default_factory=list)
    epoch_time: list[float] = field(default_factory=list)

    def to_json(self, path: Path) -> None:
        path.write_text(json.dumps(asdict(self), indent=2))


class Trainer:
    """Fits a :class:`GrayBoxSSM`.

    Example::

        trainer = Trainer(model, TrainConfig(epochs=100), q_index=layout.p("pdynmc"))
        history = trainer.fit(train_loader, val_loader)
    """

    def __init__(
        self, model: GrayBoxSSM, config: TrainConfig | None = None,
        q_index: int | None = None,
    ) -> None:
        self.model = model
        self.cfg = config or TrainConfig()
        self.q_index = q_index
        self.device = torch.device(self.cfg.device)
        self.model.to(self.device)

        self.opt = torch.optim.Adam(
            model.parameters(), lr=self.cfg.lr, weight_decay=self.cfg.weight_decay
        )
        self.sched = torch.optim.lr_scheduler.ReduceLROnPlateau(
            self.opt, mode="min", factor=self.cfg.lr_factor,
            patience=self.cfg.lr_patience, min_lr=self.cfg.min_lr,
        )
        self.history = History()
        self.best_val = float("inf")
        self.best_epoch = -1

        self.cfg.ckpt_dir.mkdir(parents=True, exist_ok=True)
        self.ckpt_path = self.cfg.ckpt_dir / f"{self.cfg.run_name}.pt"

    def _to_device(self, batch: dict[str, Tensor]) -> dict[str, Tensor]:
        return {k: v.to(self.device) for k, v in batch.items()}

    def _step(self, batch: dict[str, Tensor], train: bool) -> dict[str, float]:
        batch = self._to_device(batch)
        xdot, parts = self.model(batch["x"], batch["p"], batch["t"])
        loss, logs = graybox_loss(
            xdot, batch["xdot"], parts, self.model.residual_scale, self.cfg.lambda_reg
        )
        if train:
            self.opt.zero_grad(set_to_none=True)
            loss.backward()
            if self.cfg.grad_clip:
                torch.nn.utils.clip_grad_norm_(self.model.parameters(), self.cfg.grad_clip)
            self.opt.step()
        logs["n"] = len(batch["x"])
        return logs

    def _run_epoch(self, loader: Iterable, train: bool) -> dict[str, float]:
        self.model.train(train)
        totals: dict[str, float] = {}
        count = 0
        with torch.set_grad_enabled(train):
            for batch in loader:
                logs = self._step(batch, train)
                n = logs.pop("n")
                count += n
                for k, v in logs.items():
                    totals[k] = totals.get(k, 0.0) + v * n
        return {k: v / max(count, 1) for k, v in totals.items()}

    @torch.no_grad()
    def bucket_metrics(self, loader: Iterable) -> dict[str, float]:
        """Median acceleration error per dynamic-pressure regime.

        The residual is heavily skewed -- most samples sit in near-vacuum where
        aerodynamic force is ~0, while the mean square is dominated by the few
        max-Q samples. A single aggregate number therefore hides which regime
        actually improved, and can move opposite to the per-regime errors.
        """
        self.model.eval()
        errors, qs = [], []
        vel = self.model.layout.s_slice("VBII")
        for batch in loader:
            batch = self._to_device(batch)
            xdot, _ = self.model(batch["x"], batch["p"], batch["t"])
            errors.append((xdot - batch["xdot"])[:, vel].norm(dim=1))
            if self.q_index is not None:
                qs.append(batch["p"][:, self.q_index])

        error = torch.cat(errors)
        out = {"median_all": error.median().item()}
        if not qs:
            return out

        q = torch.cat(qs)
        for lo, hi in zip(Q_BUCKETS[:-1], Q_BUCKETS[1:]):
            sel = (q >= lo) & (q < hi)
            if sel.sum() > 10:
                out[f"median_q{lo:g}_{hi:g}"] = error[sel].median().item()
        return out

    def save_checkpoint(self, epoch: int, val_mse: float) -> None:
        torch.save(
            {
                "epoch": epoch,
                "val_mse": val_mse,
                "model": self.model.state_dict(),
                "optimizer": self.opt.state_dict(),
                "config": asdict(self.cfg) | {"ckpt_dir": str(self.cfg.ckpt_dir)},
                "known": self.model.physics.known,
                "state_names": list(self.model.layout.state_names),
                "param_names": list(self.model.layout.param_names),
            },
            self.ckpt_path,
        )

    def load_checkpoint(self, path: Path | None = None) -> dict:
        ckpt = torch.load(path or self.ckpt_path, map_location=self.device, weights_only=False)
        self.model.load_state_dict(ckpt["model"])
        return ckpt

    def fit(self, train_loader: DataLoader, val_loader: DataLoader) -> History:
        cfg = self.cfg
        baseline = self.bucket_metrics(val_loader)
        print(f"[train] analytical baseline: {_fmt(baseline)}")
        print(f"[train] {cfg.epochs} epochs max, patience {cfg.patience}\n")

        since_improved = 0
        for epoch in range(1, cfg.epochs + 1):
            start = time.time()
            train_logs = self._run_epoch(train_loader, train=True)
            val_logs = self._run_epoch(val_loader, train=False)
            val_mse = val_logs["mse"]

            self.sched.step(val_mse)
            self.history.train_loss.append(train_logs["loss"])
            self.history.train_mse.append(train_logs["mse"])
            self.history.val_mse.append(val_mse)
            self.history.lr.append(self.opt.param_groups[0]["lr"])
            self.history.epoch_time.append(time.time() - start)

            if val_mse < self.best_val - 1e-9:
                self.best_val, self.best_epoch, since_improved = val_mse, epoch, 0
                self.save_checkpoint(epoch, val_mse)
                marker = " *"
            else:
                since_improved += 1
                marker = ""

            if epoch % cfg.log_every == 0 or marker:
                print(
                    f"epoch {epoch:4d}  train {train_logs['mse']:.5f}  "
                    f"val {val_mse:.5f}  lr {self.history.lr[-1]:.2e}{marker}"
                )

            if since_improved >= cfg.patience:
                print(f"\n[train] early stop at epoch {epoch} "
                      f"(no improvement for {cfg.patience})")
                break

        print(f"[train] best epoch {self.best_epoch}, val mse {self.best_val:.5f}")
        self.load_checkpoint()
        print(f"[train] analytical : {_fmt(baseline)}")
        print(f"[train] gray-box   : {_fmt(self.bucket_metrics(val_loader))}")

        self.history.to_json(self.cfg.ckpt_dir / f"{self.cfg.run_name}_history.json")
        return self.history


def _fmt(metrics: dict[str, float]) -> str:
    return "  ".join(f"{k.replace('median_', '')}={v:.4f}" for k, v in metrics.items())