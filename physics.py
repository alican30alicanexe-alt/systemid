"""Analytical physics modules for gray-box state-space identification.

The framework identifies dynamics in state-dependent-coefficient (SDC) form::

    xdot = (A_known + dA) x + (B_known + dB) u + (c_known + dc)

where the ``_known`` terms come from analytical physics and the ``d`` terms are
learned. Modules are additive and independently replaceable: each contributes into
a shared ``(A, B, c)`` accumulator, so moving a subsystem from learned to analytical
means enabling a module, not restructuring anything.

Naming, deliberately
--------------------
``A`` here is an SDC (extended-linearisation) factorisation satisfying ``A(x,p)x``
= ``f(x,p)``, **not** a Jacobian ``df/dx``. The two coincide only when ``f`` is
genuinely linear. SDC factorisations are non-unique -- for ``n=6`` there are 36 free
entries producing 6 outputs, so infinitely many ``A`` fit any single sample. Two
things make the solution well-posed, and both are structural rather than optional:
the exact blocks below are frozen via :meth:`PhysicsModel.free_mask`, and the
learned correction is norm-penalised in the loss. Without the penalty, "if the
network learns nothing the system reduces to the analytical model" is aspirational
rather than true.

Why gravity is in ``A`` and thrust is in ``c``
---------------------------------------------
Central-body gravity is ``-(GM/r^3) r`` -- linear in position with a state-dependent
scalar, so it lands exactly in ``A[3:6, 0:3]`` with no singularity (``r`` is ~6.4e6 m
and never approaches zero). J2 is likewise expressible as a matrix on ``r``.

Thrust is not. It is a body-frame vector whose magnitude comes from ``p`` and whose
direction comes from attitude; it does not vanish as ``x -> 0``. Forcing it through
``A x`` would require entries scaling like ``(T/m)/|x|``, which is unbounded exactly
where the vehicle is slow -- i.e. at liftoff. It belongs in the affine term ``c``,
which is why ``c`` is present from the start rather than deferred.

State/parameter layout is defined by ``generator.DEFAULT_STATE`` /
``DEFAULT_PARAMS``; :class:`StateLayout` resolves names to indices so modules never
hard-code column numbers.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Mapping, Sequence

import torch
from torch import Tensor

# CADAC global_constants.hpp, reproduced exactly so A_known matches the simulator
# rather than a textbook approximation. A mismatch here would be silently absorbed
# by the learned correction, which is precisely what the gray-box split exists to
# prevent.
GM = 3.9860044e14        # gravitational parameter        - m^3/s^2
C20 = -4.8416685e-4      # 2nd-degree zonal coefficient   - ND
SMAJOR_AXIS = 6378137.0  # WGS-84 semi-major axis         - m
WEII3 = 7.292115e-5      # Earth rotation rate            - rad/s
GW_CLONG = 0.0           # Greenwich celestial longitude at t=0 - rad

DEG = torch.pi / 180.0


@dataclass(frozen=True)
class StateLayout:
    """Resolves state/parameter names to tensor indices."""

    state_names: Sequence[str]
    param_names: Sequence[str]

    @property
    def n_state(self) -> int:
        return len(self.state_names)

    def s(self, name: str) -> int:
        return list(self.state_names).index(name)

    def p(self, name: str) -> int:
        return list(self.param_names).index(name)

    def s_slice(self, prefix: str) -> slice:
        """Index range of a 3-vector block, e.g. ``SBII`` -> ``slice(0, 3)``."""
        idx = [i for i, n in enumerate(self.state_names) if n.startswith(prefix)]
        if len(idx) != 3 or idx != list(range(idx[0], idx[0] + 3)):
            raise KeyError(f"{prefix} is not a contiguous 3-vector in the state")
        return slice(idx[0], idx[0] + 3)


class PhysicsModule(ABC):
    """One analytical subsystem. Contributions are additive into ``(A, B, c)``."""

    name: str = "module"

    @abstractmethod
    def contribute(
        self, x: Tensor, p: Tensor, layout: StateLayout,
        A: Tensor, B: Tensor, c: Tensor,
    ) -> None:
        """Add this module's terms in place. ``x``/``p`` are ``(batch, n)``."""

    def exact_blocks(self, layout: StateLayout) -> list[tuple[slice, slice]]:
        """``A`` blocks this module determines exactly, frozen against learning.

        Only claim a block when the physics is exact and complete. Claiming one the
        module merely approximates hides the error from the correction term.
        """
        return []


class KinematicsModule(PhysicsModule):
    """``d(SBII)/dt = VBII``. Exact and complete -- a definition, not a model."""

    name = "kinematics"

    def contribute(self, x, p, layout, A, B, c) -> None:
        pos, vel = layout.s_slice("SBII"), layout.s_slice("VBII")
        A[:, pos, vel] += torch.eye(3, dtype=A.dtype, device=A.device)

    def exact_blocks(self, layout):
        # Freezing this removes 18 of 36 unknowns for n=6 at zero cost in fidelity.
        return [(layout.s_slice("SBII"), slice(0, layout.n_state))]


