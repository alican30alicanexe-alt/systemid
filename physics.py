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
GM = 3.9860044e14           # gravitational parameter        - m^3/s^2
C20 = -4.8416685e-4         # 2nd-degree zonal coefficient   - ND
SMAJOR_AXIS = 6378137.0     # WGS-84 semi-major axis         - m
FLATTENING = 3.33528106e-3  # WGS-84 flattening              - ND
WEII3 = 7.292115e-5         # Earth rotation rate            - rad/s
GW_CLONG = 0.0              # Greenwich celestial longitude at t=0 - rad

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
        self, x: Tensor, p: Tensor, t: Tensor, layout: StateLayout,
        A: Tensor, B: Tensor, c: Tensor,
    ) -> None:
        """Add this module's terms in place.

        ``x``/``p`` are ``(batch, n)`` and ``t`` is ``(batch,)`` seconds since
        launch. ``t`` is not decoration: every inertial<-earth-fixed rotation in
        CADAC carries ``WEII3 * time``, so a module that ignores it is wrong by
        the Earth's rotation over the trajectory.
        """

    def exact_blocks(self, layout: StateLayout) -> list[tuple[slice, slice]]:
        """``A`` blocks this module determines exactly, frozen against learning.

        Only claim a block when the physics is exact and complete. Claiming one the
        module merely approximates hides the error from the correction term.
        """
        return []


class KinematicsModule(PhysicsModule):
    """``d(SBII)/dt = VBII``. Exact and complete -- a definition, not a model."""

    name = "kinematics"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        pos, vel = layout.s_slice("SBII"), layout.s_slice("VBII")
        A[:, pos, vel] += torch.eye(3, dtype=A.dtype, device=A.device)

    def exact_blocks(self, layout):
        # Freezing this removes 18 of 36 unknowns for n=6 at zero cost in fidelity.
        return [(layout.s_slice("SBII"), slice(0, layout.n_state))]


class GravityJ2Module(PhysicsModule):
    """WGS-84 J2 gravity as an SDC block on position, transcribed from CADAC.

    This is a line-by-line port of ``cad_grav84`` plus the ``~TGI * GRAVG`` rotation
    in ``newton.cpp``, **not** the textbook inertial-Cartesian J2 formula. The two
    are not the same: CADAC's tangential (north) term carries the opposite sign to
    the textbook one, a 0.030 m/s^2 disagreement at the launch latitude. Since the
    training targets are CADAC's own trajectories, any deviation here is learned as
    aerodynamics. That is small against the 5.25 m/s^2 aerodynamic force at max-Q,
    but the residual's median over a whole ascent is 0.113 m/s^2 and most of the
    trajectory is near-vacuum, where the aerodynamic signal is zero and a 0.030
    m/s^2 bias is the entire measurement. Match the simulator, not the textbook.

    Verified against CADAC's own ``FSPB``: this block plus ``PropulsionModule``
    reproduces ``~TBI*FSPB + ~TGI*GRAVG`` to a median 4.0e-05 m/s^2, and to 8e-06
    m/s^2 at max-Q (2 runs, 37938 samples).

    In geocentric (north/east/down) coordinates, with ``latc`` the geocentric
    latitude and ``r = |SBII|``::

        GRAVG = ( -(GM/r^2) 3 sqrt(5) C20 (a_e/r)^2 sin(latc) cos(latc),
                  0,
                   (GM/r^2) [1 + (3 sqrt(5)/2) C20 (a_e/r)^2 (3 sin^2(latc) - 1)] )

    SDC form: the geocentric position is ``TGI x = (delta, 0, -r)``, so a matrix
    whose only populated column is the third maps it onto ``GRAVG`` exactly, and
    ``A = ~TGI M TGI`` reproduces ``~TGI GRAVG`` to float precision. The empty first
    two columns are what makes this exact rather than approximate -- they discard
    the small north residual ``delta`` left by CADAC's small-angle deflection ``dd``,
    instead of letting it leak into the acceleration.
    """

    name = "gravity"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        pos, vel = layout.s_slice("SBII"), layout.s_slice("VBII")
        r_vec = x[:, pos]
        r = r_vec.norm(dim=1, keepdim=True).clamp_min(1.0)

        # cad_geoc_in: latc = asin(sbii3/dbi), geocentric -- not the geodetic latx.
        sin_latc = (r_vec[:, 2:3] / r).clamp(-1.0, 1.0)
        cos_latc = (1.0 - sin_latc.pow(2)).clamp_min(0.0).sqrt()

        dum1 = GM / r.pow(2)
        dum3 = (SMAJOR_AXIS / r).pow(2)
        g1 = -dum1 * (3.0 * 5.0 ** 0.5) * C20 * dum3 * sin_latc * cos_latc
        g3 = dum1 * (1.0 + 1.5 * 5.0 ** 0.5 * C20 * dum3 * (3.0 * sin_latc.pow(2) - 1.0))

        m = torch.zeros(x.shape[0], 3, 3, dtype=A.dtype, device=A.device)
        m[:, 0, 2] = (-g1 / r)[:, 0]
        m[:, 2, 2] = (-g3 / r)[:, 0]

        tgi = inertial_to_geocentric(p, layout, t)
        A[:, vel, pos] += tgi.transpose(1, 2) @ m @ tgi


