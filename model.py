"""Gray-box neural state-space model.

Composes the analytical modules of :mod:`physics` with a learned correction::

    xdot = (A_known + dA) x + (B_known + dB) u + (c_known + dc)

The network sees ``[x, p]`` and emits only ``dA`` and ``dc``. It never sees the
target directly and never predicts the dynamics outright: with the correction at
zero the model *is* the analytical model, exactly.

Conditioning
------------
A naive ``dA`` is unusable. ``|SBII|`` is ~6.4e6 m while the aerodynamic residual
is ~0.04 m/s^2, so the entries coupling them are ~1e-8 -- eight orders of magnitude
below the weight scale an MLP initialises at. The network would have to learn its
own output scaling before learning any physics.

So the network predicts a conditioned ``A_tilde`` and the physical matrix is
recovered by a scaling sandwich::

    dA = diag(r) @ A_tilde @ diag(1/s)      dc = r * c_tilde

with ``s`` the RMS magnitude of each state and ``r`` the RMS magnitude of the
analytical residual. Then ``dA x = r * (A_tilde @ (x/s))``: both factors are O(1),
and ``A_tilde`` entries are O(1) too. The sandwich is diagonal and constant, so
``dA`` remains exactly recoverable in physical units -- interpretability is not
traded away for conditioning. Note ``s`` is an RMS, not a standard deviation: the
SDC product needs a *linear* rescaling of ``x``, and subtracting a mean would make
it affine and break the identity above. The MLP's own input normalisation is
separate and does use mean/std.

Identifiability
---------------
An SDC factorisation is non-unique -- 36 free entries producing 6 outputs means
infinitely many ``A`` fit any single sample, and different seeds would recover
different, equally-good matrices while the loss looks fine. Two mechanisms make it
well posed, and neither is optional:

- ``free_mask`` zeroes rows the physics determines exactly (the kinematic block is
  a definition, not a model), removing 18 of 36 entries from the problem.
- :func:`graybox_loss` penalises ``||A_tilde||^2 + ||c_tilde||^2``, selecting the
  minimum-norm correction. This is what makes "if the network learns nothing, the
  system reduces to the analytical model" a property rather than an aspiration.

The final layer is zero-initialised, so training *starts* at the analytical model
and moves away only as far as the data demands.
"""

from __future__ import annotations

from typing import Iterable, Sequence

import torch
from torch import Tensor, nn

from physics import PhysicsModel, StateLayout


def _mlp(sizes: Sequence[int], activation: type[nn.Module]) -> list[nn.Module]:
    layers: list[nn.Module] = []
    for a, b in zip(sizes[:-1], sizes[1:]):
        layers += [nn.Linear(a, b), activation()]
    return layers


