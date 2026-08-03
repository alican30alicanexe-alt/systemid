"""End-to-end entry point: data -> train -> evaluate.

    python3 generator.py -n 200 -o data/rocket6g.npz     # once
    python3 run.py --data data/rocket6g.npz              # train and evaluate

Reproducible from this single script; every stage is importable on its own.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import torch

from dataset import build_loaders
from evaluate import evaluate
from model import GrayBoxSSM
from physics import DEFAULT_KNOWN, MODULES, PhysicsModel, StateLayout, residual_report
from trainer import TrainConfig, Trainer


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--data", type=Path, default=Path("data/rocket6g.npz"))
    parser.add_argument("--epochs", type=int, default=200)
    parser.add_argument("--batch-size", type=int, default=2048)
    parser.add_argument("--lr", type=float, default=2e-3)
    parser.add_argument("--lambda-reg", type=float, default=1e-4)
    parser.add_argument("--hidden", type=int, nargs="+", default=[128, 128])
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--run-name", type=str, default="graybox")
    parser.add_argument("--ckpt-dir", type=Path, default=Path("checkpoints"))
    parser.add_argument("--fig-dir", type=Path, default=Path("figures"))
    parser.add_argument(
        "--learn", nargs="*", default=["aerodynamics"], choices=sorted(MODULES),
        help="subsystems left to the network; the rest are analytical",
    )
    parser.add_argument("--skip-train", action="store_true",
                        help="load the checkpoint and evaluate only")
    args = parser.parse_args(argv)

    torch.manual_seed(args.seed)
    device = "cuda" if torch.cuda.is_available() else "cpu"

    train_loader, val_loader, test, _ = build_loaders(
        args.data, batch_size=args.batch_size, seed=args.seed
    )

    layout = StateLayout(test.state_names, test.param_names)
    known = {name: name not in args.learn for name in DEFAULT_KNOWN}
    physics = PhysicsModel(layout, known=known)

    # Gate: with only the intended subsystem unknown, the analytical residual must
    # be near zero in vacuum and grow with dynamic pressure. A residual flat in
    # pdynmc means a physics module is wrong, and training would bury that error
    # inside the learned correction instead of surfacing it.
    print()
    residual_report(str(args.data), known=known)

    model = GrayBoxSSM.from_data(
        train_loader, physics, n_param=len(test.param_names), hidden=args.hidden
    )
    print(f"\n[model] {sum(p.numel() for p in model.parameters())} parameters on {device}")

    cfg = TrainConfig(
        epochs=args.epochs, lr=args.lr, lambda_reg=args.lambda_reg,
        ckpt_dir=args.ckpt_dir, run_name=args.run_name, device=device,
    )
    trainer = Trainer(model, cfg, q_index=layout.p("pdynmc"))

    if args.skip_train:
        trainer.load_checkpoint()
        print("[train] skipped, checkpoint loaded")
    else:
        trainer.fit(train_loader, val_loader)

    model.to("cpu")
    evaluate(
        test, model, physics,
        history_path=args.ckpt_dir / f"{args.run_name}_history.json",
        fig_dir=args.fig_dir,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())