class PropulsionModule(PhysicsModule):
    """Gimballed thrust, rotated into inertial coordinates.

    Contributes to ``c``, not ``A`` -- see the module docstring. ``TBI = TBD * TDI``
    reproduces CADAC's ``mat3tr`` and ``cad_tdi84``; specific force is the body-axis
    thrust vector over ``vmass``, since ``newton.cpp`` forms ``FSPB = FAPB/vmass``.

    Thrust does not point along body x
    ----------------------------------
    ``forces.cpp`` has two branches. With ``mtvc == 0`` it adds thrust to ``FAPB[0]``
    alone; with ``mtvc`` in 1..3 it adds the gimballed vector ``FPB`` from
    ``tvc.cpp``::

        FPB = thrust * (cos(eta) cos(zet), cos(eta) sin(zet), -sin(eta))

    ``input_insertion.asc`` sets ``mtvc 2`` at ``time > 10`` and back to 0 at
    second-stage ignition, so the second branch is live for the whole first-stage
    boost -- which is also where dynamic pressure peaks. Modelling thrust as
    ``(T, 0, 0)`` there leaves 0.42 m/s^2 unaccounted for against a 5.2 m/s^2
    aerodynamic force (measured, 2 runs), a 7% bias that ``dA`` would absorb and
    report as aerodynamics.

    One expression covers both branches: CADAC holds ``etax``/``zetx`` at exactly
    zero whenever TVC is inactive (verified across all four flight phases), and at
    zero the direction cosines collapse to ``(1, 0, 0)``. No ``mtvc`` flag is needed
    and none is plotted.
    """

    name = "propulsion"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
        vel = layout.s_slice("VBII")
        # TBI maps inertial -> body, so its transpose maps body -> inertial.
        tbi = body_to_inertial(p, layout, t)
        fpb = thrust_specific_force_body(p, layout)
        c[:, vel] += torch.bmm(tbi.transpose(1, 2), fpb.unsqueeze(-1)).squeeze(-1)


class AerodynamicsModule(PhysicsModule):
    """Placeholder for the subsystem left to the network.

    Present so the enable/disable switch is uniform across subsystems: flipping
    ``aerodynamics`` to known means giving this a real implementation, with nothing
    else in the framework changing. While unknown it contributes nothing, and the
    learned correction carries the aerodynamic force.
    """

    name = "aerodynamics"

    def contribute(self, x, p, t, layout, A, B, c) -> None:
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