class GrayBoxSSM(nn.Module):
    """Analytical physics plus a learned state-space correction.

    Example::

        model = GrayBoxSSM.from_data(train_loader, physics)
        xdot, _ = model(x, p, t)
    """

    def __init__(
        self,
        physics: PhysicsModel,
        n_param: int,
        hidden: Sequence[int] = (128, 128),
        activation: type[nn.Module] = nn.SiLU,
        learn_delta_c: bool = True,
    ) -> None:
        super().__init__()
        self.physics = physics
        self.layout = physics.layout
        self.learn_delta_c = learn_delta_c

        n = self.layout.n_state
        self.n_state, self.n_param = n, n_param

        sizes = [n + n_param, *hidden]
        head = nn.Linear(sizes[-1], n * n + (n if learn_delta_c else 0))
        # Zero-init: the model starts exactly at the analytical solution.
        nn.init.zeros_(head.weight)
        nn.init.zeros_(head.bias)
        self.net = nn.Sequential(*_mlp(sizes, activation), head)

        mask = physics.free_mask()
        self.register_buffer("mask", mask)
        # A row with no free entry is fully determined by physics, so its affine
        # term must not be learned either.
        self.register_buffer("row_mask", (mask.sum(dim=1) > 0).to(mask.dtype))

        for name, size in (("x_mean", n), ("x_std", n), ("p_mean", n_param),
                           ("p_std", n_param), ("state_scale", n), ("residual_scale", n)):
            self.register_buffer(name, torch.zeros(size) if "mean" in name
                                 else torch.ones(size))

    # ------------------------------------------------------------------ #
    # statistics
    # ------------------------------------------------------------------ #

    @torch.no_grad()
    def fit_scalers(self, batches: Iterable[dict[str, Tensor]], max_batches: int = 64) -> None:
        """Estimate normalisation statistics from training data only.

        Fitting on anything else leaks the evaluation distribution into the model.
        """
        xs, ps, residuals = [], [], []
        for i, batch in enumerate(batches):
            if i >= max_batches:
                break
            x, p, t = batch["x"], batch["p"], batch["t"]
            xs.append(x)
            ps.append(p)
            residuals.append(batch["xdot"] - self.physics.known_dynamics(x, p, t))

        x = torch.cat(xs)
        p = torch.cat(ps)
        residual = torch.cat(residuals)

        self.x_mean.copy_(x.mean(0))
        self.x_std.copy_(x.std(0).clamp_min(1e-6))
        self.p_mean.copy_(p.mean(0))
        self.p_std.copy_(p.std(0).clamp_min(1e-6))

        # RMS, not std -- the SDC product needs a linear rescaling of x.
        self.state_scale.copy_(x.pow(2).mean(0).sqrt().clamp_min(1e-6))
        self.residual_scale.copy_(residual.pow(2).mean(0).sqrt().clamp_min(1e-6))

        print(f"[scalers] state RMS    {self.state_scale.tolist()}")
        print(f"[scalers] residual RMS {self.residual_scale.tolist()}")

    # ------------------------------------------------------------------ #
    # forward
    # ------------------------------------------------------------------ #

    def _encode(self, x: Tensor, p: Tensor) -> Tensor:
        return torch.cat(
            [(x - self.x_mean) / self.x_std, (p - self.p_mean) / self.p_std], dim=1
        )

    def corrections(self, x: Tensor, p: Tensor) -> tuple[Tensor, Tensor]:
        """Conditioned corrections ``(A_tilde, c_tilde)``, masked.

        These are what the regulariser penalises -- they are dimensionless, so one
        ``lambda`` weights every state equally.
        """
        n = self.n_state
        out = self.net(self._encode(x, p))

        a_tilde = out[:, : n * n].view(-1, n, n) * self.mask
        c_tilde = (
            out[:, n * n:] * self.row_mask
            if self.learn_delta_c
            else torch.zeros(x.shape[0], n, dtype=x.dtype, device=x.device)
        )
        return a_tilde, c_tilde

    def forward(
        self, x: Tensor, p: Tensor, t: Tensor | None = None
    ) -> tuple[Tensor, dict[str, Tensor]]:
        """Returns ``(xdot, parts)`` where ``parts`` carries the pieces for loss/analysis."""
        A_known, _, c_known = self.physics(x, p, t)
        xdot_known = torch.bmm(A_known, x.unsqueeze(-1)).squeeze(-1) + c_known

        a_tilde, c_tilde = self.corrections(x, p)
        # dA x = r * (A_tilde @ (x / s)); see the conditioning note above.
        correction = self.residual_scale * (
            torch.bmm(a_tilde, (x / self.state_scale).unsqueeze(-1)).squeeze(-1) + c_tilde
        )

        return xdot_known + correction, {
            "xdot_known": xdot_known,
            "a_tilde": a_tilde,
            "c_tilde": c_tilde,
            "A_known": A_known,
        }

    # ------------------------------------------------------------------ #
    # interpretation
    # ------------------------------------------------------------------ #

    @torch.no_grad()
    def delta_matrices(self, x: Tensor, p: Tensor) -> tuple[Tensor, Tensor]:
        """``(dA, dc)`` in physical units -- the identified local correction.

        This is the interpretable output. ``A_known + dA`` is the identified local
        state-space matrix; it is an SDC factorisation, not a Jacobian.
        """
        a_tilde, c_tilde = self.corrections(x, p)
        dA = (
            self.residual_scale.view(1, -1, 1) * a_tilde
            / self.state_scale.view(1, 1, -1)
        )
        return dA, self.residual_scale * c_tilde

    @classmethod
    def from_data(
        cls, loader: Iterable[dict[str, Tensor]], physics: PhysicsModel,
        n_param: int | None = None, **kwargs,
    ) -> "GrayBoxSSM":
        """Build a model and fit its scalers to ``loader`` in one step."""
        if n_param is None:
            n_param = len(physics.layout.param_names)
        model = cls(physics, n_param=n_param, **kwargs)
        model.fit_scalers(loader)
        return model


def graybox_loss(
    xdot_pred: Tensor,
    xdot_true: Tensor,
    parts: dict[str, Tensor],
    residual_scale: Tensor,
    lambda_reg: float = 1e-3,
) -> tuple[Tensor, dict[str, float]]:
    """Normalised one-step MSE plus a norm penalty on the correction.

    The error is divided by ``residual_scale`` so every state contributes
    comparably; raw MSE over a state whose rows differ by ~6 orders of magnitude
    would optimise the largest row and ignore the rest.

    ``lambda_reg`` is the identifiability knob. At zero, the SDC factorisation is
    non-unique and the recovered matrices are arbitrary among the equally-good
    solutions; raising it selects the minimum-norm correction to the analytical
    model. Too high and genuine missing physics gets suppressed along with the
    ambiguity, so it is worth sweeping.
    """
    mse = (((xdot_pred - xdot_true) / residual_scale) ** 2).mean()
    penalty = parts["a_tilde"].pow(2).mean() + parts["c_tilde"].pow(2).mean()
    total = mse + lambda_reg * penalty
    return total, {
        "loss": total.detach().item(),
        "mse": mse.detach().item(),
        "penalty": penalty.detach().item(),
    }