class GravityJ2Module(PhysicsModule):
    """WGS-84 J2 gravity as an SDC block on position.

    CADAC computes this in geocentric coordinates (``cad_grav84``); the equivalent
    inertial Cartesian form is used here so it composes as a matrix on ``SBII``::

        a = -(GM/r^3)[ I + (3/2) J2 (a_e/r)^2 diag(1-5s^2, 1-5s^2, 3-5s^2) ] r

    with ``s = z/r`` and ``J2 = -sqrt(5) C20``.
    """

    name = "gravity"

    def contribute(self, x, p, layout, A, B, c) -> None:
        pos, vel = layout.s_slice("SBII"), layout.s_slice("VBII")
        r_vec = x[:, pos]
        r = r_vec.norm(dim=1, keepdim=True).clamp_min(1.0)

        j2 = -(5.0 ** 0.5) * C20
        s2 = (r_vec[:, 2:3] / r) ** 2
        k = 1.5 * j2 * (SMAJOR_AXIS / r) ** 2

        diag = torch.stack(
            [1.0 + k[:, 0] * (1.0 - 5.0 * s2[:, 0]),
             1.0 + k[:, 0] * (1.0 - 5.0 * s2[:, 0]),
             1.0 + k[:, 0] * (3.0 - 5.0 * s2[:, 0])],
            dim=1,
        )
        A[:, vel, pos] += torch.diag_embed(-GM / r.pow(3) * diag)


class PropulsionModule(PhysicsModule):
    """Thrust along the body x-axis, rotated into inertial coordinates.

    Contributes to ``c``, not ``A`` -- see the module docstring. ``TBI = TBD * TDI``
    reproduces CADAC's ``mat3tr`` and ``cad_tdi84``; specific force is ``thrust/vmass``
    since ``forces.cpp`` adds thrust to ``FAPB[0]``.
    """

    name = "propulsion"

    def contribute(self, x, p, layout, A, B, c) -> None:
        vel = layout.s_slice("VBII")
        thrust = p[:, layout.p("thrust")]
        mass = p[:, layout.p("vmass")].clamp_min(1.0)

        tbi = body_to_inertial(p, layout, x.new_tensor(0.0))
        # TBI maps inertial -> body, so its transpose maps body -> inertial. The
        # body-frame thrust vector is (T, 0, 0), so we need only TBI's first row.
        c[:, vel] += tbi[:, 0, :] * (thrust / mass).unsqueeze(1)


class AerodynamicsModule(PhysicsModule):
    """Placeholder for the subsystem left to the network.

    Present so the enable/disable switch is uniform across subsystems: flipping
    ``aerodynamics`` to known means giving this a real implementation, with nothing
    else in the framework changing. While unknown it contributes nothing, and the
    learned correction carries the aerodynamic force.
    """

    name = "aerodynamics"

    def contribute(self, x, p, layout, A, B, c) -> None:
        return


#: Registry. Adding RotationModel / EnvironmentModel / ControlModel later means
#: adding an entry here.
MODULES: dict[str, type[PhysicsModule]] = {
    "kinematics": KinematicsModule,
    "gravity": GravityJ2Module,
    "propulsion": PropulsionModule,
    "aerodynamics": AerodynamicsModule,
}

#: The default gray-box split: everything analytical except the aerodynamics.
DEFAULT_KNOWN: dict[str, bool] = {
    "kinematics": True,
    "gravity": True,
    "propulsion": True,
    "aerodynamics": False,
}


def body_to_inertial(p: Tensor, layout: StateLayout, time: Tensor) -> Tensor:
    """``TBI`` (inertial -> body), batched ``(N, 3, 3)``.

    Mirrors CADAC: ``TBD = mat3tr(psi, tht, phi)`` composed with
    ``TDI = cad_tdi84(lon, lat, time)``.
    """
    psi = p[:, layout.p("psibdx")] * DEG
    tht = p[:, layout.p("thtbdx")] * DEG
    phi = p[:, layout.p("phibdx")] * DEG
    lon = p[:, layout.p("lonx")] * DEG
    lat = p[:, layout.p("latx")] * DEG

    sp, cp = psi.sin(), psi.cos()
    st, ct = tht.sin(), tht.cos()
    sph, cph = phi.sin(), phi.cos()
    zero = torch.zeros_like(psi)

    tbd = torch.stack([
        torch.stack([cp * ct, sp * ct, -st], dim=1),
        torch.stack([cp * st * sph - sp * cph, sp * st * sph + cp * cph, ct * sph], dim=1),
        torch.stack([cp * st * cph + sp * sph, sp * st * cph - cp * sph, ct * cph], dim=1),
    ], dim=1)

    lon_cel = GW_CLONG + WEII3 * time + lon
    slon, clon = lon_cel.sin(), lon_cel.cos()
    slat, clat = lat.sin(), lat.cos()

    tdi = torch.stack([
        torch.stack([-slat * clon, -slat * slon, clat], dim=1),
        torch.stack([-slon, clon, zero], dim=1),
        torch.stack([-clat * clon, -clat * slon, -slat], dim=1),
    ], dim=1)

    return tbd @ tdi