def _tdi(lon: Tensor, lat: Tensor, time: Tensor) -> Tensor:
    """``cad_tdi84``: inertial -> geodetic, batched ``(N, 3, 3)``.

    ``time`` enters through ``lon_cel = GW_CLONG + WEII3 * time + lon`` and is the
    reason every caller must thread the real sample time: over a 190 s ascent the
    Earth turns 0.79 deg, which at a thrust specific force of 13-40 m/s^2 is
    0.2-0.5 m/s^2 of misdirected acceleration.
    """
    lon_cel = GW_CLONG + WEII3 * time + lon
    slon, clon = lon_cel.sin(), lon_cel.cos()
    slat, clat = lat.sin(), lat.cos()
    zero = torch.zeros_like(lon_cel)

    return torch.stack([
        torch.stack([-slat * clon, -slat * slon, clat], dim=1),
        torch.stack([-slon, clon, zero], dim=1),
        torch.stack([-clat * clon, -clat * slon, -slat], dim=1),
    ], dim=1)


def inertial_to_geocentric(p: Tensor, layout: StateLayout, time: Tensor) -> Tensor:
    """``TGI`` (inertial -> geocentric), batched ``(N, 3, 3)``.

    ``cad_tgi84``: ``TGD(dd) * TDI(lon, lat, time)``, where ``dd`` is CADAC's
    small-angle deflection of the geodetic normal from the geocentric radial. It
    is built from the *geodetic* ``lonx``/``latx``/``alt``, as in ``newton.cpp``.
    """
    lon = p[:, layout.p("lonx")] * DEG
    lat = p[:, layout.p("latx")] * DEG
    alt = p[:, layout.p("alt")]

    r0 = SMAJOR_AXIS * (
        1.0
        - FLATTENING * (1.0 - (2.0 * lat).cos()) / 2.0
        + 5.0 * FLATTENING ** 2 * (1.0 - (4.0 * lat).cos()) / 16.0
    )
    dd = FLATTENING * (2.0 * lat).sin() * (1.0 - FLATTENING / 2.0 - alt / r0)
    sd, cd = dd.sin(), dd.cos()
    zero, one = torch.zeros_like(dd), torch.ones_like(dd)

    tgd = torch.stack([
        torch.stack([cd, zero, -sd], dim=1),
        torch.stack([zero, one, zero], dim=1),
        torch.stack([sd, zero, cd], dim=1),
    ], dim=1)

    return tgd @ _tdi(lon, lat, time)


def thrust_specific_force_body(p: Tensor, layout: StateLayout) -> Tensor:
    """Modelled thrust per unit mass in body axes, ``(N, 3)`` -- ``FPB/vmass``.

    Transcribes ``tvc.cpp`` lines 118-120::

        FPB = thrust * (cos(eta) cos(zet), cos(eta) sin(zet), -sin(eta))

    ``|FPB| == thrust`` for any deflection: the gimbal redirects thrust, it does not
    throttle it. With ``etax``/``zetx`` at zero -- which is how CADAC holds them
    whenever ``mtvc == 0`` -- this collapses to ``(thrust, 0, 0)``, so the single
    expression covers both branches of ``forces.cpp``.

    Factored out so :class:`PropulsionModule` and :func:`aerodynamic_truth` cannot
    drift apart. They must agree exactly: the truth comparison subtracts this from
    CADAC's ``FSPB``, so any difference between the two would show up as a physics
    error that is really just two transcriptions of the same equation disagreeing.
    """
    thrust = p[:, layout.p("thrust")]
    mass = p[:, layout.p("vmass")].clamp_min(1.0)
    eta = p[:, layout.p("etax")] * DEG
    zet = p[:, layout.p("zetx")] * DEG

    return torch.stack([
        eta.cos() * zet.cos(),
        eta.cos() * zet.sin(),
        -eta.sin(),
    ], dim=1) * (thrust / mass).unsqueeze(1)


def aerodynamic_truth(
    p: Tensor, fspb: Tensor, layout: StateLayout, time: Tensor
) -> Tensor:
    """CADAC's own aerodynamic specific force in inertial coordinates, ``(N, 3)``.

    ``newton.cpp`` integrates ``ABII = ~TBI*FSPB + ~TGI*GRAVG``, where ``FSPB`` is
    every non-gravitational force per unit mass in body axes. Subtracting the
    modelled thrust leaves aerodynamics alone::

        a_aero = ~TBI * (FSPB - FPB/vmass)

    This is ground truth for ``dA x + dc`` -- the quantity the whole framework
    exists to identify -- so it turns "the residual must be aerodynamics, because
    nothing else is left" from an argument into a measurement.

    ``fspb`` comes from the dataset's ``fspb`` array, which is deliberately kept
    out of the parameter vector: it *contains* the aerodynamic force, so feeding it
    to the network as an input would let the model copy the answer.
    """
    tbi = body_to_inertial(p, layout, time)
    extra = fspb - thrust_specific_force_body(p, layout)
    return torch.bmm(tbi.transpose(1, 2), extra.unsqueeze(-1)).squeeze(-1)


def body_to_inertial(p: Tensor, layout: StateLayout, time: Tensor) -> Tensor:
    """``TBI`` (inertial -> body), batched ``(N, 3, 3)``.

    Mirrors CADAC: ``TBD = mat3tr(psi, tht, phi)`` composed with
    ``TDI = cad_tdi84(lon, lat, time)``.
    """
    psi = p[:, layout.p("psibdx")] * DEG
    tht = p[:, layout.p("thtbdx")] * DEG
    phi = p[:, layout.p("phibdx")] * DEG

    sp, cp = psi.sin(), psi.cos()
    st, ct = tht.sin(), tht.cos()
    sph, cph = phi.sin(), phi.cos()

    tbd = torch.stack([
        torch.stack([cp * ct, sp * ct, -st], dim=1),
        torch.stack([cp * st * sph - sp * cph, sp * st * sph + cp * cph, ct * sph], dim=1),
        torch.stack([cp * st * cph + sp * sph, sp * st * cph - cp * sph, ct * cph], dim=1),
    ], dim=1)

    lon = p[:, layout.p("lonx")] * DEG
    lat = p[:, layout.p("latx")] * DEG
    return tbd @ _tdi(lon, lat, time)


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
        modules: Mapping[str, type[PhysicsModule]] | None = None,
    ) -> None:
        self.layout = layout
        self.n_control = n_control
        # A domain other than ROCKET6G supplies its own registry; everything below
        # this line is name-based and dimension-free, so nothing else needs to know.
        self.registry = dict(MODULES if modules is None else modules)

        if known is None:
            # DEFAULT_KNOWN names CADAC's subsystems, so it is only a sensible
            # default for CADAC's registry. For any other one, start with every
            # module analytical and let the caller switch off what it wants learned.
            known = DEFAULT_KNOWN if modules is None else {n: True for n in self.registry}
        self.known = dict(known)

        unknown_names = set(self.known) - set(self.registry)
        if unknown_names:
            raise KeyError(f"no such physics module: {sorted(unknown_names)}")

        self.modules = [
            self.registry[name]() for name, enabled in self.known.items() if enabled
        ]

    def __repr__(self) -> str:
        on = [m.name for m in self.modules]
        off = [n for n, e in self.known.items() if not e]
        return f"PhysicsModel(analytical={on}, learned={off})"

    def __call__(self, x: Tensor, p: Tensor, t: Tensor | None = None):
        """Returns ``(A, B, c)`` shaped ``(N,n,n)``, ``(N,n,m)``, ``(N,n)``.

        ``t`` is seconds since launch and reaches the modules -- it sets the
        Earth's rotation angle in every inertial<-earth-fixed transform. Omitting
        it means evaluating the whole trajectory at lift-off attitude of the Earth;
        the default exists only for quick single-point probes.
        """
        n, batch = self.layout.n_state, x.shape[0]
        opts = {"dtype": x.dtype, "device": x.device}

        A = torch.zeros(batch, n, n, **opts)
        B = torch.zeros(batch, n, self.n_control, **opts)
        c = torch.zeros(batch, n, **opts)

        t = torch.zeros(batch, **opts) if t is None else t.to(**opts).expand(batch)

        for module in self.modules:
            module.contribute(x, p, t, self.layout, A, B, c)
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