class PhysicsModel:
    """Composes enabled modules into ``(A_known, B_known, c_known)``.

    Example::

        model = PhysicsModel(layout, known={"gravity": True, "aerodynamics": False})
        A, B, c = model(x, p, t)
        xdot = torch.bmm(A, x.unsqueeze(-1)).squeeze(-1) + c
    """

    def __init__(
        self,
        layout: StateLayout,
        known: Mapping[str, bool] | None = None,
        n_control: int = 0,
    ) -> None:
        self.layout = layout
        self.n_control = n_control
        self.known = dict(DEFAULT_KNOWN if known is None else known)

        unknown_names = set(self.known) - set(MODULES)
        if unknown_names:
            raise KeyError(f"no such physics module: {sorted(unknown_names)}")

        self.modules = [
            MODULES[name]() for name, enabled in self.known.items() if enabled
        ]

    def __repr__(self) -> str:
        on = [m.name for m in self.modules]
        off = [n for n, e in self.known.items() if not e]
        return f"PhysicsModel(analytical={on}, learned={off})"

    def __call__(self, x: Tensor, p: Tensor, t: Tensor | None = None):
        """Returns ``(A, B, c)`` shaped ``(N,n,n)``, ``(N,n,m)``, ``(N,n)``."""
        n, batch = self.layout.n_state, x.shape[0]
        opts = {"dtype": x.dtype, "device": x.device}

        A = torch.zeros(batch, n, n, **opts)
        B = torch.zeros(batch, n, self.n_control, **opts)
        c = torch.zeros(batch, n, **opts)

        if t is None:
            t = torch.zeros(batch, **opts)

        for module in self.modules:
            module.contribute(x, p, self.layout, A, B, c)
        return A, B, c

    def free_mask(self) -> Tensor:
        """``(n, n)`` mask, 1 where the learned correction ``dA`` may act.

        Rows a module determines exactly are zeroed, so the network cannot spend
        capacity contradicting a definition.
        """
        mask = torch.ones(self.layout.n_state, self.layout.n_state)
        for module in self.modules:
            for rows, cols in module.exact_blocks(self.layout):
                mask[rows, cols] = 0.0
        return mask

    def known_dynamics(self, x: Tensor, p: Tensor, t: Tensor | None = None) -> Tensor:
        """The analytical prediction alone -- what you get if the network learns nothing."""
        A, _, c = self(x, p, t)
        return torch.bmm(A, x.unsqueeze(-1)).squeeze(-1) + c


def residual_report(npz_path: str, known: Mapping[str, bool] | None = None) -> Tensor:
    """Print what the analytical modules leave behind, bucketed by dynamic pressure.

    This is the gate to clear before training anything: with aerodynamics the only
    unknown, the residual must be near zero in vacuum and grow with dynamic
    pressure. A residual that is flat in ``pdynmc`` means a module is wrong, and the
    network would quietly absorb that error as if it were aerodynamics.
    """
    import numpy as np

    data = np.load(npz_path, allow_pickle=True)
    layout = StateLayout(list(data["state_names"]), list(data["param_names"]))
    x = torch.tensor(data["x"])
    p = torch.tensor(data["p"])
    t = torch.tensor(data["t"])
    xdot = torch.tensor(data["xdot"])

    model = PhysicsModel(layout, known=known)
    residual = xdot - model.known_dynamics(x, p, t)
    accel = residual[:, layout.s_slice("VBII")].norm(dim=1)

    print(model)
    print(f"  kinematic rows   max abs error {residual[:, layout.s_slice('SBII')].abs().max():.3e} m/s")
    print(f"  accel residual   median {accel.median():.4f}  p99 {accel.quantile(0.99):.4f} m/s^2")

    q = p[:, layout.p("pdynmc")]
    for lo, hi in ((0.0, 1e1), (1e1, 1e3), (1e3, 1e4), (1e4, float("inf"))):
        sel = (q >= lo) & (q < hi)
        if sel.sum() > 10:
            print(f"  pdynmc {lo:>8.0f}-{hi:<9.0f} Pa  n={int(sel.sum()):6d}  "
                  f"median {accel[sel].median():8.4f} m/s^2")
    return residual


if __name__ == "__main__":
    import sys

    residual_report(sys.argv[1] if len(sys.argv) > 1 else "data/rocket6g.npz")